#include"EventLoop.h"
#include"Logger.h"
#include"Poller.h"
#include"Channel.h"

#include<sys/eventfd.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<memory>

// 防止一个线程创建多个eventLoop __thread->thread_local
__thread EventLoop* t_loopInThisThread = nullptr;

// 默认的Poller 超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd,用来通知subloop,处理新的channel
int createEventfd(){
	int evtfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
	if(evtfd < 0){
		LOG_FATAL("eventfd error:%d \n",errno);
	}
	return evtfd;
}

EventLoop::EventLoop()
	: looping_(false),
	  quit_(false),
	  callingPendingFunctors_(false),
	  threadId_(CurrentThread::tid()),
	  poller_(Poller::newDefaultPoller(this)),
	  wakeupFd_(createEventfd()),
	  wakeupChannel_(new Channel(this,wakeupFd_))
{
	LOG_DEBUG("EventLoop created %p in thread %d \n",this,threadId_);
	if(t_loopInThisThread){ // 该线程已经有loop了
		LOG_FATAL("Another EventLoop %p exists in this thread %d \n",
				t_loopInThisThread,threadId_);
	} else { // 当前线程第一次创建EventLoop对象
		t_loopInThisThread = this;
	}

	// 设置wakeupfd_的事件类型,以及发生事件后的回调操作
	wakeupChannel_->setReadCallback(
			std::bind(&EventLoop::handleRead,this));
	// 每个eventLoop都会监听wakeupchannel的epollin事件
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
	wakeupChannel_->disableAll(); // 对所有事件不感兴趣
	wakeupChannel_->remove(); // 从poller中删掉
	::close(wakeupFd_);
	t_loopInThisThread = nullptr;
}

void EventLoop::handleRead(){
	uint64_t one = 1;
	ssize_t n = read(wakeupFd_,&one,sizeof one);
	if(n != sizeof one){
		LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8 \n", n);
	}
}

void EventLoop::loop(){
	looping_ = true;
	quit_ = false;
	
	LOG_INFO("EventLoop %p strat looping \n",this);

	while(!quit_){
		activeChannels_.clear();
		//监听两类fd,clientfd和wakeupfd
		pollReturnTime_ = poller_->poll(kPollTimeMs,&activeChannels_);
		for(Channel* channel:activeChannels_){
			channel->handleEvent(pollReturnTime_);
		}
		// 
		doPendingFunctors(); 
		// 执行当前loop需要处理的回调操作,这个是mainloop发过来的
	}

	LOG_INFO("EventLoop %p stop looping \n",this);
	looping_ = false;
}

//1.loop在自己线程调用quit
// 2.调用别的线程的quit
void EventLoop::quit(){
	quit_ = true;

	if(!isInLoopThread()){ // 得先确保别的线程被唤醒才能退出
		wakeup();
	}
}
// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb){
	if(isInLoopThread()){
		cb();
	} else { // cb和loop不对应,那么就需要唤醒对应loop
		queueInLoop(cb);		
	}
}

// 把cb放入队列中,唤醒loop所在的线程,执行cb
void EventLoop::queueInLoop(Functor cb){
	{
		std::unique_lock<std::mutex> lock(mutex_);
		pendingFunctors_.emplace_back(cb);
	}
	// 唤醒相应的需要执行cb的loop所属的线程
	if(!isInLoopThread() or callingPendingFunctors_){
		wakeup(); // 唤醒loop所在线程
	}
}

// 用来唤醒loop所在的线程
void EventLoop::wakeup(){
	uint64_t one = 1;
	ssize_t n = write(wakeupFd_,&one,sizeof(one));
	if(n != sizeof one){
		LOG_ERROR("EventLoop::wakeup() writes %d bytes instead of 8 \n",n);
	}
}

// 实际上是直接调用poller的方法,epoll_ctl
void EventLoop::updateChannel(Channel* channel){
	poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
	poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel){
	return poller_->hasChannel(channel);
}

// 执行回调
void EventLoop::doPendingFunctors(){ // 执行回调
	std::vector<Functor> functors; // 重点
	callingPendingFunctors_ = true;
	
	{ // 把pendingFunctors_ 里的回调取出来,在局部变量里面慢慢执行
		std::unique_lock<std::mutex> lock(mutex_);
		functors.swap(pendingFunctors_);
	}
	
	for(const Functor& functor:functors){
		functor();
	}
	callingPendingFunctors_ = false;
}
