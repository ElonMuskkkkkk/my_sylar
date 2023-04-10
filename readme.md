# 目录说明
sylar -- 源码
tests -- 测试

# 功能模块

## 日志模块

代码量：1000+<br>
log4j风格

               
基本数据对象<br>
   LogEvent(日志数据)
   LogLevel(日志级别)


Logger类（日志器）<br>
1、开放给用户的写入接口，提供log方法，用户需要的参数为：LogLevel与LogEvent<br>
2、logger在添加appender时，会将设置appender的formatter对象（这里也实现了LogAppender的多态）<br>
3、用户将日志信息提供给log方法后，Logger会将数据传递给LogAppender(日志输出器)进行输出<br>

LogAppender<br>
1.日志输出地，通过继承的方式实现日志输出方式的多态（标准输出流、文件输出）
2.调用LogFormatter对象来进行具体的日志输出<br>

LogFormatter<br>
1、负责执行具体的写入操作：调用格式化函数format来输出日志内容<br>
2、LogFormatter初始化时要指定pattern信息，将其解析，对应的"格式化接口"存放到m_items当中<br>
3、format方法输出日志内容 ——> 具体方式为：遍历m_items中的项，调用项对应的接口()<br>
4、当所有的items被遍历之后，则日志流完成，在LogAppender中输出<br>

LogEventWrap<br>
1.日志包装器，减少使用日志时的代码量
2.采用RAII技术进行写入操作

## 配置模块

兼容内置类型、stl容器、自定义类型

代码量：800左右

ConfigVarBase:配置参数基类，记录配置参数项的名称和描述<br>
$~~~~~$成员对象: name、description<br>
$~~~~~$成员方法：获取name/description信息<br>
$~~~~~~~~~~~~~~~~~~~~~$获取配置参数的类型描述<br>
$~~~~~~~~~~~~~~~~~~~~~$为获取/载入配置信息的类型转换函数（virtual）<br>

LexicalCast:类型转换模板<br>
1、F->T: 将原类型转换成目标类型,封装的boost库的lexical_cast<T> 所有偏特化模板都调用该类型转换模板<br>
2、将YAML格式的string转换成指定的数据格式T，存储在容器中<br>
3、将存储在容器中的各个项，转换成YAML格式的string返回<br>

ConfigVar:配置参数项（参数名称，参数值，参数描述）<br>
1、继承ConfigVarBase<br>
2、提供了创建配置项的方法，同时提供获取、设置、转换配置项参数值的方法<br>
3、每个配置项都有对应的回调函数

Config:配置项的管理类<br>
1、ConfigVarMap数据结构，记录所有的配置项<br>
2、提供接口支持查找/创建配置项的方法<br>
3、提供初始化配置项的接口<br>
4、提供操作配置项回调函数的方法<br>


## 线程模块
1.线程 Thread<br>
自定义的线程类，封装pthread相关api，利用std::function存储回调函数，增强线程的可用性

2.锁<br>
使用RAII技术，模板技术封装锁，使锁的使用极度简洁<br>

## 协程模块
