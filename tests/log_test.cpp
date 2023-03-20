#include "../sylar/inc/log.h"


int main()
{
    std::cout << "hello sylar!" << std::endl;
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));
    auto level = sylar::LogLevel::DEBUG;
    sylar::LogEvent::ptr event(new sylar::LogEvent(logger, level, __FILE__, __LINE__, 0, 1, 2, time(0)));
    logger->log(level, event);
    return 0;
}