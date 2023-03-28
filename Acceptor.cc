#include"Acceptor.h"
#include"Logger.h"
#include"Channel.h"
#include"EventLoop.h"
#include"InetAddress.h"

#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<unistd.h>

// static 这个函数只能在这个文件使用
static int createNonblocking(){
	int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,0);
	if(sockfd < 0) {
		LOG_FATAL("%s:%s:%d listen socket create error:%d\n",__FILE__,__FUNCTION__,__LINE__,errno);
	}
	return sockfd;
}

Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport)
	:loop_(loop),
	 acceptSocket_(createNonblocking()), // socket
	 acceptChannel_(loop,acceptSocket_.fd()),
	 listenning_(false)
{
	acceptSocket_.setReuseAddr(true);
	acceptSocket_.setReusePort(reuseport);
	acceptSocket_.bindAddress(listenAddr); // bind
	acceptChannel_.setReadCallback( // 把handleRead注册到acceptChannel_上
			std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor(){
	acceptChannel_.disableAll();
	acceptChannel_.remove();
}

// 调用listen(),开启对acceptSocket_的监听,同时让acceptChannel_注册到main eventloop的事件监听器上.
void Acceptor::listen(){
	listenning_ = true;
	acceptSocket_.listen(); // listen
	acceptChannel_.enableReading(); // 把channel注册到poller上
}

// 在构造函数已经将handleRead注册到acceptChannel_上了.
// 当main eventLoop上监听到acceptChannel_上发生了可读事件(也就是有新用户连接)
// 就会调用这个readcallback,就是handleRead
// handread的作用是,首先accept接受连接,然后调用newConnectionCallback_
// 这个newConnectionCallback_是tcpserver传进来的,以负载均衡的选择方式选择一个sub eventLoop
// 把这个新连接分发到sub EventLoop上,开始循环监听
void Acceptor::handleRead() {
	InetAddress peerAddr;
	int connfd = acceptSocket_.accept(&peerAddr); // 接受新连接
	if(connfd >= 0){
		if(newConnectionCallback_){
			newConnectionCallback_(connfd,peerAddr);
		} else {
			::close(connfd);
		}
	} else { // error
		LOG_ERROR("%s:%s:%d accept error:%d\n",__FILE__,__FUNCTION__,__LINE__,errno);
		if(errno == EMFILE){
			LOG_ERROR("%s:%s:%d accept sockfd reached limit\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}
}
