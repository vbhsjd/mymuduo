#include"EventLoop.h"
#include"Logger.h"
#include"Poller.h"
#include"Channel.h"

#include<sys/eventfd.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<memory>

/*
EventLoop:事件循环,负责处理IO事件,定时器事件等,是这个系统的核心类.每个线程只能有一个EventLoop对象,EventLoop对象绑定一个线程.
Poller:用于监控文件描述符上的IO事件,如果有事件发生,则通知EventLoop.Poller类提供了一些接口用于向内核注册,删除,修改需要监控的文件描述符.
Channel:是EventLoop和Poller之间的中间层,它包装了文件描述符及其相关的IO事件,并提供了回调函数,当事件发生时调用回调函数.
 */

// 定义一个线程局部变量,用于存储当前线程的EventLoop对象指针
__thread EventLoop* t_loopInThisThread = nullptr;

// 默认的Poller 超时时间,单位是毫秒
const int kPollTimeMs = 10000;

// 创建一个eventfd,用于在主循环(EventLoop)和子循环(subLoop)之间传递消息.
int createEventfd(){
	int evtfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
	if(evtfd < 0){
		LOG_FATAL("eventfd error:%d \n",errno);
	}
	return evtfd;
}

// threadId_记录了当前线程的ID
// poller负责管理事件的监听和处理
// wakeupFd_代表eventfd的文件描述符
// wakeupChannel_代表一个通知channel
EventLoop::EventLoop()
	: looping_(false),
	  quit_(false),
	  callingPendingFunctors_(false),
	  threadId_(CurrentThread::tid()),
	  poller_(Poller::newDefaultPoller(this)),
	  wakeupFd_(createEventfd()), // 生成一个eventfd,每个EventLoop对象都有自己的eventfd
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
	wakeupChannel_->enableReading();
}

// 主要释放wakeupChannel_和wakeupFd_
EventLoop::~EventLoop(){
	wakeupChannel_->disableAll(); // 对所有事件不感兴趣
	wakeupChannel_->remove(); // 从poller中删掉
	::close(wakeupFd_);
	t_loopInThisThread = nullptr;
}

// 这个是wakeupFd_的回调函数
// 就是从eventfd中读取一个uint64_t类型的数值one
void EventLoop::handleRead(){
	uint64_t one = 1;
	ssize_t n = read(wakeupFd_,&one,sizeof one);
	if(n != sizeof one){
		LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8 \n", n);
	}
}
/*
   loop是主要的时间循环函数,会一直循环处理IO事件,直到quit_为true.其中调用了Poller的poll函数获取活跃的Channel列表,然后遍历活跃的Channel列表,调用它们的回调函数,处理发生的事件.
	其他的函数还有quit函数,runInLoop函数和queueInLoop函数,他们都是用来控制EventLoop的运行的.其中queueInLoop函数是把任务放进队列中,由EventLoop所在线程处理.如果当前任务并不在EventLoop所在线程中,那么就需要唤醒相应的需要执行任务的loop所属的线程.而runInLoop函数则是在当前loop中执行回调函数cb,如果cb和loop不对应那么就需要唤醒对应loop.
 */
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
			// 调用回调函数,处理发生的事件
		}
		// 保存的是其他线程希望你这个EventLoop线程执行的函数
		doPendingFunctors(); 
	}

	LOG_INFO("EventLoop %p stop looping \n",this);
	looping_ = false;
}

//1.loop在自己线程调用quit
// 2.调用别的线程的quit
void EventLoop::quit(){
	quit_ = true;

	// ??
	if(!isInLoopThread()){ // 得先确保别的线程被唤醒才能退出
		wakeup();
	}
}

// 该函数保证了cb这个函数对象一定在其EventLoop线程中被调用
void EventLoop::runInLoop(Functor cb){
	if(isInLoopThread()){ // 如果当前调用runInLoop的线程正好就是EventLoop所属的线程,就直接运行此函数
		cb();
	} else {  // 否则调用queueInLoop
		queueInLoop(cb);
	}
}

void EventLoop::queueInLoop(Functor cb){
	{ // 首先把cb保存起来,因为当前线程不是我们期待的EventLoop的线程
		std::unique_lock<std::mutex> lock(mutex_);
		pendingFunctors_.emplace_back(cb);
	}
	// 唤醒相应的需要执行cb的loop所属的线程
	if(!isInLoopThread() or callingPendingFunctors_){
		wakeup(); // 唤醒loop所在线程
	}
}

// wakeup函数向我们想唤醒的线程所绑定的eventloop对象的wakeupfd随便写一个8字节数据
// 因为wakeupfd已经注册到这个eventLoop的事件监听器上,这个时候事件监听器监听到文件描述符的事件发生
// epoll_Wait阻塞结束并返回,这就相当于起了唤醒线程的作用!
// 你这个EventLoop既然阻塞在事件监听上,那我就通过wakeup给你的EventLoop对象一个事件,让你结束阻塞
// 然后可以doPendingFunctors
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
