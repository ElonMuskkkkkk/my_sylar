#include "../inc/hook.h"
#include "../inc/fd_manager.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


namespace sylar{
    FdCtx::FdCtx(int fd)
        :m_isInit(false)
        ,m_isSocket(false)
        ,m_sysNonblock(false)
        ,m_userNonblock(false)
        ,m_isClosed(false)
        ,m_fd(fd)
        ,m_recvTimeout(-1)
        ,m_sendTimeout(-1) 
    {
        init();
    }

    FdCtx::~FdCtx(){}
    
    bool FdCtx::init()
    {
        if(isInit())
            return true;
        // 先重置
        m_sendTimeout = -1;
        m_recvTimeout = -1;
        // 先看一下fd的信息，查看失败，证明fd有问题，初始化失败，否则继续
        struct stat fd_stat;
        if(fstat(m_fd,&fd_stat) == -1){
            m_isInit = false;
            m_isSocket = false;
        }
        else{
            m_isInit = true;
            m_isSocket = S_ISSOCK(fd_stat.st_mode);
        }
        if(m_isSocket){ // 如果是socket文件，则设置非阻塞
            int flags = fcntl_f(m_fd, F_GETFL, 0);
            if(!(flags & O_NONBLOCK))
            {
                fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
            }
            m_sysNonblock = true;
        }
        else{
            m_sysNonblock = false;
        }

        m_userNonblock = false;
        m_isClosed = false;
        return m_isInit;
    }

    void FdCtx::setTimeOut(int type,uint64_t time)
    {
        if(type == SO_RCVTIMEO){
            m_recvTimeout = time;
        }
        else if(type == SO_SNDTIMEO){
            m_sendTimeout = time;
        }
    }

    uint64_t FdCtx::getTimeOut(int type) const
    {
        if(type == SO_RCVTIMEO)
        {
            return m_recvTimeout;
        }
        else if(type == SO_SNDTIMEO)
        {
            return m_sendTimeout;
        }
        return 0;
    }

        FdManager::FdManager()
    {
        m_datas.resize(64);
    }

    FdCtx::ptr FdManager::get(int fd,bool auto_create)
    {
        if(fd == -1)
            return nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_datas.size() <= fd)
        {
            if(auto_create == false){
                return nullptr;
            }
        }
        else{
            // 有该文件或者不自动创建时
            if(m_datas[fd] || !auto_create) {
                return m_datas[fd];
            }
        }
        lock.unlock();
        // 没有且自动创建时
        RWMutexType::WriteLock lock2(m_mutex);
        FdCtx::ptr ctx(new FdCtx(fd));
        if(fd >= (int)m_datas.size()) {
            m_datas.resize(fd * 1.5);
        }
        m_datas[fd] = ctx;
        return ctx;
    }

    
    void FdManager::del(int fd) {
        RWMutexType::WriteLock lock(m_mutex);
        if((int)m_datas.size() <= fd) {
            return;
        }
        m_datas[fd].reset();
    }

}