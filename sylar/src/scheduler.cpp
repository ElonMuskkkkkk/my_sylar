#include "../inc/scheduler.h"
#include "../inc/log.h"
#include "../inc/macro.h"


namespace sylar{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
    //指向当前线程的调度器
    static thread_local Scheduler *t_scheduler = nullptr;
    //指向当前线程的调度协程
    static thread_local Fiber *t_scheduler_fiber = nullptr;
    Scheduler::Scheduler(size_t threads,bool use_caller,const std::string& name)
    :m_name(name) {
        SYLAR_ASSERT(threads > 0);
        if(use_caller){
            sylar::Fiber::GetThis(); //用来初始化一个主协程
            --threads; //当前线程作为线程之一
            SYLAR_ASSERT(GetThis() == nullptr); //确定该线程只有一个协程调度器
            t_scheduler = this;
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            sylar::Thread::SetName(m_name);

            t_scheduler_fiber = m_rootFiber.get();
            m_rootThread = sylar::GetThreadID();
            m_threadIds.push_back(m_rootThread);
        }
        else{
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler(){
        SYLAR_ASSERT(m_stopping);
        if(GetThis() == this){
            t_scheduler = nullptr;
        }
    }

    Scheduler* Scheduler::GetThis()
    {
        return t_scheduler;
    }

    Fiber* Scheduler::GetMainFiber()
    {
        return t_scheduler_fiber;
    }

    void Scheduler::start()
    {
        MutexType::Lock lock(m_mutex);
        if(!m_stopping){
            // 说明启动了
            return;
        }
        m_stopping = false;
        SYLAR_ASSERT(m_threads.empty());
        m_threads.resize(m_threadCount);
        for(size_t i = 0; i < m_threadCount; ++i) {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                                , m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getID());
        }
        // lock.unlock();
        // if (m_rootFiber)
        // {
        //     //m_rootFiber->swapIn();
        //     m_rootFiber->call();
        //     SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
        // }
    }
    void Scheduler::stop()
    {
        m_autoStop = true;
        //调度协程存在，线程数量为0且调度协程处于TERM或者INIT状态
        //停止调度器
        if(m_rootFiber && m_threadCount == 0 
            && (m_rootFiber->getState() == Fiber::TERM
            || m_rootFiber->getState() == Fiber::INIT))
        {
            SYLAR_LOG_INFO(g_logger) << this << " stoped";
            m_stopping = true;
            if(stopping()){
                return;
            }
        }
        //如果上述流程未执行，可能不存在caller线程，m_rootThread = -1
        //可能当前线程不是caller线程
        if(m_rootThread != -1){
            SYLAR_ASSERT(GetThis() == this);
        } else {
            SYLAR_ASSERT(GetThis() != this);
        }
        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; i++){
            tickle(); //唤醒线程，结束对应线程
        }
        if(m_rootFiber){
            tickle();
        }
        //当前调度器关闭失败的话，继续执行调度协程
        if(m_rootFiber){
            if(!stopping()){
                m_rootFiber->call();
            }
        }
        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(Mutex);
            thrs.swap(m_threads);
        }
        for(auto& i : thrs){
            i->join();
        }
    }
    void Scheduler::setThis()
    {
        t_scheduler = this;
    }
    void Scheduler::run()
    {
        SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
        // set_hook_enable(true);
        setThis(); //设置调度器调度的是当前线程
        //当前线程不是caller线程的话,创建一个主协程
        if(sylar::GetThreadID() != m_rootThread){
            t_scheduler_fiber = Fiber::GetThis().get();
        }
        //创建一个空置线程
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        
        Fiber::ptr cb_fiber;
        //创建一个待添加元素
        FiberAndThread ft;

        while(true){
            ft.reset();
            bool tickle_me = false;
            bool is_active = false;
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                //找到一个可调度的协程
                while(it != m_fibers.end())
                {
                    if(it->thread != -1 && it->thread != sylar::GetThreadID()){
                        it++;
                        tickle_me = true;
                        continue;
                    }
                    SYLAR_ASSERT(it->fiber || it->cb);
                    if(it->fiber && it->fiber->getState() == sylar::Fiber::EXEC){
                        it++;
                        continue;
                    }
                    ft = *it;
                    m_fibers.erase(it++);
                    ++m_activeThreadCount;
                    is_active = true;
                    break;
                }
                //如果迭代器不等于end,证明取出了合适的ft
                tickle_me |= it != m_fibers.end();
            }
            // 通知调度器？？？
            if(tickle_me){
                tickle();
            }
            //第一种情况：注册的是协程，确保切成状态不是终止或异常，启动协程
            if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                            && ft.fiber->getState() != Fiber::EXCEPT))
            {
                ft.fiber->swapIn(); //执行该协程
                --m_activeThreadCount;
                //协程执行结束后，根据协程状态，选择放入调度队列，或者结束执行
                if(ft.fiber->getState() == Fiber::READY){
                    schedule(ft.fiber);
                }
                else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT){
                    ft.fiber->m_state = Fiber::HOLD;    
                    schedule(ft.fiber);
                }
                ft.reset();
            }
            else if(ft.cb){
                if(cb_fiber){
                    cb_fiber->reset(ft.cb);
                }else{
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();
                cb_fiber->swapIn(); // 调度当前协程
                --m_activeThreadCount;
                if(cb_fiber->getState() == Fiber::READY){
                    schedule(cb_fiber);
                    cb_fiber.reset();
                }
                else if (cb_fiber->getState() == Fiber::TERM || cb_fiber->getState() == Fiber::EXCEPT)
                {
                    cb_fiber->reset(nullptr);
                }
                else{
                    cb_fiber->m_state = Fiber::HOLD;
                    //schedule(cb_fiber); // 自行添加的代码
                    cb_fiber.reset();
                }
            }
            else{
                //未获取到可调度协程
                if(is_active){
                    --m_activeThreadCount;
                    continue;
                }
                if(idle_fiber->getState() == Fiber::TERM){
                    SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                    break;
                }
                ++m_idleThreadCount;
                idle_fiber->swapIn();
                --m_idleThreadCount;
                if(idle_fiber->getState() != Fiber::TERM
                        && idle_fiber->getState() != Fiber::EXCEPT) 
                {
                    idle_fiber->m_state = Fiber::HOLD;
                }
            }
        }
    }
    void Scheduler::idle()
    {
        SYLAR_LOG_INFO(g_logger) << "idle";
        while(!stopping()) {
            sylar::Fiber::YieldToHold();
        }
    }

    bool Scheduler::stopping()
    {
        MutexType::Lock lock(m_mutex);
        return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::tickle()
    {
        SYLAR_LOG_INFO(g_logger) << "tickle";
    }
}