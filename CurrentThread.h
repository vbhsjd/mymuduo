#pragma once

#include<unistd.h>
#include<sys/syscall.h>

/*
   这是一个获取当前线程ID的工具类,使用线程本地存储(Thread local storage,TLS)来缓存当前线程ID.
 */

namespace CurrentThread{
	extern __thread int t_cachedTid;

	void catchTid();

	inline int tid(){
		if(__builtin_expect(t_cachedTid == 0,0)){
			catchTid();
		}
		return t_cachedTid;
	}
}
