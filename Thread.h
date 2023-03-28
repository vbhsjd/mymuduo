#pragma once 

#include "noncopyable.h"

#include<functional>
#include<thread>
#include<memory>
#include<unistd.h>
#include<string>
#include<atomic>

/*
   Thread类是一个封装了C++11的std::thread类的自定义Thread类,用于开启线程.
   使用using定义了ThreadFunc类型,用于指定线程运行的函数.
   构造函数使用explicit关键字,避免隐式类型转换,需要传入一个ThreadFunc类型的参数,用于制定线程的函数,另外还可传入一个string类型参数,用于给线程指定一个名称.
   join函数用于等待线程结束,并将joined_设置为true.
   setDefault函数用于设置线程默认名称,即"Thread"+线程序号.

   通过定义ThreadFunc类型,该类可以接受任何函数指针,Lambda表达式以及可调用对象作为线程函数,这增加了代码的灵活性.
 */

class Thread : noncopyable{
	public:
		using ThreadFunc = std::function<void()>;

		explicit Thread(ThreadFunc,const std::string& name = std::string());
		~Thread();

		void start();
		void join();
		bool started() const { return started_; }
		pid_t tid() const { return tid_; }
		const std::string& name() const { return name_; }

		static int numCreated() { return numCreated_; }
	private:
		void setDefaultName();

		bool started_;
		bool joined_;
		std::shared_ptr<std::thread> thread_;
		pid_t tid_; // process id
		ThreadFunc func_; // 线程运行的函数
		std::string name_;

		static std::atomic_int numCreated_; // 线程序号
};
