/**
 * @brief 单例模式封装器
*/
#ifndef __SYLAR_SINGLETON_H_
#define __SYLAR_SINGLETON_H_

#include <memory>
#include <iostream>

namespace sylar{
    
    namespace{
        template <typename T, typename X, int N>
        T& GetInstance(){
            static T v;
            return v;
        }

        template <typename T, typename X, int N>
        std::shared_ptr<T> GetInstancePtr(){
            static std::shared_ptr<T> v(new T);
            return v;
        }
    }
    
    /**
     * @brief 单例模式封装类
     * @details T 类型
     *          X 为了创造多个实例对应的tag
     *          N 同一个tag创造多个实例索引
    */
    template <typename T, typename X = void, int N = 0>
    class Singleton{
    public:
        /**
         * @brief 返回单例裸指针
        */
        static T* GetInstance(){
            // std::cout << "hello world" << std::endl;
            static T v;
            return &v;
        }
    };

    /**
     * @brief 单例模式智能指针封装类
     * @details T 类型
     *          X 为了创造多个实例对应的tag
     *          N 同一个Tag创造多个实例索引
    */
    template <typename T, typename X = void, int N = 0>
    class SingletonPtr{
    public:
        /**
         * @brief 返回单例智能指针
        */
        static std::shared_ptr<T> GetInstance()
        {
            static std::shared_ptr<T> v(new T);
            return v;
        }
    };
}

#endif 