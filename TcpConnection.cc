#include"TcpConnection.h"
#include"Logger.h"
#include"Socket.h"
#include"Channel.h"
#include"EventLoop.h"

#include<functional>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<strings.h>
#include<netinet/tcp.h>
#include<string>

static EventLoop* CheckLoopNotNull(EventLoop* loop){
	if(loop == nullptr){
		LOG_FATAL("%s:%s:%d TcpConnection loop is null! \n",__FILE__,__FUNCTION__,__LINE__);
	}
	return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
				const std::string& name,
				int sockfd,
				const InetAddress& localAddr,
				const InetAddress& peerAddr)
	: loop_(CheckLoopNotNull(loop)),
	  name_(name),
	  state_(kConnecting),
	  reading_(true),
	  socket_(new Socket(sockfd)),
	  channel_(new Channel(loop,sockfd)),
	  localAddr_(localAddr),
	  peerAddr_(peerAddr),
	  highWaterMark_(64*1024*1024)
{
	channel_->setReadCallback(
			std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
	channel_->setWriteCallback(
			std::bind(&TcpConnection::handleWrite,this));
	channel_->setCloseCallback(
			std::bind(&TcpConnection::handleClose,this));
	channel_->setErrorCallback(
			std::bind(&TcpConnection::handleError,this));

	LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n",name_.c_str(),sockfd);
	socket_->setKeepAlive(true);
}

	  
TcpConnection::~TcpConnection()
{
	LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n",name_.c_str(),channel_->fd(),(int)state_);
}

// 负责处理TCP连接的可读事件,它将客户端发来的数据拷贝到用户缓冲区inputBuffer_
// 接着调用connectionCallback_[连接建立的处理函数]
void TcpConnection::handleRead(Timestamp receiveTime){
	int savedErrno = 0;
	ssize_t n = inputBuffer_.readFd(channel_->fd(),&savedErrno);
	if(n > 0){
		// 已建立连接的用户有可读事件发生,调用用户的回调操作
		messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
	} else if(n == 0){ // 客户端断开连接
		handleClose();
	} else {
		errno = savedErrno;
		LOG_ERROR("TcpConnection::handleRead\n");
		handleError();
	}
}

// 负责处理TCP连接的可写事件
// 调用buffer::writeFd来将buffer的数据拷贝到TCP发送缓冲区
void TcpConnection::handleWrite(){
	if(channel_->isWriting()){
		int savedErrno = 0;
		ssize_t n = outputBuffer_.writeFd(channel_->fd(),&savedErrno);
		if(n > 0){
			outputBuffer_.retrieve(n);
			// 需要发送的数据发完了,不再需要写事件了
			// 否则,继续保持可写事件的监听
			if(outputBuffer_.readableBytes() == 0){
				channel_->disableWriting();  // 移除可写事件
				if(writeCompleteCallback_){ // 调用用户自定义的[写完后的事件处理函数]
					loop_->queueInLoop(
							std::bind(writeCompleteCallback_,shared_from_this()));
				}
				if(state_ == kDisconnecting) { shutdownInLoop(); }
			}
		} // end if > 0
		else {
			LOG_ERROR("TcpConnection::handleWrite\n");
		}
	}
	else {
		LOG_ERROR("TcpConnection fd=%d is down,no more writing\n",channel_->fd());
	}
}

// 负责处理TCP连接的关闭事件
// 主要就是把这个TcpConnection的channel从事件监听器中移除
// 然后调用相应的处理函数
void TcpConnection::handleClose(){
	LOG_INFO("TcpConnection::handleClose fd=%d,state=%d\n",channel_->fd(),(int)state_);
	setState(kDisconnected);
	channel_->disableAll(); // 将channel从事件监听器上移除

	TcpConnectionPtr connPtr(shared_from_this());
	connectionCallback_(connPtr); // 调用用户自定义的连接事件处理函数
	closeCallback_(connPtr); // 
}

void TcpConnection::handleError(){
	int err = 0;
	int optval;
	socklen_t optlen = sizeof(optval);
	if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen) < 0){
		err = errno;
	} else{
		err = optval;
	}
	LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n",name_.c_str(),err);
}

void TcpConnection::send(const std::string& buf){
	if(state_ == kConnected){
		if(loop_->isInLoopThread()){
			sendInLoop(buf.c_str(),buf.size());
		} else {
			loop_->runInLoop(
					std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size()));	
		}
	}
}

// 发送数据
void TcpConnection::sendInLoop(const void* data,int len){
	size_t nwrote = 0;
	size_t remaining = len;
	bool faultError = false;
	// 已经调用过shutdown了,不能继续发送
	if(state_ == kDisconnected){
		LOG_ERROR("disconnected,give up writing\n");
		return;
	}
	// 表示channel第一次开始写数据,而且缓冲区没有发送的数据
	if(!channel_->isWriting() and outputBuffer_.readableBytes() == 0){
		nwrote = ::write(channel_->fd(),data,len);
		if(nwrote >= 0){
			remaining = len - nwrote;
			if(remaining == 0 and writeCompleteCallback_){
				loop_->queueInLoop(
						std::bind(writeCompleteCallback_,shared_from_this()));
			}
		} else {  // error
			nwrote = 0;
			if(errno != EWOULDBLOCK){
				LOG_ERROR("TcpConnection::sendInLoop\n");
				if(errno == EPIPE or errno == ECONNRESET){
					faultError = true;
				}
			}
		}
	}

	// 未发生错误而且这个write没有把数据全部发送过去
	// 未拷贝到TCP发送缓冲区的buf数据会被存到outputBuffer_中,并且向事件监听器上注册可写事件
	if(!faultError and remaining > 0){
		size_t oldLen = outputBuffer_.readableBytes();
		if(oldLen + remaining >= highWaterMark_
				and oldLen < highWaterMark_
				and highWaterMarkCallback_)
		{
			loop_->queueInLoop(
					std::bind(highWaterMarkCallback_,shared_from_this(),oldLen+remaining)
					);
		}
		// 保存没拷贝的数据
		outputBuffer_.append((char*)data+nwrote,remaining);
		if(!channel_->isWriting()){ // 注册可写事件
			channel_->enableWriting();
		}
	}
}

void TcpConnection::connectEstablished(){
	setState(kConnected);
	channel_->tie(shared_from_this());
	channel_->enableReading();

	connectionCallback_(shared_from_this());
}

// 从poller删除channel,并且取消监听
void TcpConnection::connectDestroyed(){
	if(state_ == kConnected){
		setState(kDisconnected);
		channel_->disableAll(); // update后就取消监听了
		connectionCallback_(shared_from_this()); // 用户自定义的连接事件的处理函数
	}
	channel_->remove(); // 把channel从subEventLoop的Poller中删除
}

void TcpConnection::shutdown(){
	if(state_ == kConnected){
		setState(kDisconnecting);
		loop_->runInLoop(
				std::bind(&TcpConnection::shutdownInLoop,this));
	}
}

void TcpConnection::shutdownInLoop(){
	// 数据已经全部发送完成
	if(!channel_->isWriting()){
		socket_->shutdownWrite();
	}
}
