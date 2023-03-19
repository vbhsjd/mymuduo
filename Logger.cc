#include"Logger.h"
#include"Timestamp.h"

#include<iostream>

// 获取日志类唯一的实例对象
Logger& Logger::instance(){
	static Logger logger;
	return logger;
}
// 设置日志级别
void Logger::setLogLevel(int level){
	logLevel_ = level;
}
// 写日志接口
// 格式:[级别] time : msg
void Logger::log(std::string msg){
	switch(logLevel_){
		case INFO:
			std::cout << "[INFO]";
			break;
		case ERROR:
			std::cout << "[INFO]";
			break;
		case FATAL:
			std::cout << "[INFO]";
			break;
		case DEBUG:
			std::cout << "[INFO]";
			break;
		default:
			break;
	}
	// 打印时间和msg -> 时间直接用muduo的timestamp
	std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}