#include "../inc/sylar.h"
#include <fcntl.h>


namespace sylar{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
    
    IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event)
    {
        switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext");
        }
        throw std::invalid_argument("getContext invalid event");
    }

    void IOManager::FdContext::resetContext(EventContext& ctx) {
        ctx.scd = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(Event event)
    {
        SYLAR_ASSERT(events & event);
        events = (Event)(events & ~event);
        EventContext ctx = getContext(event);
        if(ctx.cb) {
            ctx.scd->schedule(&ctx.cb);
        } else {
            ctx.scd->schedule(&ctx.fiber);
        }
        ctx.scd = nullptr;
        return;
    }

    //初始化调度器、初始化Epoll，启动IOManager
    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name)
    {
        m_efd = epoll_create(5000);
        SYLAR_ASSERT(m_efd > 0)
        int rt = pipe(m_tickleFds);  /*标记*/
        SYLAR_ASSERT(!rt);
        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0]; //读端口
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);//设置文件非阻塞
        SYLAR_ASSERT(!rt);
        contextResize(32);
        start();
    }

    IOManager::~IOManager()
    {
        stop();
        close(m_efd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);
        for(size_t i = 0; i < m_fdContexts.size(); ++i) {
            if(m_fdContexts[i]) {
                delete m_fdContexts[i];
            }
        }
    }
    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext *fd_ctx = nullptr;  //socket事件上下文，是IO调度器操作的基本单元之一
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size() > fd){
            fd_ctx = m_fdContexts[fd];
            lock.unlock();
        }
        else{
            lock.unlock();
            RWMutexType::WriteLock lock2(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexType::Lock lock3(fd_ctx->mutex);
        //一个句柄，不可能重复加载事件
        if(SYLAR_UNLIKELY(fd_ctx->events & event)){
            SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << " event=" << (EPOLL_EVENTS)event
                    << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
            SYLAR_ASSERT(!(fd_ctx->events & event));   
        }
        //注册或者修改一个epoll event
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        //socket事件上下文类主要记录事件类型是读还是写
        epevent.events = EPOLLET | fd_ctx->events | event;
        epevent.data.ptr = fd_ctx;
        int rt = epoll_ctl(m_efd, op, fd, &epevent);//监听事件的添加或调整
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_efd << ", "
                << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
        }
        //成功注册epoll事件后，刷新队列中对应的元素
        ++m_pendingEventCount;
        fd_ctx->events = (Event)(fd_ctx->events | event); //修改数组结构上的元素
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);//获取对应的事件上下文
        SYLAR_ASSERT(!event_ctx.scd && !event_ctx.fiber && !event_ctx.cb);//确保为空
        event_ctx.scd = Scheduler::GetThis();

        //添加这个事件，证明要么首次添加，要么事件被执行过一次了，所以对应的时间上下文必定为空
        //如此，就要为新的事件分配对应的执行主体与执行逻辑
        if(cb) {
            event_ctx.cb.swap(cb);
        } else {
            event_ctx.fiber = Fiber::GetThis();
            SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC
                        ,"state=" << event_ctx.fiber->getState());
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        // 获取要删除的元素
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        // 要删除的事件大概率是存在的，否则报错
        if (SYLAR_UNLIKELY(!(fd_ctx->events & event)))
        {
            return false;
        }
        Event new_Event = (Event)(fd_ctx->events & ~event);
        int op = new_Event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_Event;
        epevent.data.ptr = fd_ctx;
        int rt = epoll_ctl(m_efd, op, fd, &epevent);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_efd << ", "
                << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }
        //成功删除事件后,重置对应的元素
        --m_pendingEventCount;
        fd_ctx->events = new_Event;
        FdContext::EventContext event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }
    
    bool IOManager::cancelEvent(int fd, Event event)
    {
        //获取要操作的元素
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size() <= fd){
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();
        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(SYLAR_UNLIKELY(!(fd_ctx->events & event))) {
            return false;
        }
        // 修改或删除事件
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;
        int rt = epoll_ctl(m_efd, op, fd, &epevent);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_efd << ", "
                << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
         //获取要操作的元素
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size() <= fd){
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();
        //将该元素锁住并操作
        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(!fd_ctx->events) {
            return false;
        }
        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_efd, op, fd, &epevent);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_efd << ", "
                << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }
        // 删除对应的事件
        if(fd_ctx->events & READ) {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if(fd_ctx->events & WRITE) {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }

        SYLAR_ASSERT(fd_ctx->events == 0);
        return true;
    }

    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);
        for (size_t i = 0; i < m_fdContexts.size(); i++){
            if(!m_fdContexts[i]){
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }
    IOManager* IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }
    
    void IOManager::tickle()
    {
        if (!hasIdleThreads())
        {
            return;
        }
        int rt = write(m_tickleFds[1], "T", 1);
        //std::cout << "**********" << std::endl;
        SYLAR_ASSERT(rt == 1);
    }

    void IOManager::idle()
    {
        SYLAR_LOG_DEBUG(g_logger) << "idle";
        const uint64_t MAX_EVENTS = 256;
        epoll_event *events = new epoll_event[MAX_EVENTS]();
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
            delete[] ptr;
        });
        while(true){
            if(SYLAR_UNLIKELY(stopping())){
                SYLAR_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
                break;
            }

            int rt = 0;
            do{
                static const int MAX_TIMEOUT = 3000;
                rt = epoll_wait(m_efd, events, MAX_EVENTS, MAX_TIMEOUT);
                if(rt < 0 && errno == EINTR) {
                } else {
                    break;
                }
            } while (true);
            // std::vector<std::function<void()> > cbs;
            // listExpiredCb(cbs);
            // if(!cbs.empty()) {
            //     //SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
            //     schedule(cbs.begin(), cbs.end());
            //     cbs.clear();
            // }

            // //if(SYLAR_UNLIKELY(rt == MAX_EVNETS)) {
            // //    SYLAR_LOG_INFO(g_logger) << "epoll wait events=" << rt;
            // //}

            //遍历监听到的事件
            for(int i = 0; i < rt; ++i) {
                epoll_event& event = events[i];
                //处理无效消息
                if(event.data.fd == m_tickleFds[0]) {
                    uint8_t dummy[256];
                    while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                    continue;
                }
                //对Socket事件上下文的处理
                FdContext* fd_ctx = (FdContext*)event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->mutex);
                //如果出现错误和中断，那么相关的读写事件要进行处理
                if(event.events & (EPOLLERR | EPOLLHUP)) {
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
                }
                int real_events = NONE;
                if(event.events & EPOLLIN) {
                    real_events |= READ;
                }
                if(event.events & EPOLLOUT) {
                    real_events |= WRITE;
                }
                //在其他地方已经被处理掉了，因为有多个监听的idle
                if((fd_ctx->events & real_events) == NONE) {
                    continue;
                }

                //提取剩余事件，重新注册
                int left_events = (fd_ctx->events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;

                int rt2 = epoll_ctl(m_efd, op, fd_ctx->fd, &event);
                if(rt2) {
                    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_efd << ", "
                        << op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                        << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                //SYLAR_LOG_INFO(g_logger) << " fd=" << fd_ctx->fd << " events=" << fd_ctx->events
                //                         << " real_events=" << real_events;
                if(real_events & READ) {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if(real_events & WRITE) {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }
            // 切换出去执行任务协程
            Fiber::ptr cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();

            raw_ptr->swapOut();
        }
    }

    bool IOManager::stopping()
    {
        return Scheduler::stopping() && m_pendingEventCount == 0;
    }

}