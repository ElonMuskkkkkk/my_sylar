#include "../inc/util.h"


namespace sylar{
    pid_t GetThreadID()
    {
        return syscall(SYS_gettid);
    }
    uint32_t GetFiberID()
    {
        //待完善
        return 2;
    }
}