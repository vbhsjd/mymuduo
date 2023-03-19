#pragma once

#include"noncopyable.h"
#include"Timestamp.h"
#include"CurrentThread.h"

#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>

class Channel;
class Poller;

// 事件循环类,主要包含了Channel和Poller(epoll)两大模块
class EventLoop:noncopyable {
	public:
		using Functor = std::function<void()>;

		EventLoop();
		~EventLoop();

		void loop(); // 开启事件循环
		void quit(); // 退出事件循环

		Timestamp pollReturnTime() const { return pollReturnTime_; }
		
		// 在当前loop中执行cb
		void runInLoop(Functor cb);
		// 把cb放入队列中,唤醒loop所在的线程,执行cb
		void queueInLoop(Functor cb);
		
		// 用来唤醒loop所在的线程
		void wakeup();
		
		// 实际上是直接调用poller的方法,epoll_ctl
		void updateChannel(Channel* channel);
		void removeChannel(Channel* channel);
		bool hasChannel(Channel* channel);

		// 判断eventLoop是否在自己的线程里面
		bool isInLoopThread() const
			{ return threadId_ == CurrentThread::tid(); }
	private:
		void handleRead(); // waked up
		void doPendingFunctors(); // 执行回调函数

		using ChannelList = std::vector<Channel*>;

		std::atomic<bool> looping_; // 原子操作
		std::atomic<bool> quit_; // 标志是否退出loop

		const pid_t threadId_; // 记录当前loop所在线程的id

		Timestamp pollReturnTime_; // poller返回发生事件的时间点
		std::unique_ptr<Poller> poller_;

		int wakeupFd_; // ***重点当mainLoop获取到channel
		// 通过轮询算法,选择一个subLoop,通过wakeupFd_唤醒loop
		std::unique_ptr<Channel> wakeupChannel_;

		ChannelList activeChannels_;
		
		// 标识当前loop是否有需要执行的回调操作
		std::atomic<bool> callingPendingFunctors_; 
		std::vector<Functor> pendingFunctors_; // 存储loop需要执行的回调操作
		std::mutex mutex_; // 互斥锁,用来保护上面vector的线程安全操作
};
