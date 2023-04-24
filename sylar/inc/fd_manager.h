#ifndef __SYLAR_FD_MANAGER_
#define __SYLAR_FD_MANAGER_


#include <memory>
#include <vector>
#include "thread.h"
#include "singleton.h"

namespace sylar{
    /**
     * @brief 文件句柄上下文类
     * @details 管理文件句柄类型（是否socket）、是否阻塞、是否关闭、读写超时等
    */
    class FdCtx : public std::enable_shared_from_this<FdCtx>
    {
    public:
        typedef std::shared_ptr<FdCtx> ptr;
        /// @brief 构造函数
        /// @param fd 文件句柄
        FdCtx(int fd);
        /// @brief 析构
        ~FdCtx();
        /// @brief 判断文件管理器是否初始化
        bool isInit() const { return m_isSocket; };
        /// @brief 判断是否管理的是socket文件句柄
        bool isSocket() const { return m_isSocket; };
        /// @brief 判断文件是否关闭
        bool isClose() const { return m_isClosed; };
        /// @brief 设置用户主动非阻塞
        void setUserNonBlock(bool v) { m_userNonblock = v; };
        bool getUserNonBlock() const { return m_userNonblock; };
        /// @brief 设置系统主动非阻塞
        void setSysNonBlock(bool v) { m_sysNonblock = v; };
        bool getSysNonBlock() const { return m_sysNonblock; };
        /// @brief 设置超时时间
        /// @param type SO_RCVTIMEO(读超时)，SO_SNDTIMEO(写超时)
        /// @param v 超时时间毫秒
        void setTimeOut(int type, uint64_t v);
        uint64_t getTimeOut(int type) const;
    
    private:
        bool init();

    private:
        /// 是否初始化
        bool m_isInit: 1;
        /// 是否socket
        bool m_isSocket: 1;
        /// 是否hook非阻塞
        bool m_sysNonblock: 1;
        /// 是否用户主动设置非阻塞
        bool m_userNonblock: 1;
        /// 是否关闭
        bool m_isClosed: 1;
        /// 文件句柄
        int m_fd;
        /// 读超时时间毫秒
        uint64_t m_recvTimeout;
        /// 写超时时间毫秒
        uint64_t m_sendTimeout;
    };

    class FdManager {
    public:
        typedef RWMutex RWMutexType;
        /**
         * @brief 无参构造函数
         */
        FdManager();

        /**
         * @brief 获取/创建文件句柄类FdCtx
         * @param[in] fd 文件句柄
         * @param[in] auto_create 是否自动创建
         * @return 返回对应文件句柄类FdCtx::ptr
         */
        FdCtx::ptr get(int fd, bool auto_create = false);

        /**
         * @brief 删除文件句柄类
         * @param[in] fd 文件句柄
         */
        void del(int fd);
    private:
        /// 读写锁
        RWMutexType m_mutex;
        /// 文件句柄集合
        std::vector<FdCtx::ptr> m_datas;
    };

    /// 文件句柄单例
    typedef Singleton<FdManager> FdMgr;
}

#endif