#include "Poller.h"
#include "EPollPoller.h"

#include<stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop* loop){
	if(::getenv("MUDUO_USE_POLL")){
		return nullptr; // 生成poll的对象
	} else {
		return new EPollPoller(loop); // epoll实例
	}
}
