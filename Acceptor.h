#pragma once 

#include"noncopyable.h"
#include"Socket.h"
#include"Channel.h"

#include<functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable {
	public:
		using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;

		Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
		~Acceptor();

		// 这个callback的作用是公平地选择一个subEventLoop,把接受连接的连接分发给他
		void setNewConnectionCallback(const NewConnectionCallback& cb) {
			newConnectionCallback_ = cb;
		}
		
		void listen();
		bool listenning() const { return listenning_; }
	private:
		void handleRead();

		EventLoop* loop_; // 这个listenfd(acceptSocket_ or acceptChannel_) 由哪个EventLoop负责循环监听以及处理相应事件
		// 其实就是main eventloop
		Socket acceptSocket_; // 监听套接字的文件描述符->listenfd
		Channel acceptChannel_; // 是个Channel,就是封装了lisenfd(acceptSocket_),以及事件
		NewConnectionCallback newConnectionCallback_;
		bool listenning_;
};
