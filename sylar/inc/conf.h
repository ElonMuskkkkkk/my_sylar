#ifndef __SYLAR_CONFIG_H_
#define __SYLAR_CONFIG_H_

#include <memory>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <string>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <exception>
#include "log.h"
#include "util.h"
#include "cxxabi.h"

namespace sylar
{
    class ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        /**
         * @brief 构造函数
         * @param[in] name,配置参数名称[0-9a-z_.]
         * @param[in] description,配置参数描述
         */
        ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name),
              m_description(description)
        {
            std::transform(m_name.cbegin(), m_name.cend(), m_name.begin(), ::tolower);
        }

        /**
         * @brief 析构函数
         */
        virtual ~ConfigVarBase() {}

        /**
         * @brief 返回配置参数名称
         */
        const std::string getName() { return m_name; }
        /**
         * @brief 返回配置参数描述
         */
        const std::string getDescription() { return m_description; }
        /**
         * @brief 转成字符串
         */
        virtual std::string toString() = 0;
        /**
         * @brief 从字符串初始化值
         */
        virtual bool fromString(const std::string &val) = 0;
        /**
         * @brief 返回配置参数的类型名称
         */
        virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;
        std::string m_description;
    };
    /**
     * @brief 类型转换模板，（F原类型，T目标类型）
     */
    template <typename F, typename T>
    class LexicalCast
    {
    public:
        /**
         * @brief 类型转换
         * @param[in] v 原值类型
         * @return 返回转换后的类型
         * @exception 抛出异常
         */
        T operator()(const F &v)
        {
            return boost::lexical_cast<T>(v);
        }
    };
    /**
     * @brief 类型模板类偏特化（YAML STRING 转换成 std::vector<T>)
     */
    template <typename T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };
    /**
     * @brief 类型转换模板偏特化（std::vector<T> 转换成 YAML STRING)
     */
    template <typename T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    /**
     * @brief 将模板类型偏特化（YAML STRING 转 std::list<T>)
     */
    template <typename T>
    class LexicalCast<std::string,std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> lis;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                lis.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return lis;
        }
    };
    /**
     * @brief 将模板类型偏特化（std::list<T> 转换成 YAML STRING)
     */
    template <typename T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    /**
     * @brief 模板偏特化 （将YAML STRING 转换成 std::set<T>)
     */
    template <typename T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v); // 将Yaml风格的字符串，格式化为项
            typename std::set<T> Set;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                Set.insert(LexicalCast<std::string, T>()(ss.str())); // 将每一项的内容放入容器当中
            }
            return Set;
        }
    };
    /**
     * @brief 模板偏特化（将std::set<T> 转换成 YAML STRING）
     */
    template <typename T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i : v)
            {
                // 先将每一项转为string，放入node中，处理为YAML风格的数据
                node.push_back(LexicalCast<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    /**
     * @brief 模板偏特化（YAML STRING 转换为 std::unordered_set<T>)
     */
    template <typename T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> uset;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                uset.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return uset;
        }
    };
    /**
     * @brief 模板偏特化 （unordered_set<T> 转换成 YAML STRING)
     */
    template <typename T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(std::unordered_set<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i : v)
            {
                node.push_back(LexicalCast<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    /**
     * @brief 模板偏特化（YAML STRING 转向 std::unordered_map<std::string,T>)
     */
    template <typename T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> umap;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++)
            {
                ss.str("");
                ss << it->second;
                umap.insert(std::make_pair(it->first.Scalar(),
                                           LexicalCast<std::string, T>()(ss.str())));
            }
            return umap;
        }
    };
    /**
     * @brief 模板偏特化 （unordered_map<std::string,T> 转换成 YAML STRING)
     */
    template <typename T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(std::unordered_map<std::string, T> &v)
        {
            YAML::Node node(YAML::NodeType::Map);
            for (auto &i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 配置模板参数子类，保存对应的参数值
     * @details T是具体的参数类型
     *          FromStr 从 std::string 转换成 T 类型的仿函数
     *          ToStr 从 T 类型转成 转换成 std::string 的仿函数
     *          std::string 为YAML格式的字符串
     */
    template <typename T, typename FromStr = LexicalCast<std::string, T>,
              typename ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;

        /**
         * @brief 通过参数名、参数值，以及描述，来构造ConfigVar
         * @param[in] name  参数名称有效字符为[0-9a-z_.]
         * @param[in] default_value 参数的默认值
         * @param[in] description   参数的描述
         */
        ConfigVar(const std::string &name,
                  const T &default_value,
                  const std::string &description = "")
            : ConfigVarBase(name, description),
              m_val(default_value) {}
        /**
         * @brief 将参数值转换成 YAML STRING
         * @exception 当转换失败时，抛出异常
        */
        std::string toString() override
        {
            try{
                return ToStr()(m_val);
            }
            catch(std::exception& e){
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception "
                                                  << e.what() << " convert: " << TypeToName<T>() << " to string"
                                                  << " name=" << m_name;
            }
            return "";
        }
        /**
         * @brief 从YAML STRING 转成参数的值
         * @exception 失败后抛出异常
        */
        bool fromString(const std::string& val) override
        {
            try{
                setValue(FromStr()(val));
                return true;
            }
            catch(std::exception& e){
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception "
                                                  << e.what() << " convert: string to " << TypeToName<T>() << " name="
                                                  << m_name << " - " << val;
            }
            return false;
        }

        /**
         * @brief 获取当前参数的值
        */
        const T getValue(){
            return m_val;
        }

        /**
         * @brief 设置当前的参数值
         * @details 如果参数的值发生变化，则通知对应的回调函数
        */
        void setValue(const T& v)
        {
            {
                if(v == m_val)
                    return;
                for(auto& i : m_cbs){
                    i.second(m_val, v);
                }
            }
            m_val = v;
        }
        /**
         * @brief 返回参数值类型的名称（typeinfo)
        */
        std::string getTypeName() const override { return TypeToName<T>(); }

        /**
         * @brief 添加变化的回调函数
         * @return 返回该回调函数对应的唯一id，用于删除回调
        */
        uint64_t addListener(on_change_cb cb){
            static uint64_t s_fun_id = 0;
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        /**
         * @brief 删除回调函数
         * @param[in] key 回调函数的id
        */
        void delListener(uint64_t key)
        {
            m_cbs.erase(key);
        }

        /**
         * @brief 取回调函数
         * @param[in] key 回调函数的唯一id
         * @return 如果存在对应的回调函数，返回回调函数，否则，返回nullptr
         */
        on_change_cb getListener(uint64_t key)
        {
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        /**
         * @brief 清理所有回调函数
        */
        void cleanListener()
        {
            m_cbs.clear();
        }

    private:
        T m_val;
        //变更回调函数数组，
        std::map<uint64_t, on_change_cb> m_cbs;
    };

    /**
     * @brief Config是ConfigVar的管理类
     * @details 提供便捷方法创建/访问ConfigVar
    */
    class Config
    {
    public:
        typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

        /**
         * @brief 获取或创建对应参数名的配置参数
         * @param[in] name 配置参数名称
         * @param[in] default_value 参数默认值
         * @param[in] description 参数描述
         * @details     获取参数名为name的配置参数，如果存在直接返回
         *              如果不存在，创建参数配置并用default_value赋值
         * @return      返回对应的参数配置信息，如果参数名存在但类型不匹配，则返回nullptr
         * @exception   如果参数名包含非法字符[^0-9a-z_.]，抛出异常
        */
        template <typename T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name,
                                                 const T &default_value,
                                                 const std::string &description = "")
        {
            auto it = GetDatas().find(name);
            if(it != GetDatas().end())
            {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if(tmp){
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
                    return tmp;
                } else{
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists but type not " << TypeToName<T>() << " real_type=" << it->second->getTypeName()
                                                      << " " << it->second->toString();
                    return nullptr;
                }
            }
            if(name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        /**
         * @brief 查找配置参数
         * @param[in] name 配置参数名称
         * @return 返回配置参数名为name的配置
        */
        template <typename T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name)
        {
            auto it = GetDatas().find(name);
            if(it == GetDatas().end()){
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        /**
         * @brief 使用YAML::Node 初始化配置模块
        */
        static void LoadFromYaml(const YAML::Node &root);
        /**
         * @brief 加载path文件夹里的配置文件
         */
        static void LoadFromConfDir(const std::string &path, bool force = false);

        /**
         * @brief 查找配置参数，返回配置参数的基类
         * @param[in] name 配置参数名称
        */
        static ConfigVarBase::ptr LookupBase(const std::string &name);
        /**
         * @brief 遍历配置模块里所有的配置项
         * @param[in] cb 配置项回调函数
         */
        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

    private:
        /**
         * @brief 返回所有的配置项
        */
        static ConfigVarMap& GetDatas(){
            static ConfigVarMap s_datas;
            return s_datas;
        }
    };
}

#endif