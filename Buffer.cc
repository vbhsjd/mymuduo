#include"Buffer.h"

#include<sys/uio.h>
#include<errno.h>
#include<unistd.h>

// 从fd上读取数据 LT模式
ssize_t Buffer::readFd(int fd,int* savedErrno){
	char extrabuf[65536]; // 栈上开辟的内存空间 64K
	struct iovec vec[2];
	const size_t writable = writableBytes(); // 可写缓冲区的大学
	vec[0].iov_base = begin() + writerIndex_; // 第一块缓冲区起始地址
	vec[0].iov_len = writable; // 当我们用readv从socket缓冲区读数据,首先会填充这个vec[0],也就是我们的buffer
	vec[1].iov_base = extrabuf; // 第二块缓冲区,如果buffer填满了,就会填到这里
	vec[1].iov_len = sizeof extrabuf; // 栈空间大小

	const int iovcnt = (writable < sizeof extrabuf)?2:1;
	const ssize_t n = readv(fd,vec,iovcnt);
	if(n < 0){
		*savedErrno = errno;  // 出错了
	}
	else if(n <= writable){ // 说明buffer空间够用
		writerIndex_ += n;
	} else { // buffer空间不够,extrabuf上也有数据,用append把这部分数据拷贝过来
		writerIndex_ = buffer_.size();
		append(extrabuf,n - writable);
	}
	return n;
}

ssize_t Buffer::writeFd(int fd,int* savedErrno){
	ssize_t n = ::write(fd,peek(),readableBytes());
	if(n < 0){
		*savedErrno = errno;
	}
	return n;
}
