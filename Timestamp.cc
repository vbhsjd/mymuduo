#include"Timestamp.h"

Timestamp::Timestamp():microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

Timestamp Timestamp::now(){
  return Timestamp(time(NULL));
}
/*
   snprintf虽然可以避免缓冲区溢出问题,但是在格式化字符串时仍需要保证缓冲区的	大小足够.在这里,如果时间字符串超过128字节,就会出现问题.
   不过这里字符串不保证超过128字节.
 */
std::string Timestamp::toString() const{
	char buf[128];
	tm* tm_time = localtime(&microSecondsSinceEpoch_);
	snprintf(buf,128,"%4d/%02d/%02d %02d:%02d:%02d",
			tm_time->tm_year + 1900,
			tm_time->tm_mon + 1,
			tm_time->tm_mday,
			tm_time->tm_hour,
			tm_time->tm_min,
			tm_time->tm_sec);
	return buf;
}
