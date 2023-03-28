#include"TcpServer.h"
#include"Logger.h"
#include"InetAddress.h"
#include"TcpConnection.h"

#include<functional>
#include<strings.h>

// static表示这个函数作用域只是这个文件
static EventLoop* CheckLoopNotNull(EventLoop* loop){
	if(loop == nullptr){
		LOG_FATAL("%s:%s:%d mainloop is null! \n",__FILE__,__FUNCTION__,__LINE__);
	}
	return loop;
}

TcpServer::TcpServer(EventLoop* loop,
		const InetAddress& listenAddr,
		const std::string& nameArg,
		Option option)
	:loop_(CheckLoopNotNull(loop)),
	 ipPort_(listenAddr.toIpPort()),
	 name_(nameArg),
	 acceptor_(new Acceptor(loop,listenAddr,option == kReusePort)),
	 threadPool_(new EventLoopThreadPool(loop,name_)),
	 connectionCallback_(),
	 messageCallback_(),
	 nextConnId_(1),
	 started_(0)
{
	// 当有新用户连接时,会执行下面这个回调Tcp::newConnection
	acceptor_->setNewConnectionCallback(
			std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));
}

TcpServer::~TcpServer(){
	for(auto& item:connections_){
		TcpConnectionPtr conn(item.second); // 现在conn和item.second共同持有这个TcpConnection
		item.second.reset(); // 释放掉TcpServer保存的该TcpConnection的智能指针,现在只剩下conn了
		conn->getLoop()->runInLoop( // subloop执行connectDestroyed
				std::bind(&TcpConnection::connectDestroyed,conn)
		);
	} // 出了这个作用域,TcpConnectionPtr析构,TcpConnection在Destroy执行结束也会析构
}

void TcpServer::newConnection(int sockfd,const InetAddress& peerAddr){
	EventLoop* ioLoop = threadPool_->getNextLoop();
	char buf[64] = {0};
	snprintf(buf,sizeof buf,"-%s#%d",ipPort_.c_str(),nextConnId_);
	++nextConnId_;
	std::string connName = name_ + buf;
	LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());

	sockaddr_in local;
	::bzero(&local,sizeof local);
	socklen_t len = sizeof local;
	if(::getsockname(sockfd,(sockaddr*)&local,&len) < 0){
		LOG_ERROR("getsockname error\n");

	}
	InetAddress localAddr(local);

	// 根据连接成功的sockfd,创建TcpConnection对象
	TcpConnectionPtr conn(new TcpConnection(ioLoop,connName,sockfd,localAddr,peerAddr));
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	// 设置了关闭连接的回调
	conn->setCloseCallback(
			std::bind(&TcpServer::removeConnection,this,std::placeholders::_1)
	);

	ioLoop->runInLoop(
			std::bind(&TcpConnection::connectEstablished,conn)
	);
}

// 设置底层subloop的个数
void TcpServer::setThreadNum(int numThreads){
	threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听
void TcpServer::start(){
	if(started_++ == 0){ // 防止一个tcpserver对象被start多次
		threadPool_->start(threadInitCallback_); // 启动底层的线程池
		loop_->runInLoop([this]{ acceptor_->listen(); });
	}
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn){
	loop_->runInLoop(
			std::bind(&TcpServer::removeConnectionInLoop,this,conn)
	);	
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn){
	LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n",name_.c_str(),conn->name().c_str());
	connections_.erase(conn->name());
	EventLoop* ioLoop = conn->getLoop();
	ioLoop->queueInLoop(
			std::bind(&TcpConnection::connectDestroyed,conn)
	);
}
