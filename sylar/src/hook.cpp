#include "../inc/hook.h"
#include "../inc/io_manager.h"
#include "../inc/fiber.h"
#include "../inc/conf.h"
#include "../inc/log.h"
#include "../inc/fd_manager.h"
#include "../inc/macro.h"
#include <errno.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar{

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
    sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");


#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)  \
    XX(nanosleep)\
    XX(socket)

    static thread_local bool t_hook_enable = false;

    bool is_hook_enable()
    {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag)
    {
        t_hook_enable = flag;
    }
    void hook_init() 
    {
        static bool is_inited = false;
        if(is_inited) {
            return;
        }
    /// 初始化hook过的函数，去动态库里找
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
    }
    // static uint64_t s_connect_timeout = -1;
    struct _HookIniter {
        _HookIniter() {
            hook_init();
            // s_connect_timeout = g_tcp_connect_timeout->getValue();

            // g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
            //         SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from "
            //                                 << old_value << " to " << new_value;
            //         s_connect_timeout = new_value;
            // });
        }
    };
    static _HookIniter s_hook_initer;

    struct timer_info {
        int cancelled = 0;
    };

    template <typename OriginFun, typename... Args>
    static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                         uint32_t event, int timeout_so, Args &...args)
    {
        //如果不能hook，使用原始函数
        if(!sylar::t_hook_enable)
        {
            return fun(fd, std::forward<Args>(args)...);
        }
        sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
        //如果文件上下文没有，使用原始函数
        if(!ctx){
            return fun(fd, std::forward<Args>(args)...);
        }
        if(ctx->isClose()){
            errno = EBADF;
            return -1;
        }
        //不是socket文件或者用户态没有设置非阻塞，使用原函数
        if(!ctx->isSocket() || ctx->getUserNonBlock()) {
            return fun(fd, std::forward<Args>(args)...);
        }
        uint64_t to = ctx->getTimeOut(timeout_so);
        std::shared_ptr<timer_info> tinfo(new timer_info);
        //是socket，且用户态设置了非阻塞
    retry:
        ssize_t n = fun(fd, std::forward<Args>(args)...);
        while(n == -1 && errno == EINTR) {
            n = fun(fd, std::forward<Args>(args)...);
        }
        //如果发生阻塞，执行如下处理，没有的话，返回n,相当于只执行了原始函数
        if(n == -1 && errno == EAGAIN) {
            // 找到当前线程的IO调度器
            sylar::IOManager* iom = sylar::IOManager::GetThis();
            // 添加一个定时器
            sylar::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);
            // 不等于-1，证明有超时时间，添加一个定时器，超时时间为to
            if(to != (uint64_t)-1) {
                //添加定时器的内容为超时时间到了之后，触发该事件
                timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                    auto t = winfo.lock();
                    if(!t || t->cancelled) {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (sylar::IOManager::Event)(event));
                }, winfo);
            }
            // 添加定时器要触发的事件
            int rt = iom->addEvent(fd, (sylar::IOManager::Event)(event));
            if(SYLAR_UNLIKELY(rt)) {
                // 事件添加失败，取消定时器
                SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                    << fd << ", " << event << ")";
                if(timer) {
                    timer->cancel();
                }
                return -1;
            } else {
                // 添加成功，让出该协程，去执行其他协程
                sylar::Fiber::YieldToHold();
                // 唤醒回来后，如果timer还存在，取消timer
                if(timer) {
                    timer->cancel();
                }
                // 如果触发了定时器
                if(tinfo->cancelled) {
                    errno = tinfo->cancelled;
                    return -1;
                }
                goto retry;
            }
        }
        
        return n;
    }    
}

extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

    unsigned int sleep(useconds_t usec)
    {
        if(!sylar::t_hook_enable){
            return sleep_f(usec);
        }
        sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
        sylar::IOManager* iom = sylar::IOManager::GetThis();
        iom->addTimer(usec * 1000, std::bind((void(sylar::Scheduler::*)
                (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule
                ,iom, fiber, -1));
        sylar::Fiber::YieldToHold();
        return 0;
    }
    int usleep(useconds_t usec) {
        if(!sylar::t_hook_enable) {
            return usleep_f(usec);
        }
        sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
        sylar::IOManager* iom = sylar::IOManager::GetThis();
        iom->addTimer(usec / 1000, std::bind((void(sylar::Scheduler::*)
                (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule
                ,iom, fiber, -1));
        sylar::Fiber::YieldToHold();
        return 0;
    }
    int nanosleep(const struct timespec *req, struct timespec *rem) {
        if(!sylar::t_hook_enable) {
            return nanosleep_f(req, rem);
        }

        int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
        sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
        sylar::IOManager* iom = sylar::IOManager::GetThis();
        iom->addTimer(timeout_ms, std::bind((void(sylar::Scheduler::*)
                (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule
                ,iom, fiber, -1));
        sylar::Fiber::YieldToHold();
        return 0;
    }

    int socket(int domain,int type,int protocol)
    {
        if(!sylar::t_hook_enable){
            return socket_f(domain, type, protocol);
        }
        int fd = socket_f(domain, type, protocol);
        sylar::FdMgr::GetInstance()->get(fd,true);
        return fd;
    }

    int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)
    {

    }

}