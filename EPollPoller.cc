#include"EPollPoller.h"
#include"Logger.h"
#include"Channel.h"

#include<errno.h>
#include<cstring>
#include<unistd.h>

const int kNew = -1; // 表示channel从未添加进poller中
const int kAdded = 1; // channel在poller的channelMap中
const int kDeleted = 2; // channel不在channelMap中

EPollPoller::EPollPoller(EventLoop* loop)
	:Poller(loop),
	 epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
	 events_(kInitEventListSize) // epoll_event
{
	if(epollfd_ < 0){
		LOG_FATAL("epoll_create error:%d \n",errno);
	}
}

EPollPoller::~EPollPoller(){
	::close(epollfd_);
}

// 主要调用epoll_wait
Timestamp EPollPoller::poll(int timeoutMs,ChannelList* activeChannels){
	// 这里写日志对效率影响大,用LOG_DEBUG更加合理
	LOG_INFO("func=%s -> fd total count:%lu \n",__FUNCTION__,channels_.size());

	int numEvents = ::epoll_wait(epollfd_,
			&*events_.begin(),
			static_cast<int>(events_.size()),
			timeoutMs);

	int savedErrno = errno; // errno是全局的,所以save一下
	Timestamp now(Timestamp::now());

	if(numEvents > 0){
		LOG_INFO("%d events happened \n",numEvents);
		fillActiveChannels(numEvents,activeChannels);
		if(numEvents == events_.size()) { // 发生事件数=vector size
			events_.resize(2 * events_.size());
		}
	} else if(numEvents == 0){
		// nothing happened , timeout
		LOG_DEBUG("%s timeout. \n",__FUNCTION__);
	} else {
		if(savedErrno != EINTR) {// 中断
			errno = savedErrno;
			LOG_ERROR("EPollPoller::Poll() error! \n");
		}
	}
	return now;
}

/*
			   eventLoop
		ChannelList		poller
		存放所有channel channelMap  <fd,channel*>
 */
// 从poller中删除channel
void EPollPoller::removeChannel(Channel* channel){
	LOG_INFO("func=%s => fd=%d \n",__FUNCTION__,channel->fd());

	int fd = channel->fd();
	channels_.erase(fd);  // 从channelMap中删除

	int index = channel->index();
	if(index == kAdded) {
		update(EPOLL_CTL_DEL,channel);
	}
	channel->set_index(kNew); // kNew从来没有往poller从添加过
}

void EPollPoller::updateChannel(Channel* channel){
	const int index = channel->index();
	LOG_INFO("func=%s => fd=%d events=%d index=%d \n",__FUNCTION__,channel->fd(),channel->events(),index);

	if(index == kNew or index == kDeleted){
		if(index == kNew){
			// 添加到poller的channelMap里面
			int fd = channel->fd();
			channels_[fd] = channel;
		} else { // kDeleted
			// do nothing
		}
		channel->set_index(kAdded);
		update(EPOLL_CTL_ADD,channel);
	} else {	// index == kAdded
		// 表示channel已经在poller上注册过了
		int fd = channel->fd();
		if(channel->isNoneEvent()){
			update(EPOLL_CTL_DEL,channel);
			channel->set_index(kDeleted);
		} else {
			update(EPOLL_CTL_MOD,channel);
		}
	}
}

void EPollPoller::fillActiveChannels(int numEvents,ChannelList* activeChannels) const {
	for(int i = 0;i < numEvents;++i){
		// events.data.ptr是void*,所以强转一下
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
		channel->set_revents(events_[i].events);
		activeChannels->push_back(channel);
	} // 这样eventloop就拿到了poller返回的所有发生事件的channel列表
}

// epoll_ctl具体操作
void EPollPoller::update(int operation,Channel* channel) {
	epoll_event event;
	memset(&event,0,sizeof(event));
	int fd = channel->fd();

	event.events = channel->events();
	event.data.fd = fd;
	event.data.ptr = channel;

	if(::epoll_ctl(epollfd_,operation,fd,&event) < 0){
		if(operation == EPOLL_CTL_DEL){
			LOG_ERROR("epoll_ctl_del error:%d \n",errno);
		} else {
			LOG_FATAL("epoll_ctl add/mod error:%d \n",errno);
		}
	}
}
