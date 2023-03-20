#ifndef SYLAR_LOG_H_
#define SYLAR_LOG_H_

#include <string>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <functional>

namespace sylar {
    /**
     * @brief 定义日志级别【UNKOWN,DEBUG,INFO,WARN,ERROR,FATAL】
     */
    class Logger;
    class LogLevel
    {
    public:
        enum Level
        {
            UNKOWN = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };
        /**
         * @brief 将日志级别转换成字符串输出
        */
        static const char *ToString(LogLevel::Level level);
    };

    /**
     * @brief 日志数据(日志零件),记录了一条日志所需要的各种信息
    */
    class LogEvent{
    public:
        typedef std::shared_ptr<LogEvent> ptr;

        /**
         * @brief 构造函数
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] file 文件名
         * @param[in] line 文件行号
         * @param[in] elapse 程序启动依赖的耗时（毫秒）
         * @param[in] thread_id 线程id
         * @param[in] fiber_id 协程id
         * @param[in] time 日志时间（秒）
         * @param[in] thread_name 线程名称
        */

        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file,
                 int32_t line, uint32_t elapes, uint32_t thread_id,
                 uint32_t fiber_id, time_t time);
        /**
         * @brief 获取日志文件名称
         */
        const char *getLogFile() const { return m_log_file; }
        /**
         * @brief 获取日志行号
        */
        int32_t getLine() const { return m_line; }
        /**
         * @brief 获取耗时
        */
        uint32_t getElapse() const { return m_elapse; }
        /**
         * @brief 获取线程ID
        */
        uint32_t getThreadID() const { return m_threadID; }
        /**
         * @brief 获取协程ID
        */
        uint32_t getFiberID() const { return m_fiberID; }
        /**
         * @brief 获取耗时
        */
        uint32_t getTime() const { return m_time; }
        /**
         * @brief 获取日志内容
        */
        std::string getMessage() const {return m_content.str();}
        /**
         * @brief 获取日志器
        */
        std::shared_ptr<Logger> getLogger() const { return m_logger; }

    private:
        std::shared_ptr<Logger> m_logger; //日志器
        LogLevel::Level m_level;
        const char *m_log_file = nullptr;
        int32_t m_line = 0;         // 行号
        uint32_t m_elapse = 0;      //自服务器启动至今的事件
        uint32_t m_threadID = 0;    //线程号
        uint32_t m_fiberID = 0;     //协程号
        time_t m_time;              //时间戳
        std::stringstream m_content;      //日志内容
        std::string m_threadName;   //线程名称
    };

    /**
     * @brief 格式化日志
     * @brief init()函数：将日志内容格式化，%xxx, %xxx{xxx}, %%
    */
    class LogFormatter{
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
    /**
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details 
     * %m:消息
     * %p:日志级别
     * %r:累计时间（毫秒）
     * %c:日志名称
     * %t:线程id
     * %n:换行
     * %d:时间
     * %f:文件名
     * %l:行号
     * %T:Tab
     * %F:协程id
     * %N:线程名称
     * 
     * 默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
    */
        LogFormatter(const std::string &pattern);
        std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event);
        std::ostream &format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
        void init();

    public:
        /**
         * @brief 日志内容项格式化
        */
        class FormatterItem{
        public:
            typedef std::shared_ptr<FormatterItem> ptr;
            virtual ~FormatterItem(){}
        /**
         * @brief 日志内容格式化到流
         * @param[out] os
         * @param[in]  logger   日志器
         * @param[in]  LogEvent 日志数据
         * @param[in]  LogLevel 日志等级
        */
            virtual void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) = 0;
        };
    private:
        std::string m_pattern;
        std::vector<FormatterItem::ptr> m_items;
        bool m_error;
    };

    /**
     * @brief 日志添加，worker
     */
    class LogAppender
    {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender() {}
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
        LogFormatter::ptr getFormatter() const { return m_formatter; }

    protected:
        LogLevel::Level m_level;
        LogFormatter::ptr m_formatter;
    };


    /**
     * @brief 日志器,管理者的作用
    */
    class Logger : public std::enable_shared_from_this<Logger>
    {
    public:
        typedef std::shared_ptr<Logger> ptr;
        Logger(const std::string &name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr ptr);
        void delAppender(LogAppender::ptr ptr);
        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

        std::string getName() const { return m_name; }

    private:
        std::string m_name;                     //日志名称
        LogLevel::Level m_level;                //日志级别
        std::list<LogAppender::ptr> m_appenders; //日志输出地的集合
        LogFormatter::ptr m_formatter;
    };
    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        virtual void log(Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override;

    };
    class FileoutLogAppender : public LogAppender{
    public:
        typedef std::shared_ptr<FileoutLogAppender> ptr;
        FileoutLogAppender(const std::string name);
        virtual void log(Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override;
        bool reopen();

    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };
}

#endif
