#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include<functional>
#include<memory>
// 前置声明
class EventLoop;

/*
   channel封装了sockfd以及其感兴趣的事件(listenfd和clntfd都有)
   事件就是EPOLLIN,EPOLLOUT等
   还绑定了poller(epoll)返回的具体事件
 */

class Channel : noncopyable
{
	public:
		using EventCallback = std::function<void()>;
		using ReadEventCallback = std::function<void(Timestamp)>;
		
		Channel(EventLoop* loop,int fd);
		~Channel();

		// 处理事件
		// fd得到poller通知后,调用对应的回调函数
		void handleEvent(Timestamp receiveTime);
		
		// 设置回调函数对象
		void setReadCallback(ReadEventCallback cb){
			readCallback_ = std::move(cb);
		}
		void setWriteCallback(EventCallback cb){
			writeCallback_ = std::move(cb);
		}
		void setCloseCallback(EventCallback cb){
			closeCallback_ = std::move(cb);
		}
		void setErrorCallback(EventCallback cb){
			errorCallback_ = std::move(cb);
		}

		// 防止channel被手动remove掉,channel还在执行回调操作
		// ??tie有什么用
		void tie(const std::shared_ptr<void>&);

		int fd() const { return fd_; }
		int events() const { return events_; }
		void set_revents(int revt) { revents_  = revt; }
		
		// enable表达的是fd对读事件感兴趣,设置fd相应的事件状态
		// 这样写方便调用epoll_ctl,update就是epoll_ctl
		// update通知poller把这个fd感兴趣的事件添加进去
		void enableReading() { events_ |= kReadEvent; update(); }
		void disableReading() { events_ &= ~kReadEvent; update(); }
		void enableWriting() { events_ |= kWriteEvent; update(); }
		void disableWriting() { events_ &= ~kWriteEvent; update(); }
		void disableAll() { events_ = kNoneEvent; update(); }
		
		// 返回fd当前的事件的状态	
		bool isNoneEvent() const { return events_ == kNoneEvent; }
		bool isWriting() const { return events_ & kWriteEvent; }
		bool isReading() const { return events_ & kReadEvent; }

		int index() const { return index_; }
		void set_index(int idx) { index_ = idx; }

		EventLoop* ownerLoop() { return loop_; }
		// 在channel所属的eventloop删除掉该channel
		void remove();
	private:
		void update();
		void handleEventWithGuard(Timestamp receiveTime);

		static const int kNoneEvent;
		static const int kReadEvent;
		static const int kWriteEvent;

		EventLoop* loop_; // 事件循环
		const int fd_; // fd,poller监听的对象
		int events_; // 注册fd感兴趣的事件
		int revents_; // poller返回的具体发生的事件
		int index_; // 用于poller,表示channel未添加or已添加or已被删除

		std::weak_ptr<void> tie_;
		bool tied_;
		
		// 因为channel里面能够获得fd发生的revents
		// 所以他负责调用具体事件的回调操作
		ReadEventCallback readCallback_;
		EventCallback writeCallback_;
		EventCallback closeCallback_;
		EventCallback errorCallback_;
};
