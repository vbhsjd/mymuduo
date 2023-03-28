#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include<functional>
#include<memory>

// 前置声明
class EventLoop;

/*
   channel封装了sockfd以及其感兴趣的事件
   只关注通信通道相关的事件处理和状态维护,将对应的事件处理回调交给其他对象去实现.
   
   类的成员变量使用下划线后缀命名法,这样可以更好地区分成员变量和局部变量.
 */

class Channel : noncopyable
{
	public:
		// 回调函数类型定义
		using EventCallback = std::function<void()>;
		using ReadEventCallback = std::function<void(Timestamp)>;
		
		// 构造函数和析构函数
		Channel(EventLoop* loop,int fd);
		~Channel();
		
		// 设置回调函数对象
		void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
		void setWriteCallback(EventCallback cb){ writeCallback_ = std::move(cb); }
		void setCloseCallback(EventCallback cb){ closeCallback_ = std::move(cb); }
		void setErrorCallback(EventCallback cb){ errorCallback_ = std::move(cb); }

		void tie(const std::shared_ptr<void>&);

		// 获取fd和events的值
		int fd() const { return fd_; }
		int events() const { return events_; }
		void set_revents(int revt) { revents_  = revt; } // used by poller
		
		// 设置感兴趣的事件状态
		void enableReading() { events_ |= kReadEvent; update(); }
		void disableReading() { events_ &= ~kReadEvent; update(); }
		void enableWriting() { events_ |= kWriteEvent; update(); }
		void disableWriting() { events_ &= ~kWriteEvent; update(); }
		void disableAll() { events_ = kNoneEvent; update(); }
		
		// 返回fd当前的事件的状态	
		bool isNoneEvent() const { return events_ == kNoneEvent; }
		bool isWriting() const { return events_ & kWriteEvent; }
		bool isReading() const { return events_ & kReadEvent; }

		// 获取/设置 index_ 值
		int index() const { return index_; }
		void set_index(int idx) { index_ = idx; }

		// 获取EventLoop对象指针
		// for poller
		EventLoop* ownerLoop() { return loop_; }

		// 在channel所属的eventloop删除掉该channel
		void remove();

		// 处理事件
		// fd得到poller通知后,调用对应的回调函数
		void handleEvent(Timestamp receiveTime);
	private:
		// 更新感兴趣的事件
		void update();
		// 处理事件(包含安全检查)
		void handleEventWithGuard(Timestamp receiveTime);

		static const int kNoneEvent;
		static const int kReadEvent;
		static const int kWriteEvent;

		EventLoop* loop_; // 事件循环
		const int fd_; // fd,poller监听的对象
		int events_; // 注册fd感兴趣的事件
		int revents_; // poller返回的具体发生的事件
		int index_; // used by poller 
		std::weak_ptr<void> tie_;
		bool tied_;
		
		ReadEventCallback readCallback_;
		EventCallback writeCallback_;
		EventCallback closeCallback_;
		EventCallback errorCallback_;
};
