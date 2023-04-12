#include "../inc/util.h"
#include "../inc/log.h"

namespace sylar{
    static Logger::ptr g_logger = SYLAR_LOG_NAME("system");
    pid_t GetThreadID()
    {
        return syscall(SYS_gettid);
    }
    uint32_t GetFiberID()
    {
        return sylar::Fiber::GetFiberID();
    }
    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        void **array = (void **)malloc(sizeof(void *) * size);
        size_t s = backtrace(array, size);
        char **strings = backtrace_symbols(array, s);
        if(strings == nullptr){
            SYLAR_LOG_ERROR(g_logger) << "backtrace synbols error";
            return;
        }
        for (size_t i = skip; i < s;i++){
            bt.push_back(strings[i]);
        }
        free(array);
        free(strings);
    }

    std::string BacktraceToString(int size, int skip, const std::string &prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); i++)
        {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();    
    }
}