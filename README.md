# mymuduo
### 简介
mymuduo基于事件驱动和事件回调的epoll+线程池面向对象编程，采用Multi-Reactor架构以及One Loop Per Thread设计思想的多线程C++网络库。mymuduo重写了muduo核心组件，去除了对boost库的依赖，使用C++11重构，使用互斥锁以及智能指针管理对象。重要组件包括Channel、Poller、EventLoop、TcpServer等，同时还实现了Buffer缓冲区。

### 技术特点
* 完全去掉了Muduo库中的Boost依赖，全部使用C++11新语法，如智能指针、function函数对象、lambda表达式等
* 没有单独封装Thread，使用C++11引入的std::thread搭配lambda表达式实现工作线程，没有直接使用pthread库
* 只实现了epoll这一个IO-Multiplexing,未实现poll
* Buffer部分Muduo库没有提供writeFd方法，本项目加入了writeFd，在处理outputBuffer剩余未发数据时交给Buffer来处理
* 整个项目采用Cmake编译，同时使用shell脚本实用一键编译
### 使用说明
* 环境：Linux系统，g++需要支持C++11及以上标准
* 编译安装：运行mymuduo目录下的autobuild.sh文件
```
sudo sh autobuild.sh
```
autobuild脚本会将整个项目编译，并将代码头文件自动拷贝到系统的/usr/local/include/mymuduo目录下，将生成的lib库直接拷到系统的/usr/lib目录下
* 示例代码：在安装好mymuduo后，进入mymuduo下的test目录下，执行make就会生成可执行文件
### 项目梳理
#### 第一模块：基本类
* Noncopyable：不可拷贝的基类
* Logger：日志类
* Timestamp：时间类
* InetAddress：封装 socket 地址
#### 第二模块：Channel、Poller、EventLoop
* Channel：打包 fd 和感兴趣的 events
* Poller：epoll、poll 的基类
* EPollPoller：封装 epoll 的操作
* CurrentThread：获取当前线程 tid，每个 channel 要在自己的 eventloop 线程上进行处理
* Eventloop：事件轮询，连接 poller 和 channel
* Thread：底层线程
* EventLoopThread：one loop per thread，绑定一个 loop 和一个 thread
* EventLoopThreadPool：线程池，有 main loop 和 sub loop
#### 第三模块：Acceptor、TcpServer、TcpConnection
* Socket：封装socket、bind、listen、accept等操作
* InetAddress：封装socket地址
* Acceptor：处理accept，监听新用户的连接，打包成channel，分发给subloop
* TcpServer：集大成者，负责协调各个类之间的关系，实现多线程、多连接的网络编程
* Buffer：实现缓冲区
* TcpConnection：实现建立和客户端的新连接，处理连接上的数据发送和接收
