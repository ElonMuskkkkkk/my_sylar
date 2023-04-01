/**
 * @file util.h
 * @brief 常用的工具函数
*/

#ifndef __UTIL_H_
#define __UTIL_H_

#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdint.h>
#include <cxxabi.h>

namespace sylar{
    pid_t GetThreadID();
    uint32_t GetFiberID();

    template <typename T>
    const char* TypeToName(){
        static const char *s_name = abi::__cxa_demangle(typeid(T).name(), 0, 0, 0);
        return s_name;
    }
}

#endif