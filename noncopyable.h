#pragma once
// #pragma once 避免头文件重复包含

// noncopyable这个类,拷贝构造和赋值函数被delete掉
// 不允许拷贝和赋值,但是构造和析构是默认的
// 派生类也一样
/*
   noncopyable被继承后,派生类对象可以正常构造和析构
   但是无法进行赋值和拷贝操作
 */
class noncopyable{
	public:
		noncopyable(const noncopyable&)=delete;
		void operator=(const noncopyable&)=delete;
	protected:
		noncopyable()=default;
		~noncopyable()=default;
};
