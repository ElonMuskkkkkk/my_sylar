#ifndef __SYLAR_HOOK_H_
#define __SYLAR_HOOK_H_

#include <time.h>
#include <unistd.h>
#include <dlfcn.h>


namespace sylar{

    //该线程hook是否可用
    bool is_hook_enable();
    //设置当前线程的hook状态
    void set_hook_enable(bool flag);
}

extern "C" {
    //sleep
    typedef unsigned int (*sleep_fun)(unsigned int seconds);
    extern sleep_fun sleep_f;
    
    typedef int (*usleep_fun)(useconds_t usec);
    extern usleep_fun usleep_f;
}

#endif