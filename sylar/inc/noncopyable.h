#ifndef __SYLAR_NONCOPYABLE_H_
#define __SYLAR_NONCOPYABLE_H_

namespace sylar{
    /**
     * @brief 对象无法拷贝赋值
    */
    class NonCopyAble{
    public:
        /**
         * @brief 默认构造函数
        */
        NonCopyAble() = default;
        /**
         * @brief 默认析构函数
        */
        ~NonCopyAble() = default;
        /**
         * @brief 禁用拷贝构造函数
        */
        NonCopyAble(const NonCopyAble &) = delete;
        /**
         * @brief 禁用赋值运算符
        */
        NonCopyAble(const NonCopyAble &&) = delete;

    };
}

#endif