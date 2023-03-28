#pragma once

#include"EventLoop.h"
#include"Acceptor.h"
#include"InetAddress.h"
#include"noncopyable.h"
#include"EventLoopThreadPool.h"
#include"Callbacks.h"
#include"TcpConnection.h"
#include"Buffer.h"

#include<functional>
#include<string>
#include<memory>
#include<atomic>
#include<unordered_map>

class TcpServer : noncopyable{
	public:
		using ThreadInitCallback = std::function<void(EventLoop*)>;
				
		enum Option{
			kNoReusePort,
			kReusePort,
		};

		TcpServer(EventLoop* loop,
				const InetAddress& listenAddr,
				const std::string& nameArg,
				Option option = kNoReusePort);
		~TcpServer();

		void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
		void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
		void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
		void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
		
		// 设置底层subloop的个数
		void setThreadNum(int numThreads);
		
		// 开启服务器监听
		void start();
	private:
		void newConnection(int sockfd,const InetAddress& peerAddr);
		void removeConnection(const TcpConnectionPtr& conn);
		void removeConnectionInLoop(const TcpConnectionPtr& conn);

		using ConnectionMap = std::unordered_map<std::string,TcpConnectionPtr>;

		EventLoop* loop_; // baseloop

		const std::string ipPort_;
		const std::string name_;

		std::unique_ptr<Acceptor> acceptor_; // mainloop 的acceptor
		std::shared_ptr<EventLoopThreadPool> threadPool_;
		
		ConnectionCallback connectionCallback_; // 有新用户连接时的回调
		MessageCallback messageCallback_; // 有读写消息时的回调
		WriteCompleteCallback writeCompleteCallback_; // 消息发送完成时的回调
		ThreadInitCallback threadInitCallback_; // loop线程初始化回调
		std::atomic_int started_;
		
		int nextConnId_;
		ConnectionMap connections_; // 保存所有的连接
};
