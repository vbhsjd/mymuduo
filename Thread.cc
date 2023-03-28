#include"Thread.h"
#include"CurrentThread.h"

#include<semaphore.h>

std::atomic_int Thread::numCreated_{0};

Thread::Thread(ThreadFunc func,const std::string& name)
	:started_(false),
	 joined_(false),
	 tid_(0),
	 func_(std::move(func)),
	 name_(name)
{
	setDefaultName();
}

Thread::~Thread(){
	if(started_ and !joined_){ // 开始运行and不是工作线程
		thread_->detach(); // 线程分离
	}
}

void Thread::start(){
	started_ = true;
	sem_t sem;
	sem_init(&sem,false,0);

	// 临时对象,开启线程
	thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
		// 获取当前线程id
		tid_ = CurrentThread::tid();

		sem_post(&sem);
		func_(); // 执行线程函数
	}));

	// 要等待子线程获取到线程的tid
	sem_wait(&sem);
}

// 把该线程join到当前线程上,使得当前线程等待该线程结束
void Thread::join(){
	joined_ = true;
	thread_->join();
}

void Thread::setDefaultName(){
	int num = ++numCreated_;
	if(name_.empty()){
		char buf[32] = {0};
		snprintf(buf,sizeof buf,"Thread%d",num);
		name_ = buf;
	}
}
