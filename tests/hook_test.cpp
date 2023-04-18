#include "../sylar/inc/sylar.h"
#include "../sylar/inc/hook.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_sleep(){
    sylar::IOManager iom(1);
    iom.schedule([]()
                 { sleep(2);
                 SYLAR_LOG_INFO(g_logger) << "sleep 2"; });
    iom.schedule([]()
                 { sleep(3);
                 SYLAR_LOG_INFO(g_logger) << "sleep 3"; });
}

int main()
{
    test_sleep();
    return 0;
}