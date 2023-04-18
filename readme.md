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
1.主协程负责分配子协程，子协程执行结束返回主协程<br>
2.协程只能在堆上进行分配<br>
3.协程的内存模型：<br>
--->协程对象都在堆上，线程只操作协程的智能指针<br>
--->协程对应的栈空间都在堆上分配<br>
--->主协程没有栈空间<br>
--->将子协程的回调函数分配给对应的上下文<br>
4.协程提供的方法主要有三种<br>
--->获取自身属性<br>
--->切换自身状态<br>
--->封装给协程调度器的安全的调度接口<br>

## 协程调度模块
Scheduler(调度器，基类)<br>
1.成员对象：<br>
&emsp;&emsp;线程相关：线程组、线程id组、线程数量、工作线程数量、空闲线程数量<br>
&emsp;&emsp;调度器相关：调度器的自身状态<br>
&emsp;&emsp;协程相关：待执行的协程队列，待添加的协程元素<br>

2.sylar的协程调度器是一个N（线程）-M（协程）的多线程协程调度器<br>
&emsp;&emsp;在N个调度线程中，存在一个caller线程，是核心的调度线程，存在该线程时，只有该线程可以执行stop方法<br>

3.协程调度器的创建<br>
&emsp;&emsp;sylar的协程调度器可以在N个线程上调度M个协程，创建时只创建caller线程<br>
&emsp;&emsp;创建所需要的参数为线程数量、use_caller标记、调度器名称<br>
&emsp;&emsp;调度器创建时，将当前线程设置为caller线程，并在当前线程中创建一个调度协程执行run方法<br>

4.协程调度器的启动和运行<br>
&emsp;&emsp;启动除caller外的其他N-1个线程<br>
&emsp;&emsp;为其他N-1个线程绑定调度器的run方法<br>
&emsp;&emsp;sylar的协程调度器的run方法:<br>
<1> 首先遍历协程链表找到一个可调度协程，这个可调度协程在获取到的同时也被从链表上摘除下来了。<br>
<2> 执行该可调度协程时，在执行完成后，他有三种逻辑流：<br>
---->该协程为Ready状态，重新放入调度队列<br>
---->非终止或非异常状态，将协程设置为Hold状态(个人认为，该逻辑流是一个冗余操作)<br>
---->什么都不操作<br>
<3> 不存在可调度协程时，执行idle协程<br>
--->idle协程中途退出证明未达到退出条件，继续执行run方法，正常退出说明达到了退出条件

5.协程调度器的停止<br>
<1> 为了支持调度器在启动后仍能接收协程任务，stop方法首先确保caller线程的调度协程执行结束<br>
<2> 负责接收其他线程<br>

## IO协程调度模块
1.IOManager继承自协程调度器Scheduler,可以在多个线程中对IO调度进行管理<br>
2.idle方法重写，负责执行epoll_wait，即，无IO事件时，监听IO到来<br>
3.IO协程调度的策略是：有待执行任务时，优先执行待执行任务，减少IO等待<br>
4.调度的基本单位：协程（继承）、socket事件上下文（特有）<br>
5.基本流程：<br>
&emsp;&emsp：通过addEvent，向IO调度器的Fd_Contexts添加socket事件上下文<br>
&emsp;&emsp：通过唤醒idle协程，将获取到的IO就绪事件进行包装，将对应事件的具体执行放到协程组中<br>

## IO定时器模块
1.Timer的基本方法：addTimer、cancelTimer<br>
2.Timer 需要能够获取当前定时器距离触发时间的时间差
3.工作流程：
&emsp;&emsp：添加定时器模块，定时器在idle协程中发挥作用



