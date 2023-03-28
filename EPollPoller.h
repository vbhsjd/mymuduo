#pragma once

#include"Poller.h"
#include"Timestamp.h"

#include<vector>
#include<sys/epoll.h>

class Channel;
/*
   这是一个epoll实现的多路事件分发器.其内部实现依赖了epoll相关的系统调用(epoll_create,epoll_ctl,epoll_wait)和std::vector

   poll函数等待事件发生,并且返回活跃的Channel列表以及时间戳.
   updateChannel函数讲一个Channel对应的文件描述符和关注的事件添加到epoll机制中.
   removeChannel将一个Channel对应的文件描述符和关注的事件从epoll机制中删除.
 */

class EPollPoller:public Poller{
	public:
		EPollPoller(EventLoop* loop);
		~EPollPoller() override;

		// 重写基类的抽象方法
		Timestamp poll(int timeoutMs,ChannelList* activeChannels) override;
		void updateChannel(Channel* channel) override;
		void removeChannel(Channel* channel) override;
	private:
		static const int kInitEventListSize = 16; // 初始化events_的大小
		// 填写活跃的连接
		void fillActiveChannels(int numEvents,
				ChannelList* activeChannels) const;
		// 将一个Channel对应的文件描述符的关注的事件添加或从epoll中修改或删除
		void update(int operation,Channel* channel);

		using EventList = std::vector<epoll_event>;

		int epollfd_;
		EventList events_; // epoll_event集合
};
