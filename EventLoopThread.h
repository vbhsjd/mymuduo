#pragma once

#include"noncopyable.h"
#include"Thread.h"

#include<functional>
#include<mutex>
#include<condition_variable>
#include<string>

class EventLoop;

/*
   这个类封装了一个运行在新线程中的EventLoop对象,提供了启动和停止该线程的方法.
   该类主要用于将EventLoop对象和一个新线程封装在一起,方便使用.启动线程池时,创建一个新线程,并在该线程中创建EventLoop对象,之后执行回调函数,最后进入事件循环.停止线程池时,将exiting_设置为true,并通过mutex_和cond_对象通知线程结束,等待线程结束.

	loop_:指向EventLoop对象的指针,线程池运行的事件循环
	exiting_:标识该线程是否正在退出
	thread_:Thread对象,用于管理新线程
	mutex和cond_:用于线程间同步
	callback_:在EventLoop对象被创建后,事件循环启动前执行的回调函数
	startLoop:启动线程池,返回EventLoop对象的指针
	threadFunc:线程执行函数,创建了EventLoop对象,执行回调函数,开始事件循环.
 */

class EventLoopThread :noncopyable{
	public:
		using ThreadInitCallback = std::function<void(EventLoop*)>;

		EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
				const std::string& name = std::string());
		~EventLoopThread();

		EventLoop* startLoop();
	private:
		void threadFunc();
		
		EventLoop* loop_;
		bool exiting_;
		Thread thread_;
		std::mutex mutex_;
		std::condition_variable cond_;
		ThreadInitCallback callback_;
};
