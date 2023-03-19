#pragma once

#include"noncopyable.h"
#include"Timestamp.h"

#include<vector>
#include<unordered_map>

class Channel;
class EventLoop;
// muduo库中多路事件分发器的核心,IO复用模块
class Poller{
	public:
		using ChannelList = std::vector<Channel*>;

		Poller(EventLoop* loop);
		virtual ~Poller() = default;

		// 给select,poll,epoll保留统一的接口
		virtual Timestamp poll(int timeoutMs,ChannelList* activeChannels)
			= 0;
		virtual void updateChannel(Channel* channel) = 0;
		virtual void removeChannel(Channel* channel) = 0;

		//?? 判断channel是否在当前poller当中
		// poller使用unordered_map存的channel
		// 查找一下就可以
		bool hasChannel(Channel* channel) const;
		
		// eventLoop可以通过这个接口获取默认的Poller的对象
		static Poller* newDefaultPoller(EventLoop* loop);

	protected:
		// map的key:sockfd,value:sockfd对应的channel
		using ChannelMap = std::unordered_map<int,Channel*>;
		ChannelMap channels_;
	private:
		EventLoop* ownerLoop_; // poller所属的eventloop
};
