#pragma once 

#include "noncopyable.h"

#include<functional>
#include<string>
#include<vector>
#include<memory>

class Thread;
class EventLoopThread;
class EventLoop;

class EventLoopThreadPool{
	public:
		using ThreadInitCallback = std::function<void(EventLoop*)>;
		
		EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg);
		~EventLoopThreadPool();

		void setThreadNum(int numThreads) { numThreads_ = numThreads; }
		void start(const ThreadInitCallback& cb = ThreadInitCallback());

		// 如果是多线程,baseLoop_会以轮询的方式分配Channel给subLoop
		EventLoop* getNextLoop();
		std::vector<EventLoop*> getAllLoops();

		bool started() const { return started_; }
		const std::string& name() const { return name_; }
		
	private:
		EventLoop* baseLoop_;
		std::string name_;
		bool started_;
		int numThreads_;
		int next_;
		std::vector<std::unique_ptr<EventLoopThread>> threads_;
		std::vector<EventLoop*> loops_;

};
