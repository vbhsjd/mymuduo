#include"Poller.h"
#include"Channel.h"

Poller::Poller(EventLoop* loop)
	:ownerLoop_(loop)
{
}

bool Poller::hasChannel(Channel* channel) const{
	auto iter = channels_.find(channel->fd());		
	return iter != channels_.end() and iter->second == channel;	
}

