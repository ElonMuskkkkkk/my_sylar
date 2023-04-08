#include "../inc/thread.h"
#include "../inc/log.h"
#include "../inc/util.h"
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace sylar
{
    static thread_local Thread *t_thread = nullptr;
    static thread_local std::string t_thread_name = "UNKNOWN";
    Thread *Thread::GetThis()
    {
        return t_thread;
    }
    const std::string Thread::GetName()
    {
        return t_thread_name;
    }
    void Thread::SetName(const std::string& name)
    {
        if(t_thread)
            t_thread->m_name = name;
        t_thread_name = name;
    }

    Thread::Thread(std::function<void()> cb, const std::string &name)
    : m_cb(cb),
      m_name(name)
    {
        if(name.empty()){
            m_name = "UNKNOWN";
        }
        int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
        if(rt){
            SYLAR_LOG_ERROR(g_logger) << "pthread_create thread faile "
                                      << "rt= "
                                      << rt << " name = " << name;
            throw std::logic_error("pthread create error");
        }
    }
    Thread::~Thread()
    {
        if(m_thread){
            pthread_detach(m_thread);
        }
    }

    void Thread::join()
    {
        if(m_thread){
            int rt = pthread_join(m_thread,nullptr);
            if(rt){
                SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }
    void* Thread::run(void* args)
    {
        Thread *thread = (Thread *)args;
        t_thread = thread;
        t_thread_name = thread->m_name;
        thread->m_id = sylar::GetThreadID();
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

        std::function<void()> cb;
        cb.swap(thread->m_cb);
        cb();
        return 0;
    }
}