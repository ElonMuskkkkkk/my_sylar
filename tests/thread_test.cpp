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
    while(true){
        SYLAR_LOG_INFO(g_logger) << "*********************************************";
    }
}
void func3(){
    while(true){
        SYLAR_LOG_INFO(g_logger) << "=============================================";
    }
}

int main()
{
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("../bin/config/test.yml");
    sylar::Config::LoadFromYaml(root);
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 1; i++){
        //sylar::Thread::ptr thr1(new sylar::Thread(&func1, "name2_" + std::to_string(i)));
        sylar::Thread::ptr thr2(new sylar::Thread(&func2, "name2_" + std::to_string(i)));
        //sylar::Thread::ptr thr3(new sylar::Thread(&func3, "name3_" + std::to_string(i)));
        //thrs.push_back(thr1);
        thrs.push_back(thr2);
        //thrs.push_back(thr3);
    }
    for (size_t i = 0; i < thrs.size(); i++){
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "ok!";
}