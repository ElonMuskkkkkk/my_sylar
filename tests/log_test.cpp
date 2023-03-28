#include "../sylar/inc/log.h"
#include "../sylar/inc/util.h"
#include <unistd.h>


int main()
{
    std::cout << "hello sylar!" << std::endl;
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));
    logger->addAppender(sylar::LogAppender::ptr(new sylar::FileoutLogAppender("./log.txt")));
    //auto level = sylar::LogLevel::DEBUG;
    SYLAR_LOG_INFO(logger) << "macro test";
    SYLAR_LOG_FMT_ERROR(logger, "test macro fmt %s", "aa");
    
    return 0;
}