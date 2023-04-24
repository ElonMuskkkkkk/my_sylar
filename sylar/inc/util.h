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
#include <vector>
#include <string>
#include <execinfo.h>
#include <sys/time.h>
#include "fiber.h"
#include <stdint.h>

namespace sylar{
    pid_t GetThreadID();
    uint32_t GetFiberID();
    /**
     * @brief 获取当前的调用栈
     * @param[out] bt 保存的调用栈
     * @param[in] size 最多返回层数
     * @param[in] skip 跳过栈顶的层数
    */
    void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);
    /**
     * @brief 获取当前栈信息的字符串
     * @param[in] size 栈顶的最大层数
     * @param[in] skip 跳过栈顶的层数
     * @param[in] prefix 栈信息前输出的内容
    */
    std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");

    template <typename T>
    const char *TypeToName()
    {
        static const char *s_name = abi::__cxa_demangle(typeid(T).name(), 0, 0, 0);
        return s_name;
    }

    //获取 ms
    uint64_t GetCurrentMS();
}

#endif