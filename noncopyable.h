#pragma once
// #pragma once 避免头文件重复包含,确保头文件只被包含一次
// 它不是c++标准规范的一部分,但是gcc支持

/*
   该类的作用是禁止拷贝构造函数和赋值操作符,从而避免被拷贝.
   这个类的实现是基于C++11中的delete关键字,可以确保对象不会被拷贝.
   同类,还使用了C++11中的default关键字,用于生成默认的构造函数和析构函数.
   该类的使用方法是在需要禁止拷贝的类私有继承noncopyable,从而确保不会被拷贝.
 */
class noncopyable{
	public:
		noncopyable(const noncopyable&)=delete;
		void operator=(const noncopyable&)=delete;
	protected:
		noncopyable()=default;
		~noncopyable()=default;
};
