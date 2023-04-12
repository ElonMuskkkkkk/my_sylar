#ifndef __SYLAR_SCHEDULER_H_
#define __SYLAR_SCHEDULER_H_

#include <iostream>
#include <vector>
#include <list>
#include <iostream>
#include "fiber.h"
#include "thread.h"


namespace sylar{
    /**
     * @brief 协程调度器
     * @details 封装的N-M的协程调度器，内部有一个线程池，支持协程在线程池里切换
    */
    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;

        /**
         * @brief 构造函数
         * @param[in] threads 线程数量
         * @param[in] use_caller 是否使用当前调用线程
         * @param[in] name 协程调度器名称
        */
        Scheduler(size_t threads = 1, bool use_call = true, const std::string &name = "");
        /**
         * @brief 虚析构
        */
        virtual ~Scheduler();

        const std::string getName() const { return m_name; }
        /**
         * @brief 返回当前协程调度器
        */
        static Scheduler *GetThis();
        /**
         * @brief 返回当前协程调度器的调度协程
        */
        static Fiber *GetMainFiber();

        /**
         * @brief 启动
        */
        void start();
        /**
         * @brief 结束
        */
        void stop();

        /**
         * @brief 调度协程，将单个协程放入协程组中
        */
        template <typename FiberOrCb>
        void schedule(FiberOrCb fc,int thread = -1){
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = schedulerNoLock(fc, thread);
            }
            if(need_tickle){
                tickle();
            }
        }
        /**
         * @brief 批量放入协程
        */
        template <typename InputIterator>
        void schedule(InputIterator begin,InputIterator end){
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while(begin != end){
                    need_tickle = schedulerNoLock(&*begin, -1) || need_tickle;
                    ++begin;
                }
            }
            if(need_tickle){
                tickle();
            }
        }
    protected:
        /**
         * @brief 通知协程调度器有任务了
        */
        void tickle();
        /**
         * @brief 协程调度函数
        */
        void run();

        /**
         * @brief 返回是否可以停止
        */
        virtual bool stopping();
        /**
         * @brief 协程无任务可调度时执行idle函数
        */
        virtual void idle();

        /**
         * @brief 设置当前的协程调度器
        */
        void setThis();
        /**
         * @brief 是否有空闲线程
        */
        bool hasIdleThreads() { return m_idleThreadCount > 0;}
    private:
        /**
         * @brief 协程/函数/线程   组
         * @details 被调度的元素
        */
        struct FiberAndThread{
            ///协程
            Fiber::ptr fiber;
            ///函数
            std::function<void()> cb;
            ///线程组
            int thread;

            FiberAndThread(Fiber::ptr f, int thr)
                : fiber(f), thread(thr){}
            FiberAndThread(Fiber::ptr *f, int thr)
                : fiber(*f), thread(thr){}
            FiberAndThread(std::function<void()> f, int thr)
                : cb(f), thread(thr){}
            FiberAndThread(std::function<void()>* f, int thr)
                :thread(thr) {
                    cb.swap(*f);
                }
            /**
            * @brief 无参构造函数
            */
            FiberAndThread()
                :thread(-1) {
            }
            /**
             * @brief 重置
            */
            void reset()
            {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };
    private:
        template<class FiberOrCb>
        bool schedulerNoLock(FiberOrCb fc,int thread){
            bool need_tickle = m_fibers.empty();
            FiberOrCb(fc, thread);
            if(ft.fiber || ft.cb){
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

    private:
        MutexType m_mutex;
        std::string m_name;
        std::vector<Thread::ptr> m_threads;
        //待执行的协程队列
        std::list<FiberAndThread> m_fibers;
        Fiber::ptr m_rootFiber;

    protected:
        /// 线程id数组
        std::vector<int> m_threadIds;
        /// 线程数量
        size_t m_threadCount = 0;
        /// 工作线程数量
        std::atomic<size_t> m_activeThreadCount = {0};
        /// 空闲线程数量
        std::atomic<size_t> m_idleThreadCount = {0};
        /// 是否正在停止
        bool m_stopping = true;
        /// 是否自动停止
        bool m_autoStop = false;
        /// 主线程id(use_caller)
        int m_rootThread = 0;
    };
}

#endif