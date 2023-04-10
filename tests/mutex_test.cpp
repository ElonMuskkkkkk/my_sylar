#include "../sylar/inc/mutex.h"
#include <iostream>

int main()
{
    typedef sylar::Spinlock MutexType;
    MutexType m_mutex;
    {
        MutexType::Lock lock(m_mutex);
        std::cout << "hello world" << std::endl;
    }
    {
        MutexType::Lock lock(m_mutex);
        std::cout << "***********" << std::endl;
    }
}