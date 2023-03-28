#include"EventLoopThread.h"
#include"EventLoop.h"

// cb是一个回调函数,用于在EventLoop对象初始化后执行
EventLoopThread::EventLoopThread(
		const ThreadInitCallback& cb,const std::string& name)
		: loop_(nullptr),
		  exiting_(false),
		  thread_(std::bind(&EventLoopThread::threadFunc,this),name),
		  mutex_(),
		  cond_(),
		  callback_(cb)
{
}

EventLoopThread::~EventLoopThread(){
	exiting_ = true;
	if(loop_ != nullptr){
		loop_->quit();
		thread_.join();
	}
}

EventLoop* EventLoopThread::startLoop(){
	thread_.start(); // 启动新线程

	EventLoop* loop = nullptr;
	// 等待threadFunc函数创建loop
	{
		std::unique_lock<std::mutex> lock(mutex_);
		while(loop_ == nullptr){
			cond_.wait(lock);
		}
		loop = loop_;
	}

	return loop;
}

// 这个函数在新线程中运行,用于执行事件循环
// 首先创建一个EventLoop对象,并将其设置为loop_成员变量.
// 然后执行回调函数cb
// 接下来,调用loop.loop()开始事件循环,等待事件发生.最后将loop_设置为nullptr,释放资源
//
void EventLoopThread::threadFunc(){
	EventLoop loop; // 创建新的loop,和上面线程对应

	if(callback_){
		callback_(&loop);
	}

	{
		std::unique_lock<std::mutex> lock(mutex_);
		loop_ = &loop;
		cond_.notify_one();
	}

	loop.loop(); // poller.poll	
	std::unique_lock<std::mutex> lock(mutex_);
	loop_ = nullptr;	
}
