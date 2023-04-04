#include "../sylar/inc/conf.h"
#include "../sylar/inc/log.h"
#include <yaml-cpp/yaml.h>

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::Config::Lookup("system.port", int(8080), "system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::Config::Lookup("system.value", (float)10, "system value");

sylar::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config =
    sylar::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int vec");

sylar::ConfigVar<std::list<int> >::ptr g_int_list_value_config =
    sylar::Config::Lookup("system.int_list", std::list<int>{5,6}, "system int list");

sylar::ConfigVar<std::set<int> >::ptr g_int_set_value_config =
    sylar::Config::Lookup("system.int_set", std::set<int>{1,2}, "system int set");

sylar::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config =
    sylar::Config::Lookup("system.int_uset", std::unordered_set<int>{1,2}, "system int uset");

sylar::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config =
    sylar::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k",2}}, "system str int map");

void print_yaml(const YAML::Node &node, int level)
{
    if (node.IsScalar())
    {
        std::cout << std::string(level * 4, ' ')
                  << node.Scalar() << " - " << node.Type() << " - " << level
                  << std::endl;
    }
    else if (node.IsNull())
    {
        std::cout << std::string(level * 4, ' ')
                  << "Null - " << node.Type() << " - " << level
                  << std::endl;
    }
    else if (node.IsMap())
    {
        for (auto it = node.begin(); it != node.end(); it++)
        {
            std::cout << std::string(level * 4, ' ')
                      << it->first << " - " << it->second.Type() << " - " << level
                      << std::endl;
            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence())
    {
        for (size_t i = 0; i < node.size(); ++i)
        {
            std::cout << std::string(level * 4, ' ')
                      << i << " - " << node[i].Type() << " - " << level
                      << std::endl;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml()
{
    YAML::Node root = YAML::LoadFile("../bin/config/log.yml");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root["test"].IsDefined();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root["logs"].IsDefined();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}

void test_config()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->toString();
#define XX(g_var, name, prefix)                                                               \
    {                                                                                         \
        auto v = g_var->getValue();                                                           \
        for (auto &i : v)                                                                     \
        {                                                                                     \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " : " << i;                 \
        }                                                                                     \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " :yaml " << g_var->toString(); \
    }
#define XX_M(g_var, name, prefix)                                                             \
    {                                                                                         \
        auto v = g_var->getValue();                                                           \
        for (auto &i : v)                                                                     \
        {                                                                                     \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": {"                       \
                                             << i.first << " - " << i.second << "}";          \
        }                                                                                     \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }
    XX(g_int_vec_value_config, "before", "int_vec");
    XX(g_int_list_value_config, "before", "int_list");
    XX(g_int_set_value_config, "before", "int_set");
    XX(g_int_uset_value_config, "before", "int_uset");
    // XX_M(g_str_int_map_value_config, before, str_int_map_value);
    XX_M(g_str_int_umap_value_config, before, str_int_umap_value);

    YAML::Node root = YAML::LoadFile("../bin/config/log.yml");
    sylar::Config::LoadFromYaml(root);
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();
    XX(g_int_vec_value_config, "after", "int_vec");
    XX(g_int_list_value_config, "after", "int_list");
    XX(g_int_set_value_config, "after", "int_set");
    XX_M(g_str_int_umap_value_config, after, str_int_umap_value);

}

class Person {
public:
    Person() {};
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};

namespace sylar{
template<>
class LexicalCast<std::string, Person>{
public:
    Person operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};
template<>
class LexicalCast<Person, std::string>{
public:
    std::string operator()(const Person& p)
    {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
}
void test_class()
{
    sylar::ConfigVar<Person>::ptr g_person =
        sylar::Config::Lookup("class.person", Person(), "system person");

    sylar::ConfigVar<std::unordered_map<std::string, Person>>::ptr g_person_map =
        sylar::Config::Lookup("class.map", std::unordered_map<std::string, Person>(), "system person");

    sylar::ConfigVar<std::unordered_map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
        sylar::Config::Lookup("class.vec_map", std::unordered_map<std::string, std::vector<Person> >(), "system person");
    g_person->addListener([](const Person& old_value, const Person& new_value){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << old_value.toString()
                << " new_value=" << new_value.toString();
    });
    YAML::Node root = YAML::LoadFile("../bin/config/log.yml");
    sylar::Config::LoadFromYaml(root);
#define XX_PM(g_var, prefix)                                                  \
    {                                                                         \
        auto m = g_var->getValue();                                           \
        for (auto &i : m)                                                     \
        {                                                                     \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << ": " << i.first    \
                                             << " - " << i.second.toString(); \
        }                                                                     \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << ": size=" << m.size(); \
    }

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();
    
    XX_PM(g_person_map, before);
}

int main()
{
    //SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getName();
    //SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    //SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getDescription();
    // test_yaml();
    //YAML::Node root = YAML::LoadFile("../bin/config/log.yml");
    //print_yaml(root, 0);

    //test_config();
    test_class();
}
