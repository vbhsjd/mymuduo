#pragma once

#include<arpa/inet.h>
#include<netinet/in.h>
#include<string>

/*
   InetAddr类用于封装IP地址和端口号.

   toIp函数返回IP地址的字符串表示
   toPort函数返回端口号
 */
class InetAddress{
	public:
		explicit InetAddress(uint16_t port = 0,
				std::string ip = "127.0.0.1");
		explicit InetAddress(const sockaddr_in& addr)
			:addr_(addr) {}

		std::string toIp() const;
		std::string toIpPort() const;
		uint16_t toPort() const;

		const sockaddr_in* getSockAddr() const { return &addr_; }
		void setSockAddr(const sockaddr_in& addr) { addr_ = addr; }
	private:
		sockaddr_in addr_;
};
