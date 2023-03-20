#include "../inc/log.h"
#include <map>
namespace sylar
{
    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        break;

            XX(DEBUG)
            XX(WARN)
            XX(INFO)
            XX(ERROR)
            XX(FATAL)
#undef XX
        default:
            return "UNKNOWN";
            break;
        }
        return "UNKNOWN";
    }

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file,
                       int32_t line, uint32_t elapes, uint32_t thread_id,
                       uint32_t fiber_id, time_t time)
        : m_logger(logger),
          m_level(level),
          m_log_file(file),
          m_line(line),
          m_elapse(elapes),
          m_threadID(thread_id),
          m_fiberID(fiber_id),
          m_time(time){}

    void Logger::addAppender(LogAppender::ptr appender)
    {
        if(!appender->getFormatter()){
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }
    void Logger::delAppender(LogAppender::ptr ptr)
    {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); it++)
        {
            if (*it == ptr)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    Logger::Logger(const std::string &name):m_name(name),m_level(LogLevel::DEBUG)
    {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    }
    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        auto self = shared_from_this();
        if (level >= 0)
        {
            for (auto &i : m_appenders)
            {
                i->log(self, level, event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::DEBUG, event);
    }
    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::INFO, event);
    }
    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::WARN, event);
    }
    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::ERROR, event);
    }
    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::FATAL, event);
    }

    FileoutLogAppender::FileoutLogAppender(const std::string name) : m_filename(name) {}

    void FileoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {

    }

    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
            std::cout << m_formatter->format(logger, level, event);
    }

    bool FileoutLogAppender::reopen()
    {
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        return !!m_filestream;
    }

    LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern) { init(); }
    std::string LogFormatter::format(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &i : m_items)
        {
            i->format(ss, logger, level, event);
        }
        return ss.str();
    }
    /**
     * @brief 多态，格式化日志内容
     */
    class MessageFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        MessageFormatterItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getMessage();
        }
    };
    /**
     * @brief 多态，格式化日志级别
     */
    class LevelFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        LevelFormatterItem(const std::string str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << LogLevel::ToString(level);
        }
    };
    /**
     * @brief 多态，格式化日志耗时
     */
    class ElapseFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        ElapseFormatterItem(const std::string str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };
    /**
     * @brief 多态，格式化日志名称
     */
    class NameFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        NameFormatterItem(const std::string str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLogger()->getName();
        }
    };
    /**
     * @brief 多态，格式化日志线程id
     */
    class ThreadIDFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        ThreadIDFormatterItem(const std::string str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getThreadID();
        }
    };
    /**
     * @brief 多态，格式化日志级别
     */
    class FiberIDFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        FiberIDFormatterItem(const std::string str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFiberID();
        }
    };
    /**
     * @brief 多态，格式化线程名称
     */
    class ThreadNameFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        ThreadNameFormatterItem(const std::string str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getThreadID();
        }
    };
    /**
     * @brief 多态，格式化日期
     */
    class DateTimeFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        DateTimeFormatterItem(const std::string fmt = "%Y-%m-%d %H:%M:%S")
            : m_fmt(fmt) {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            struct tm tm;
            time_t time = event->getTime();
            // 线程安全版本
            localtime_r(&time, &tm);
            char buf[64];
            // strftime C库函数，格式化时间格式
            strftime(buf, sizeof(buf), m_fmt.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_fmt;
    };
    /**
     * @brief 多态，格式化文件名称
     */
    class FileNameFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        FileNameFormatterItem(const std::string str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLogFile();
        }
    };
    /**
     * @brief 多态，格式化线程名称
     */
    class LineFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        LineFormatterItem(const std::string str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
    };
    /**
     * @brief 多态，格式化线程名称
     */
    class NewLineFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        NewLineFormatterItem(const std::string str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << std::endl;
        }
    };
    /**
     * @brief 多态，格式化线程名称
     */
    class StringFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        StringFormatterItem(const std::string str)
            : m_string(str) {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << m_string;
        }

    private:
        std::string m_string;
    };
    /**
     * @brief 多态，格式化线程名称
     */
    class TabFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        TabFormatterItem(const std::string str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << "\t";
        }
    };
    void LogFormatter::init()
    {
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        size_t n = m_pattern.size();
        for (size_t i = 0; i < n; i++)
        {
            // 解析正常的日志内容
            if (m_pattern[i] != '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }
            // 当出现%时，判断是不是%转义
            if (i + 1 < n && m_pattern[i + 1] == '%')
            {
                nstr.append(1, '%');
                continue;
            }
            // 若不是，证明出现了%xxx格式或者%xxx{xxx}格式的内容
            size_t next_pos = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;

            std::string str; // 记录xxx
            std::string fmt; // 记录{xxx}
            while (next_pos < n)
            {
                if (!fmt_status && !isalpha(m_pattern[next_pos]) && m_pattern[next_pos] != '{' && m_pattern[next_pos] != '}')
                {
                    str = m_pattern.substr(i + 1, next_pos - i - 1);
                    break;
                }
                if (fmt_status == 0)
                {
                    if (m_pattern[next_pos] == '{')
                    {
                        str = m_pattern.substr(i + 1, next_pos - i - 1);
                        fmt_status = 1;
                        fmt_begin = next_pos;
                        next_pos++;
                        continue;
                    }
                }
                else if (fmt_status == 1)
                {
                    if (m_pattern[next_pos] == '}')
                    {
                        fmt = m_pattern.substr(fmt_begin + 1, next_pos - fmt_begin - 1);
                        fmt_status = 0;
                        ++next_pos;
                        break;
                    }
                }
                ++next_pos;
                if (next_pos == n)
                {
                    if (str.empty())
                    {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }
            if (fmt_status == 0)
            {
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = next_pos - 1;
            }
            else if (fmt_status == 1)
            {
                std::cout << "pattern error: " << m_pattern << '-' << m_pattern.substr(i) << std::endl;
                vec.push_back(std::make_tuple("<<pattern error>>", fmt, 0));
                m_error = true;
            }
        }
        if (!nstr.empty())
        {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }
        /**
         * @brief 将不同功能对应的api存储起来
        */
        static std::map<std::string, std::function<LogFormatter::FormatterItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str,C)        \
        {#str, [](const std::string &fmt) { return LogFormatter::FormatterItem::ptr(new C(fmt));}}

            XX(m, MessageFormatterItem),   // m:消息
            XX(p, LevelFormatterItem),     // p:日志级别
            XX(r, ElapseFormatterItem),    // r:累计毫秒数
            XX(c, NameFormatterItem),      // c:日志名称
            XX(t, ThreadIDFormatterItem),  // t:线程id
            XX(n, NewLineFormatterItem),   // n:换行
            XX(d, DateTimeFormatterItem),  // d:时间
            XX(f, FileNameFormatterItem),  // f:文件名
            XX(l, LineFormatterItem),      // l:行号
            XX(T, TabFormatterItem),       // T:Tab
            XX(F, FiberIDFormatterItem),   // F:协程id
            XX(N, ThreadNameFormatterItem) // N:线程名称
#undef XX
        };
        for (auto& i : vec){
            if(std::get<2>(i) == 0){
                m_items.push_back(FormatterItem::ptr(new StringFormatterItem(std::get<0>(i))));
            }
            else {
                auto it = s_format_items.find(std::get<0>(i));
                if(it == s_format_items.end()){
                    m_error = true;
                    m_items.push_back(FormatterItem::ptr(new StringFormatterItem("<<error_format %" + std::get<0>(i) + ">>")));
                }
                else{
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
        }
    }
    
}
