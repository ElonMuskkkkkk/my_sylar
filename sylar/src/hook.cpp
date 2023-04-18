#include "../inc/hook.h"
#include "../inc/io_manager.h"
#include "../inc/fiber.h"
#include "../inc/conf.h"
#include "../inc/log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar{

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
    sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");


#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)

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
}