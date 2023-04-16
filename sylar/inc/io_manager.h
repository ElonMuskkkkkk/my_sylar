#ifndef __SYLAR_IOMANAGER_H_
#define __SYLAR_IOMANAGER_H_

#include "scheduler.h"


namespace sylar{

    /**
     * @brief 基于epoll的IO调度协程
    */
    class IOManager : public Scheduler
    {
    public:
        typedef std::shared_ptr<IOManager> ptr;
        typedef RWMutex RWMutexType;

        /**
         * @brief IO事件
        */
        enum Event
        {
            // 无事件
            NONE = 0x0,
            // 读事件
            READ = 0x1,
            // 写事件
            WRITE = 0x4
        };

    private:
        /**
         * @brief Socket事件上下文类
        */
        struct FdContext
        {
            /**
             * @brief 事件上下文类
            */
            typedef Mutex MutexType;
            struct EventContext
            {
                Scheduler *scd = nullptr;
                Fiber::ptr fiber = nullptr;
                std::function<void()> cb = nullptr;
            };
            /**
             * @brief 获取对应的事件上下文类
             * @param[in] event 事件类型
            */
            EventContext& getContext(Event event);
            /**
             * @brief 重置事件上下文
             * @param[in, out] ctx 待重置的上下文
            */
            void resetContext(EventContext &ctx);
            /**
             * @brief 触发事件
             * @param[in] event 事件类型
            */
            void triggerEvent(Event event);
            // 与事件关联的句柄
            int fd = 0;
            // 读事件
            EventContext read;
            // 写事件
            EventContext write;

            MutexType mutex;

            Event events = NONE;
        };
    public:
        /**
         * @brief 构造函数
         * @param[in] threads 线程数量
         * @param[in] use_caller 是否使用当前线程
         * @param[in] name 调度器名称
        */
        IOManager(size_t thread = 1, bool use_caller = true, const std::string &name = "");

        /**
         * @brief 析构函数
        */
        ~IOManager();

        /**
         * @brief 添加事件
         * @param[in] fd 文件句柄
         * @param[in] event 事件类型
         * @param[in] cb 回调函数
         * @return 成功返回0，否则-1
        */
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

        /**
         * @brief 删除事件
         * @param[in] fd 文件句柄
         * @param[in] event 事件类型
         * @attention 不会触发事件
        */
        bool delEvent(int fd, Event event);

        /**
         * @brief 取消事件
         * @param[in] fd 文件句柄
         * @param[in] event 事件类型
         * @attention 若事件存在会触发事件
        */
        bool cancelEvent(int fd, Event event);
        /**
         * @brief 取消所有事件
         * @param[in] fd socket句柄
         */
        bool cancelAll(int fd);

        /**
         * @brief 返回当前的IOManager
         */
        static IOManager* GetThis();
    protected:
        void tickle() override;
        void idle() override;
        bool stopping() override;
        //void onTimerInsertedAtFront() override;

        /**
         * @brief 重置socket句柄上下文的容器大小
         * @param[in] size 容量大小
         */
        void contextResize(size_t size);

        // /**
        //  * @brief 判断是否可以停止
        //  * @param[out] timeout 最近要出发的定时器事件间隔
        //  * @return 返回是否可以停止
        //  */
        // bool stopping(uint64_t& timeout);
    private:
        /// epoll 文件句柄
        int m_efd = 0;
        /// pipe 文件句柄
        int m_tickleFds[2];
        /// 当前等待执行的事件数量
        std::atomic<size_t> m_pendingEventCount = {0};
        /// IOManager的Mutex
        RWMutexType m_mutex;
        /// socket事件上下文的容器
        std::vector<FdContext*> m_fdContexts;
    };
}

#endif