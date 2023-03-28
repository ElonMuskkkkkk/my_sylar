# 目录说明
sylar -- 源码
tests -- 测试

# 功能模块

## 日志模块

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


