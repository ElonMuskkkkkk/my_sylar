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
        //上述断言代码意义不明,看看得了......
        //其实是写死了use_caller只能为true,但给的接口似乎可以是false，个人感觉是设计问题
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
        setThis();
        //当前线程不是caller线程的话,创建一个主协程
        if(sylar::GetThreadID() != m_rootThread){
            t_scheduler_fiber = Fiber::GetThis().get();
        }
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        Fiber::ptr cb_fiber;
    }
}