#include"Channel.h"
#include"Logger.h"
#include"EventLoop.h"

#include<sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;


Channel::Channel(EventLoop* loop,int fd)
	:loop_(loop),
	fd_(fd),
	events_(0),
	revents_(0),
	index_(-1),
	tied_(false)
{
}

Channel::~Channel() { } // 确保channel被ownerloop析构

void Channel::tie(const std::shared_ptr<void>& obj){
	tie_ = obj;
	tied_ = true;
}

/*
   当改变channel的fd的events后
   需要update在poller里面改变事件的状态epoll_ctl
 */
void Channel::update(){
	// 通过EventLoop,调用poller方法
	loop_->updateChannel(this);
}

// 在channel所属的eventLoop删除掉该channel
void Channel::remove(){
	// 通过eventLoop删除channel
	loop_->removeChannel(this);
}

// ??
void Channel::handleEvent(Timestamp receiveTime){
	if(tied_){
		std::shared_ptr<void> guard = tie_.lock(); // 提升
		if(guard) { // 提升成功
			handleEventWithGuard(receiveTime);
		}
	} else {
		handleEventWithGuard(receiveTime);
	}
}

// 根据poller通知给channel的相应事件,由channel负责调用回调函数
void Channel::handleEventWithGuard(Timestamp receiveTime){
	LOG_INFO("channel handleEvent revents:%d\n",revents_);

	if((revents_ & EPOLLHUP) and !(revents_ & EPOLLIN)){
		if(closeCallback_) {
			closeCallback_();
		}
	}

	if(revents_ & EPOLLERR) {
		if(errorCallback_){
			errorCallback_();
		}
	}

	if(revents_ & (EPOLLIN | EPOLLPRI)) {
		if(readCallback_){
			readCallback_(receiveTime);
		}
	}

	if(revents_ & EPOLLOUT){
		if(writeCallback_){
			writeCallback_();
		}
	}
}

