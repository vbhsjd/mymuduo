#pragma once

#include<iostream>
#include<string>

/*
   microSecondsSinceEpoch_表示1970.1.1 0分0秒到现在的微秒数
   构造函数被显式声明为explicit,
   这意味着构造函数不会被用于隐式类型转换,只能显示调用.

 */
class Timestamp{
	public:
		Timestamp();
		explicit Timestamp(int64_t microSecondsSinceEpoch);
		static Timestamp now();
		std::string toString() const;
	private:
		int64_t microSecondsSinceEpoch_;
};
