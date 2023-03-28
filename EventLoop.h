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

/* 
   事件循环类,主要包含了Channel和Poller(epoll)两大模块
*/
class EventLoop:noncopyable {
	public:
		using Functor = std::function<void()>;

		EventLoop();
		~EventLoop();

		void loop(); // 开启事件循环
		void quit(); // 退出事件循环

		// 返回poller发生事件的时间点
		Timestamp pollReturnTime() const { return pollReturnTime_; }
		
		// 在当前loop中执行cb
		void runInLoop(Functor cb);
		// 把cb放入队列中,唤醒loop所在的线程,执行cb
		void queueInLoop(Functor cb);
		
		// 用来唤醒loop所在的线程
		void wakeup();
		
		// 实际上是直接调用poller的方法,epoll_ctl
		void updateChannel(Channel* channel); // for poller
		void removeChannel(Channel* channel);
		bool hasChannel(Channel* channel);

		// 判断eventLoop是否在自己的线程里面
		bool isInLoopThread() const
			{ return threadId_ == CurrentThread::tid(); }
	private:
		// 被wakeupChannel_的read事件回调函数调用,唤醒loop所在线程
		void handleRead();
		void doPendingFunctors(); // 执行回调函数

		using ChannelList = std::vector<Channel*>;

		std::atomic<bool> looping_; // 标志是否在循环中
		std::atomic<bool> quit_; // 标志是否退出loop

		const pid_t threadId_; // 记录当前loop所在线程的id

		Timestamp pollReturnTime_; // poller返回发生事件的时间点
		std::unique_ptr<Poller> poller_;

		// 用于唤醒loop所在的线程的文件描述符
		int wakeupFd_;
		std::unique_ptr<Channel> wakeupChannel_;

		// 存储发生事件的Channel对象
		ChannelList activeChannels_;
		
		// 标识当前loop是否有需要执行的回调操作
		std::atomic<bool> callingPendingFunctors_; 
		std::vector<Functor> pendingFunctors_; // 存储loop需要执行的回调操作
		std::mutex mutex_; // 互斥锁,用来保护上面vector的线程安全
};
