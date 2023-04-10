#ifndef __SYLAR_THREAD_H_
#define __SYLAR_THREAD_H_

#include <thread>
#include <functional>
#include <memory>
#include <string>
#include "noncopyable.h"
#include "mutex.h"
namespace sylar{
    class Thread : public NonCopyAble
    {
    public:
        typedef std::shared_ptr<Thread> ptr;
        Thread(std::function<void()> cb, const std::string &name);
        ~Thread();

        const std::string getName() const { return m_name; }
        const pid_t getID() const { return m_id; }
        static void SetName(const std::string& name);
        static const std::string GetName();

        void join();
        static Thread *GetThis();
        static void *run(void *args);


    private:
        pid_t m_id = -1;
        pthread_t m_thread = 0;
        std::function<void()> m_cb;
        std::string m_name;
        Semaphore m_semaphore;
    };
}

#endif