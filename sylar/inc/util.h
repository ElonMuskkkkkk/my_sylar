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

namespace sylar{
    pid_t GetThreadID();
    uint32_t GetFiberID();
}

#endif