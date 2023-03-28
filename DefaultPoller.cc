#include "Poller.h"
#include "EPollPoller.h"

#include<stdlib.h>

/*
   根据环境变量选择使用哪种Poller对象实例.
   该函数提供了一定的灵活性,可以再运行时动态选择不同的Poller实现.
 */

Poller* Poller::newDefaultPoller(EventLoop* loop){
	if(::getenv("MUDUO_USE_POLL")){
		return nullptr; // 生成poll的对象
	} else {
		return new EPollPoller(loop); // epoll实例
	}
}
