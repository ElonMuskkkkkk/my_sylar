#include "../inc/fiber.h"
#include "../inc/log.h"
#include "../inc/conf.h"
#include "../inc/macro.h"
#include "../inc/scheduler.h"
#include <atomic>

namespace sylar {
    static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id{0};
    static std::atomic<uint64_t> s_fiber_count{0};
    
    static thread_local Fiber *t_fiber = nullptr;  //正在执行的协程
    static thread_local Fiber::ptr t_threadFiber = nullptr;

    //启动时注册配置项
    static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
        Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

    class MallocStackAllocator {
    public:
        static void* Alloc(size_t size) {
            return malloc(size);
        }
        static void Dealloc(void *vp, size_t size)
        {
            return free(vp);
        }
    };

    using StackAllocator = MallocStackAllocator;


    Fiber::Fiber()
    {
        m_state = EXEC;
        SetThis(this);
        if(getcontext(&m_ctx)){
            SYLAR_ASSERT2(false, "getcontext");
        }
        ++s_fiber_count;
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
    }
    /**
     * @brief 构造函数
     * @param[in] cb 协程执行的函数
     * @param[in] stacksize 协程栈大小
     * @param[in] use_caller 是否在MainFiber上调度
    */
    Fiber::Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false)
    : m_id(++s_fiber_id),
      m_cb(cb)
    {
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

        m_stack = StackAllocator::Alloc(m_stacksize);
        if(getcontext(&m_ctx)) {
            SYLAR_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_size = m_stacksize;
        m_ctx.uc_stack.ss_sp = m_stack;
        if(!use_caller){
            makecontext(&m_ctx, &Fiber::MainFunc, 0);
        }
        else{
            makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
        }
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
    }

    /**
     * @brief 析构函数
    */
    Fiber::~Fiber()
    {
        --s_fiber_count;
        if(m_stack){
            //子协程析构流程
            //只有处于终止、异常、初始化三种状态的协程可以被析构
            SYLAR_ASSERT(m_state == TERM ||
                         m_state == EXCEPT ||
                         m_state == INIT);
            StackAllocator::Dealloc(m_stack, m_stacksize);
        }
        else{
            //主协程的析构流程，先确认是主协程（没有回调函数，始终处于EXEC状态）
            SYLAR_ASSERT(!m_cb);
            SYLAR_ASSERT(m_state == EXEC);

            Fiber *cur = t_fiber;
            if(cur == this){
                SetThis(nullptr);
            }
        }
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                              << " total=" << s_fiber_count;
    }

    /**
     * @brief 重置协程执行函数，并设置状态
     * @pre getState() 为 TERM EXCEPT INIT
     * @post getState() = INIT
    */
    void Fiber::reset(std::function<void()> cb)
    {
        SYLAR_ASSERT(m_stack); //确定当前协程为子协程
        SYLAR_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
        m_cb = cb;
        if(getcontext(&m_ctx)){
            SYLAR_ASSERT2(false, "getcontext");
        }

        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_size = m_stacksize;
        m_ctx.uc_stack.ss_sp = m_stack;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = INIT;
    }

    /**
     * @brief 将协程切换到运行状态
     * @pre getState() != EXEC
     * @post getState() == EXEC
    */
    void Fiber::swapIn()
    {
        SetThis(this);
        SYLAR_ASSERT(m_state != EXEC);
        m_state = EXEC;
        if(swapcontext(&Scheduler::GetMainFiber()->m_ctx,&m_ctx))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    /**
     * @brief 将协程切换到后台
    */
    void Fiber::swapOut()
    {
        SetThis(Scheduler::GetMainFiber());
        if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }
    /**
     * @brief 将当前线程切换为执行状态
     * @pre 执行的为当前线程的主协程
    */
    void Fiber::call()
    {
        SetThis(this);
        m_state = EXEC;
        if(swapcontext(&t_threadFiber->m_ctx,&m_ctx)){
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }
    /**
     * @brief 当前线程切换到后台
     * @pre 执行的为该协程
     * @post 返回到该协程的主协程
    */
    void Fiber::back()
    {
        SetThis(t_threadFiber.get());
        if(swapcontext(&m_ctx,&t_threadFiber->m_ctx)){
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    /**
     * @brief 设置当前线程的运行协程
     * @param[in] f 协程
    */
    void Fiber::SetThis(Fiber *f)
    {
        t_fiber = f;
    }

    /**
     * @brief 返回当前所在的协程
    */
    Fiber::ptr Fiber::GetThis()
    {
        //t_fiber 指向的是当前正在执行的协程，若存在，则返回
        if(t_fiber){
            return t_fiber->shared_from_this();
        }
        //否则，创建主协程
        Fiber::ptr main_fiber(new Fiber);
        SYLAR_ASSERT(t_fiber == main_fiber.get());
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }
    /**
     * @brief 当前协程切换到后台，并设置为Ready状态
     * @post getThis() = READY
    */
    void Fiber::YieldToReady()
    {
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = READY;
        cur->swapOut();
    }
    /**
     * @brief 当前协程切换到后台，并设置为hold状态
     * @post getThis() = HOLD
    */
    void Fiber::YieldToHold()
    {
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = HOLD;
        cur->swapOut();
    }
    /**
     * @brief 返回当前协程的总数量
    */
    uint64_t Fiber::TotalFibers()
    {
        return s_fiber_count;
    }
    /**
     * @brief 协程执行函数
     * @post 执行完成后返回到主协程
    */
    void Fiber::MainFunc()
    {
        Fiber::ptr cur = GetThis(); 
        SYLAR_ASSERT(cur);  //确定当前协程非空
        try{
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        } catch(std::exception& ex) {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                << " fiber_id=" << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
        } catch (...){
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                << " fiber_id=" << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
        }
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut();

        SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
        
    }
    /**
     * @brief 协程执行函数
     * @post 执行完成后返回到线程调度协程
    */
    void Fiber::CallerMainFunc()
    {
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur);
        try {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        } catch (std::exception& ex) {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                << " fiber_id=" << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
        } catch (...) {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                << " fiber_id=" << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
        }

        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->back();
        SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
    }
    /**
     * @brief 获取当前协程的id
    */
    uint64_t Fiber::GetFiberID()
    {
        if(t_fiber){
            return t_fiber->getId();
        }
        return 0;
    }
}