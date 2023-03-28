#pragma once

#include<vector>
#include<string>
#include<algorithm>

/*
   append函数:把长度为len的data写入缓冲区
   retrieveAsString函数:获取缓冲区上长度为len的数据,并以string返回
   begin函数:返回缓冲区的起始地址
   peek函数:返回可读数据的起始地址
   ensureWritableBytes函数:当打算想缓冲区写数据,调用append函数时,会调用这个函数,它会检查你的缓冲区可写区域free1+free2能不能装下len长度的数据,如果不能牛么就动态扩容,调用makeSpace函数
readFd函数:客户端发来数据,readFd从TCP接收缓冲区中将数据读出来并放到Buffer中
writeFd函数:服务端要向这条TCP连接发送数据,通过这个函数把Buufer中的数据拷贝到TCP发送缓冲区中.
 */
class Buffer{
	public:
		static const size_t kCheapPrepend = 8; // 8字节空间,用于记录数据长度
		static const size_t kInitialSize = 1024; // 缓冲区大小

		explicit Buffer(size_t initialSize = kInitialSize)
			:buffer_(kCheapPrepend + initialSize),
			 readerIndex_(kCheapPrepend),
			 writerIndex_(kCheapPrepend)
		{}
		~Buffer() {} // buffer_由vector自己析构
		
		// 缓冲区中可读数据的长度
		size_t readableBytes() const { return writerIndex_ - readerIndex_; }
		// 缓冲区中可写区域free区域的长度
		size_t writableBytes() const { return buffer_.size() - writerIndex_; }
		// free1长度 + kCheapPrepend
		size_t prependableBytes() const { return readerIndex_; }

		// peek 窥视 返回可读数据的起始地址
		const char* peek() const { return begin() + readerIndex_; }

		// 调整readerIndex_的位置
		void retrieve(size_t len){
			if(len < readableBytes()) { 
				readerIndex_ += len;
			} else {
				// 所有数据都读了,把readerIndex_和writerIndex_复位
				retrieveAll();
			}
		}
		
		// 把readerIndex_和writerIndex_复位
		void retrieveAll(){
			readerIndex_ = kCheapPrepend;
			writerIndex_ = kCheapPrepend;
		}
		
		// 读取len长度的数据
		std::string retrieveAsString(size_t len){
			std::string result(peek(),len);
			retrieve(len); // len长度数据已读,调整readerIndex_位置
			return result;
		}

		// 读取所有可读的数据
		std::string retrieveAllAsString(){
			return retrieveAsString(readableBytes());
		}
		
		void ensureWritableBytes(size_t len){
			// 这样可能需要扩容 free2 < len
			if(writableBytes() < len) {
				makeSpace(len);				
			}
		}

		// 写入数据
		void append(const char* data,size_t len){
			ensureWritableBytes(len);
			std::copy(data,data+len,beginWrite()); // 写入数据
			writerIndex_ += len; // 调整writerIndex_
		}

		// free区域的起始位置
		char* beginWrite() {
			return begin() + writerIndex_;
		}

		const char* beginWrite() const {
			return begin() + writerIndex_;
		}
		
		// 从fd上读取数据
		ssize_t readFd(int fd,int* savedErrno);
		ssize_t writeFd(int fd,int* savedErrno);
	private:
		// 给缓冲区扩容 or 移位
		void makeSpace(size_t len){
			// free2 + free1 + 8 < len + 8
			// 长度不够,说明要扩容
			if(writableBytes() + prependableBytes() < len + kCheapPrepend){
				buffer_.resize(writerIndex_ + len);
			} else {
				// 长度够,把可读数据移到free1的位置
				size_t readable = readableBytes();
				std::copy(begin()+readerIndex_,
						  begin()+writerIndex_,
						  begin()+kCheapPrepend);
				readerIndex_ = kCheapPrepend;
				writerIndex_ = readerIndex_ + readable;
			}
		}

		// 返回缓冲区的起始地址
		char* begin() { return &*buffer_.begin(); }
		const char* begin() const { return &*buffer_.begin(); }
	
		std::vector<char> buffer_;
		size_t readerIndex_;
		size_t writerIndex_;
 };
