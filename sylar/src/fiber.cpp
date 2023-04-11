#include "../inc/fiber.h"
#include "../inc/log.h"
#include "../inc/conf.h"
#include "../inc/macro.h"
#include <atomic>

namespace sylar {
    static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id{0};
    static std::atomic<uint64_t> s_fiber_count{0};
    
    static thread_local Fiber *t_fiber = nullptr;
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
        
    }

    /**
     * @brief 重置协程执行函数，并设置状态
     * @pre getState() 为 TERM EXCEPT INIT
     * @post getState() = INIT
    */
    void Fiber::reset(std::function<void()> cb)
    {

    }

    /**
     * @brief 将协程切换到运行状态
     * @pre getState() != EXEC
     * @post getState() == EXEC
    */
    void Fiber::swapIn()
    {

    }

    /**
     * @brief 将协程切换到后台
    */
    void Fiber::swapOut()
    {

    }
    /**
     * @brief 将当前线程切换为执行状态
     * @pre 执行的为当前线程的主协程
    */
    void Fiber::call()
    {

    }
    /**
     * @brief 当前线程切换到后台
     * @pre 执行的为该协程
     * @post 返回到该协程的主协程
    */
    void Fiber::back()
    {

    }

    /**
     * @brief 设置当前线程的运行协程
     * @param[in] f 协程
    */
    void Fiber::SetThis(Fiber *f)
    {

    }

    /**
     * @brief 返回当前所在的协程
    */
    Fiber::ptr Fiber::GetThis()
    {

    }
    /**
     * @brief 当前协程切换到后台，并设置为Ready状态
     * @post getThis() = READY
    */
    void Fiber::YieldToReady()
    {

    }
    /**
     * @brief 当前协程切换到后台，并设置为hold状态
     * @post getThis() = HOLD
    */
    void Fiber::YieldToHold()
    {

    }
    /**
     * @brief 返回当前协程的总数量
    */
    uint64_t Fiber::TotalFibers()
    {

    }
    /**
     * @brief 协程执行函数
     * @post 执行完成后返回到主协程
    */
    void Fiber::MainFunc()
    {

    }
    /**
     * @brief 协程执行函数
     * @post 执行完成后返回到线程调度协程
    */
    void Fiber::CallerMainFunc()
    {

    }
    /**
     * @brief 获取当前协程的id
    */
    uint64_t Fiber::GetFiberID()
    {
        
    }

}