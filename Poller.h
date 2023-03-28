#pragma once

#include"noncopyable.h"
#include"Timestamp.h"

#include<vector>
#include<unordered_map>

class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心,IO复用模块
/*
   Poller是个抽象基类,用于实现多路复用分发器,它定义了一些虚函数,需要派生了实现.
   poll函数是实现IO复用的函数,用于等待事件的发生.timeoutMs表示等待的时间,单位是毫秒,activeChannels是输出参数,表示活跃的channel,需要在这个vector中返回活跃的Channel.
   updateChannel函数更新Channel的关注事件,将Channel对应的文件描述符及其关注的事件加入到IO复用的数据结构中.
	removeChannel函数移除Channel对应的文件描述符及其关注的事件.
	
	ChannelMap成员变量用于维护文件描述符与Channel对象的映射关系.
	Poller的派生类需要实现这些纯虚函数,已完成具体的IO复用机制.在Poller中提供了一个静态函数newDefaultPoller,用于获取默认的多路事件分发器.
 */

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
		ChannelMap channels_; // 文件描述符->channel的映射,channels_负责保管所有注册在这个poller上的channel
	private:
		EventLoop* ownerLoop_; // poller所属的eventloop
};
