#pragma once

#include"noncopyable.h"
#include"InetAddress.h"
#include"Buffer.h"
#include"Callbacks.h"
#include"Timestamp.h"

#include<memory>
#include<string>
#include<atomic>

class Channel;
class EventLoop;
class Socket;

/*
   这个类主要封装了一个已建立的TCP连接,以及控制该TCP连接的一些方法,还有连接发生后的各种事件的处理函数,以及这个连接的服务端客户端信息.
   Acceptor和TcpConnection应该是兄弟关系,Acceptor用于mainloop,对服务器监听套接字fd及其相关方法进行封装(监听,接受连接,分发连接给subloop等),而TcpConnection用于subloop,对连接套接字fd及其相关方法进行封装(读消息事件,发送消息事件,连接关闭事件,错误时间等)
   重要的成员函数:handleError,handleClose,handleWrite,handleRead
 */
class TcpConnection : noncopyable,public std::enable_shared_from_this<TcpConnection>{
	public:
		TcpConnection(EventLoop* loop,
				const std::string& name,
				int sockfd,
				const InetAddress& localAddr,
				const InetAddress& peerAddr);
		~TcpConnection();

		EventLoop* getLoop() const { return loop_; }
		const std::string& name() const { return name_; }
		const InetAddress& localAddress() const { return localAddr_; }
		const InetAddress& peerAddress() const { return peerAddr_; }

		bool connected() const { return state_ == kConnected; }

		// 发送数据
		void send(const std::string& buf);
		void shutdown();

		void setConnectionCallback(const ConnectionCallback& cb)
		{ connectionCallback_ = cb; }
		void setMessageCallback(const MessageCallback& cb)
		{ messageCallback_ = cb; }
		void setWriteCompleteCallback(const WriteCompleteCallback& cb)
		{ writeCompleteCallback_ = cb; }
		void setHighWaterMarkCallback(const HighWaterMarkCallback& cb)
		{ highWaterMarkCallback_ = cb; }
		void setCloseCallback(const CloseCallback& cb)
		{ closeCallback_ = cb; }

		void connectEstablished();
		void connectDestroyed();
	private:
		// 负责处理TCP连接的可读事件,把客户端发来的数据拷贝到用户缓冲区
		// 也就是inputBuffer_,接着调用connectionCallback_[连接建立后的处理函数]
		void handleRead(Timestamp receiveTime);
		void handleWrite();
		void handleClose();
		void handleError();

		void sendInLoop(const void* message,int len);
		void shutdownInLoop();

		enum StateE{ kDisconnected,kConnecting,kConnected,kDisconnecting };
		void setState(StateE state) { state_ = state; }

		EventLoop* loop_; // 表示Tcp连接的channel注册到哪一个subloop上
		const std::string name_;
		std::atomic<int> state_; // 标识了当前TCP连接所处的状态
		bool reading_;

		// 3.channel
		std::unique_ptr<Socket> socket_; // 用于保存已建立连接的客户端fd
		std::unique_ptr<Channel> channel_;
		// 封装了上面的Socket及其各类事件的处理函数(读,写,错误,关闭等处理函数).这些处理事件函数是在TcpConnection的构造函数中注册的
		const InetAddress localAddr_;
		const InetAddress peerAddr_;

		// 1.用户会自定义部分回调函数.
		// 这些函数会分别注册给下面的变量保存
		ConnectionCallback connectionCallback_;
		MessageCallback messageCallback_;
		WriteCompleteCallback writeCompleteCallback_;
		CloseCallback closeCallback_;
		HighWaterMarkCallback highWaterMarkCallback_;		
		size_t highWaterMark_;
		
		//2.缓冲区
		Buffer inputBuffer_; // 用户接受缓冲区
		Buffer outputBuffer_;
		// 也是个缓冲区,不过是暂存那些发不出去的待发送数据
		// 因为Tcp发送缓冲区是有大小限制的,加入到了高水位线,
		// 就没办法把数据都通过send直接拷贝去tcp发送缓冲区,
		// 而是暂存一些在这个outputBuffer_中,等待Tcp发送缓冲区有了空间
		// 出发了写事件,再把outputBuffer_拷贝到Tcp发送缓冲区
};
