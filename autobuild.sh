#!/bin/bash

set -e

if [ ! -d `pwd`/build ];then
	mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build && 
	cmake .. &&
	make

#回到根目录
cd ..

#把头文件拷贝到/usr/include/mymuduo so库到/usr/lib
#环境变量不用设置
if [ ! -d /usr/include/mymuduo ];then
	mkdir /usr/include/mymuduo
fi

for header in `ls *.h`
do
	cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib

ldconfig

