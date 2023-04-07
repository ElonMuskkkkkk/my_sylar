#include "../sylar/inc/sylar.h"
#include <vector>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void func1(){
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                             << " this.name: " << sylar::Thread::GetThis()->getName()
                             << " id: " << sylar::GetThreadID()
                             << " this.id: " << sylar::Thread::GetThis()->getID();
}
void func2(){

}

int main()
{
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 5; i++){
        sylar::Thread::ptr thr(new sylar::Thread(&func1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }
    for (int i = 0; i < 5; i++){
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "ok!";
}