#include<mymuduo/TcpServer.h>
#include<mymuduo/Logger.h>

#include<string>
#include<functional>

class EchoServer{
	public:
		EchoServer(EventLoop* loop,const InetAddress& addr,const std::string& name):
			loop_(loop),server_(loop,addr,name)
	{
		// 注册回调函数
		server_.setConnectionCallback(
				std::bind(&EchoServer::onConnection,this,std::placeholders::_1));
		server_.setMessageCallback(
				std::bind(&EchoServer::onMessage,this,std::placeholders::_1,
					std::placeholders::_2,std::placeholders::_3));

		// 设置线程数量
		server_.setThreadNum(3);
	}

		void start(){
			server_.start();
		}
	private:
		// 用户自定义连接建立or断开的处理函数
		void onConnection(const TcpConnectionPtr& conn){
			if(conn->connected()){
				LOG_INFO("connection UP:%s\n",conn->peerAddress().toIpPort().c_str());
			}
			else {
				LOG_INFO("connection DOWN:%s\n",conn->peerAddress().toIpPort().c_str());
			}
		}
		// 用户自定义可读事件的处理函数
		void onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp time){
			std::string msg = buf->retrieveAllAsString();
			conn->send(msg); // 原封不动发送给客户
			conn->shutdown(); // 关闭写端 -> EPOLLHUP
		}
		EventLoop* loop_;
		TcpServer server_;
};

int main(){
	EventLoop loop;
	InetAddress addr(8000);

	EchoServer server(&loop,addr,"server01");
	server.start();
	loop.loop();

	return 0;
}
