#ifndef __SYLAR_MUTEX_H_
#define __SYLAR_MUTEX_H_

#include <thread>
#include <functional>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>
#include <list>
#include "noncopyable.h"

namespace sylar{
    /**
     * @brief 信号量
    */
    class Semaphore : NonCopyAble{
    public:
        /**
         * @brief 构造函数
         * @param[in] count 信号量大小
        */
        Semaphore(uint32_t count = 0);
        ~Semaphore();
        /**
         * @brief 获取信号量
        */
        void wait();
        /**
         * @brief 释放信号量
        */
        void notify();

    private:
        sem_t m_semaphore;
    };

    /**
     * @brief 局部锁的模板实现
    */
    template <typename T>
    struct ScopedLockImpl
    {
    public:
        /**
         * @brief 构造函数
         * @param[in] mutex
        */
        ScopedLockImpl(T& mutex)
        : m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked = true;
        }
        ~ScopedLockImpl(){
            unlock();
        }
        /**
         * @brief 上锁
        */
        void lock()
        {
            if(!m_locked){
                m_mutex.lock();
                m_locked = true;
            }
        }

        /**
         * @brief 解锁
        */
        void unlock()
        {
            if(m_locked){
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    /**
     * @brief 局部读锁模板实现
    */
    template<typename T>
    struct ReadScopedLockImpl
    {
    public:
        /**
         * @brief 构造函数
         * @param[in] mutex 读写锁
        */
        ReadScopedLockImpl(T& mutex)
        : m_mutex(mutex)
        {
            m_mutex.rdlock();
            m_locked = true;
        }
        /**
         * @brief 析构函数，RAII
        */
        ~ReadScopedLockImpl()
        {
            unlock();
        }
        /**
         * @brief 上读锁
        */
        void lock()
        {
            if(!m_locked){
                m_mutex.rdlock();
                m_locked = true;
            }
        }
        /**
         * @brief 释放锁
        */
        void unlock(){
            if(m_locked){
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };
    /**
     * @brief 局部写锁模板实现
    */
    template <typename T>
    struct WriteScopedLockImpl{
    public:
        /**
         * @brief 构造函数
         * @param[in] mutex 读写锁
        */
        WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex){
            m_mutex.wrlock();
            m_locked = true;
        }
        /**
         * @brief 析构函数
        */
        ~WriteScopedLockImpl()
        {
            unlock();
        }
        /**
         * @brief 上写锁
        */
        void lock()
        {
            if(!m_locked){
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if(m_locked){
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T& m_mutex;
        bool m_locked;
    };

    /**
     * @brief 互斥量
    */
    class Mutex : public NonCopyAble
    {
    public:
        typedef ScopedLockImpl<Mutex> Lock;
        /**
         * @brief 构造函数
        */
        Mutex(){
            pthread_mutex_init(&m_mutex, nullptr);
        }
        /**
         * @brief 析构函数
        */
        ~Mutex(){
            pthread_mutex_destroy(&m_mutex);
        }
        /**
         * @brief 上锁
        */
        void lock(){
            pthread_mutex_lock(&m_mutex);
        }
        /**
         * @brief 解锁
        */
        void unlock(){
            pthread_mutex_unlock(&m_mutex);
        }

    private:
        pthread_mutex_t m_mutex;
    };

    /**
     * @brief 空锁（用于调试）
    */
    class NullMutex : public NonCopyAble
    {
    public:
        typedef ScopedLockImpl<NullMutex> Lock;

        NullMutex(){}

        ~NullMutex(){}

        void lock(){}

        void unlock(){}

    };

    /**
     * @brief 读写互斥量
    */
    class RWMutex : public NonCopyAble
    {
    public:
        typedef WriteScopedLockImpl<RWMutex> WriteLock;
        typedef ReadScopedLockImpl<RWMutex> ReadLock;

        /**
         * @brief 构造函数
        */
        RWMutex(){
            pthread_rwlock_init(&m_lock, nullptr);
        }
        /**
         * @brief 析构函数
        */
        ~RWMutex(){
            pthread_rwlock_unlock(&m_lock);
        }

        /**
         * @brief 上读锁
        */
        void rdlock()
        {
            pthread_rwlock_rdlock(&m_lock);
        }
        /**
         * @brief 上写锁
        */
        void wrlock()
        {
            pthread_rwlock_wrlock(&m_lock);
        }
        /**
         * @brief 解锁
        */
        void unlock()
        {
            pthread_rwlock_unlock(&m_lock);
        }

    private:
        pthread_rwlock_t m_lock;
    };
    /**
     * @brief 自旋锁
    */
    class Spinlock : NonCopyAble
    {
    public:
        typedef ScopedLockImpl<Spinlock> Lock;

        /**
         * @brief 构造函数
         */
        Spinlock() {
            pthread_spin_init(&m_mutex, 0);
        }

        /**
         * @brief 析构函数
        */
        ~Spinlock() {
            pthread_spin_destroy(&m_mutex);
        }

        /**
         * @brief 上锁
        */
        void lock() {
            pthread_spin_lock(&m_mutex);
        }

        /**
         * @brief 解锁
        */
        void unlock() {
            pthread_spin_unlock(&m_mutex);
        }
    private:
        /// 自旋锁
        pthread_spinlock_t m_mutex;
    };

    /**
     * @brief CAS乐观锁
    */
    class CASLock : public NonCopyAble
    {
    public:
        typedef ScopedLockImpl<CASLock> Lock;

        CASLock(){
            m_mutex.clear();
        }
        ~CASLock(){}

        void lock()
        {
            while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
        }

        void unlock()
        {
            std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
        }

    private:
        volatile std::atomic_flag m_mutex;
    };
    /**
    class Scheduler;
    class FiberSemaphore : NonCopyAble
    {
    public:
        typedef Spinlock MutexType;

    private:
        MutexType m_mutex;
        std::list<std::pair<Scheduler*,
    };
    */
}

#endif