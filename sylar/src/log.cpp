#include "../inc/log.h"
#include <yaml-cpp/yaml.h>
#include "../inc/conf.h"
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
    LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v)            \
    if (str == #v)              \
    {                           \
        return LogLevel::level; \
    }
        XX(DEBUG, debug);
        XX(INFO, info);
        XX(WARN, warn);
        XX(ERROR, error);
        XX(FATAL, fatal);

        XX(DEBUG, DEBUG);
        XX(INFO, INFO);
        XX(WARN, WARN);
        XX(ERROR, ERROR);
        XX(FATAL, FATAL);
        return LogLevel::UNKOWN;
    #undef XX
    }
    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file,
                       int32_t line, uint32_t elapes, uint32_t thread_id,
                       uint32_t fiber_id, time_t time, std::string threadName)
        : m_logger(logger),
          m_level(level),
          m_file(file),
          m_line(line),
          m_elapse(elapes),
          m_threadID(thread_id),
          m_fiberID(fiber_id),
          m_time(time),
          m_threadName(threadName) {}

    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if(len != -1){
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    LogEventWrap::LogEventWrap(LogEvent::ptr event) : m_event(event){}
    std::stringstream& LogEventWrap::getSS(){
        return m_event->getSS();
    }
    LogEventWrap::~LogEventWrap(){
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }
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

    void Logger::setFormatter(LogFormatter::ptr val) {
        // MutexType::Lock lock(m_mutex);
        m_formatter = val;

        for(auto& i : m_appenders) {
            // MutexType::Lock ll(i->m_mutex);
            if(!i->m_hasFormatter) {
                i->m_formatter = m_formatter;
            }
        }
    }

    void Logger::setFormatter(const std::string& val) {
        std::cout << "---" << val << std::endl;
        sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
        if(new_val->isError()) {
            std::cout << "Logger setFormatter name=" << m_name
                    << " value=" << val << " invalid formatter"
                    << std::endl;
            return;
        }
        //m_formatter = new_val;
        setFormatter(new_val);
    }

    std::string Logger::toYamlString()
    {
        YAML::Node node;
        node["name"] = m_name;
        if(m_level != LogLevel::UNKOWN){
            node["level"] = LogLevel::ToString(m_level);
        }
        if(m_formatter){
            node["formatter"] = m_formatter->getFormat();
        }
        for(auto& i : m_appenders){
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
    void LogAppender::setFormatter(LogFormatter::ptr format){
        m_formatter = format;
        if(m_formatter)
            m_hasFormatter = true;
        else
            m_hasFormatter = false;
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
        if(level >= m_level)
        {
            uint64_t now = event->getTime();
            if(now >= m_lastTime + 3){
                reopen();
                m_lastTime = now;
            }
            if(!m_formatter->format(m_filestream,logger,level,event)){
                std::cout << "error!" << std::endl;
            }
        }
    }

    std::string FileoutLogAppender::toYamlString(){
        YAML::Node node;
        node["type"] = "FileoutLogAppender";
        node["file"] = m_filename;
        if(m_level != LogLevel::UNKOWN){
            node["level"] = LogLevel::ToString(m_level);
        }
        if(m_hasFormatter && m_formatter){
            node["formatter"] = m_formatter->getFormat();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        // std::cout << m_level << " " << level << std::endl;
        if (level >= m_level)
            std::cout << m_formatter->format(logger, level, event) << std::endl;
    }

    std::string StdoutLogAppender::toYamlString() {
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        if(m_level != LogLevel::UNKOWN) {
            node["level"] = LogLevel::ToString(m_level);
        }
        if(m_hasFormatter && m_formatter) {
            node["formatter"] = m_formatter->getFormat();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
    void Logger::clearAppenders() {
        //MutexType::Lock lock(m_mutex);
        m_appenders.clear();
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
    std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        for(auto& i : m_items){
            i->format(ofs, logger, level, event);
        }
        return ofs;
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
            os << event->getThreadName();
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
            os << event->getFile();
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
    LoggerManager::LoggerManager(){
        m_root.reset(new Logger);
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
        m_loggers[m_root->getName()] = m_root;
        init();
    }
    /**
     * @brief 获取日志器，若没有，则注册一个
    */
    Logger::ptr LoggerManager::getLogger(const std::string& name)
    {
        auto it = m_loggers.find(name);
        if(it != m_loggers.end()){
            return it->second;
        }
        Logger::ptr logger(new Logger(name));
        m_loggers[name] = logger;
        return logger;
    }
    struct LogAppenderDefine{
        int type = 0;
        LogLevel::Level level = LogLevel::UNKOWN;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine& oth) const {
            return type == oth.type && level == oth.level && formatter == oth.formatter && file == oth.file;
        }
    };

    struct LogDefine{
        std::string name;
        LogLevel::Level level = LogLevel::UNKOWN;
        std::string formatter;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine& oth) const {
            return name == oth.name && level == oth.level && formatter == oth.formatter && appenders == oth.appenders;
        }

        bool operator<(const LogDefine& oth) const {
            return name < oth.name;
        }

        bool isValid() const {
            return !name.empty();
        }
    };
    template <>
    class LexicalCast<std::string,LogDefine>{
    public:
        LogDefine operator()(const std::string& v){
            YAML::Node n = YAML::Load(v);
            LogDefine ld;
            if(!n["name"].IsDefined()) {
                std::cout << "log config error: name is null, " << n
                        << std::endl;
                throw std::logic_error("log config name is null");
            }
            ld.name = n["name"].as<std::string>();
            ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
            if(n["formatter"].IsDefined()) {
                ld.formatter = n["formatter"].as<std::string>();
            }

            if(n["appenders"].IsDefined()) {
                //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
                for(size_t x = 0; x < n["appenders"].size(); ++x) {
                    auto a = n["appenders"][x];
                    if(!a["type"].IsDefined()) {
                        std::cout << "log config error: appender type is null, " << a
                                << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;
                    if(type == "FileLogAppender") {
                        lad.type = 1;
                        if(!a["file"].IsDefined()) {
                            std::cout << "log config error: fileappender file is null, " << a
                                << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        if(a["formatter"].IsDefined()) {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    } else if(type == "StdoutLogAppender") {
                        lad.type = 2;
                        if(a["formatter"].IsDefined()) {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    } else {
                        std::cout << "log config error: appender type is invalid, " << a
                                << std::endl;
                        continue;
                    }

                    ld.appenders.push_back(lad);
                }
            }
            return ld;
        }
    };

    template<>
    class LexicalCast<LogDefine, std::string> {
    public:
        std::string operator()(const LogDefine& i) {
            YAML::Node n;
            n["name"] = i.name;
            if(i.level != LogLevel::UNKOWN) {
                n["level"] = LogLevel::ToString(i.level);
            }
            if(!i.formatter.empty()) {
                n["formatter"] = i.formatter;
            }

            for(auto& a : i.appenders) {
                YAML::Node na;
                if(a.type == 1) {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                } else if(a.type == 2) {
                    na["type"] = "StdoutLogAppender";
                }
                if(a.level != LogLevel::UNKOWN) {
                    na["level"] = LogLevel::ToString(a.level);
                }

                if(!a.formatter.empty()) {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);
            }
            std::stringstream ss;
            ss << n;
            return ss.str();
        }
    };
    sylar::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
        sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    struct LogIniter {
        LogIniter() {
            g_log_defines->addListener([](const std::set<LogDefine>& old_value,
                        const std::set<LogDefine>& new_value){
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";
                for(auto& i : new_value) {
                    auto it = old_value.find(i);
                    sylar::Logger::ptr logger;
                    if(it == old_value.end()) {
                        //新增logger
                        logger = SYLAR_LOG_NAME(i.name);
                    } else {
                        if(!(i == *it)) {
                            //修改的logger
                            logger = SYLAR_LOG_NAME(i.name);
                        } else {
                            continue;
                        }
                    }
                    logger->setLevel(i.level);
                    //std::cout << "** " << i.name << " level=" << i.level
                    //<< "  " << logger << std::endl;
                    if(!i.formatter.empty()) {
                        // std::cout << "****" << std::endl;
                        logger->setFormatter(i.formatter);
                    }

                    logger->clearAppenders();
                    for(auto& a : i.appenders) {
                        sylar::LogAppender::ptr ap;
                        if(a.type == 1) {
                            ap.reset(new FileoutLogAppender(a.file));
                        } else if(a.type == 2) {
                            /*
                            if(!sylar::EnvMgr::GetInstance()->has("d")) {
                                ap.reset(new StdoutLogAppender);
                            } else {
                                continue;
                            }
                            */
                            ap.reset(new StdoutLogAppender);
                        }
                        ap->setLevel(a.level);
                        if(!a.formatter.empty()) {
                            LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                            if(!fmt->isError()) {
                                ap->setFormatter(fmt);
                            } else {
                                std::cout << "log.name=" << i.name << " appender type=" << a.type
                                        << " formatter=" << a.formatter << " is invalid" << std::endl;
                            }
                        }
                        logger->addAppender(ap);
                    }
                }

                for(auto& i : old_value) {
                    auto it = new_value.find(i);
                    if(it == new_value.end()) {
                        //删除logger
                        auto logger = SYLAR_LOG_NAME(i.name);
                        logger->setLevel((LogLevel::Level)0);
                        logger->clearAppenders();
                    }
                }
            });
        }
    };

    static LogIniter __log_init;
    
    std::string LoggerManager::toYamlString()
    {
        YAML::Node node;
        for(auto& i : m_loggers) {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
}
