#ifndef __SYLAR_FIBER_H_
#define __SYLAR_FIBER_H_

#include <memory>
#include <ucontext.h>
#include "thread.h"

namespace sylar{
    class Scheduler;
    /**
     * @brief 协程类
     */
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
        friend Scheduler;

    public:
        typedef std::shared_ptr<Fiber> ptr;
        /**
         * @brief 协程状态
        */
        enum State
        {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPT
        };
    private:
        /**
         * @brief 无参构造，每个线程的第一个协程
        */
        Fiber();
    public:
        /**
         * @brief 构造函数
         * @param[in] cb 协程执行的函数
         * @param[in] stacksize 协程栈大小
         * @param[in] use_caller 是否在MainFiber上调度
        */
        Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

        /**
         * @brief 析构函数
        */
        ~Fiber();

        /**
         * @brief 重置协程执行函数，并设置状态
         * @pre getState() 为 TERM EXCEPT INIT
         * @post getState() = INIT
        */
        void reset(std::function<void()> cb);

        /**
         * @brief 将协程切换到运行状态
         * @pre getState() != EXEC
         * @post getState() == EXEC
        */
        void swapIn();

        /**
         * @brief 将协程切换到后台
        */
        void swapOut();
        /**
         * @brief 将当前线程切换为执行状态
         * @pre 执行的为当前线程的主协程
        */
        void call();
        /**
         * @brief 当前线程切换到后台
         * @pre 执行的为该协程
         * @post 返回到该协程的主协程
        */
        void back();
        /**
         * @brief 返回协程id
        */
        uint64_t getId() const { return m_id; }
        /**
         * @brief 返回协程状态
        */
        State getState() const { return m_state; }

    public:
        /**
         * @brief 设置当前线程的运行协程
         * @param[in] f 协程
        */
        static void SetThis(Fiber *f);
        /**
         * @brief 返回当前所在的协程
        */
        static Fiber::ptr GetThis();
        /**
         * @brief 当前协程切换到后台，并设置为Ready状态
         * @post getThis() = READY
        */
        static void YieldToReady();
        /**
         * @brief 当前协程切换到后台，并设置为hold状态
         * @post getThis() = HOLD
        */
        static void YieldToHold();
        /**
         * @brief 返回当前协程的总数量
        */
        static uint64_t TotalFibers();
        /**
         * @brief 协程执行函数
         * @post 执行完成后返回到主协程
        */
        static void MainFunc();
        /**
         * @brief 协程执行函数
         * @post 执行完成后返回到线程调度协程
        */
        static void CallerMainFunc();
        /**
         * @brief 获取当前协程的id
        */
        static uint64_t GetFiberID();

    private:
        uint64_t m_id = 0;
        uint32_t m_stacksize = 0;
        State m_state = INIT;
        ucontext_t m_ctx;
        void *m_stack = nullptr;
        std::function<void()> m_cb;
    };
}

#endif