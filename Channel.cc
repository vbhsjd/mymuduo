#include<sys/epoll.h>

#include"Channel.h"
#include"Logger.h"
#include"EventLoop.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

/*
   tie函数的作用是在Channel中绑定一个shared_ptr对象,目的是在该shared_ptr对象被销毁时,将Channel对象从其所属的EventLoop中删除.如果不使用tie函数,则在Channel对象被销毁时,可能会在其所属的EventLoop中引发空悬指针问题.
   handleEvent函数的作用是处理Channel上的事件.在该函数中,首先判断tied_标志是否为true.如果为true,说明在构造函数时,使用了tie函数绑定了一个shared_ptr对象,因此需要先提升shared_ptr对象的引用计数,以确保在处理事件的过程中,该shared_ptr对象不会被析构.
	接下来,handleEventWithGuard函数会根据收到的事件类型,调用相应的回调函数.
 */

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
// 日志输出可以更加详细的说明发生了哪些事件,比如输出revents_中每个位对应的事件名称
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
