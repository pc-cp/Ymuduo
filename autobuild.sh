#!/bin/bash

set -e 

# 如果没有build目录，创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build 
fi

# 删除之前编译好的文件
rm -rf `pwd`/build/*

cd `pwd`/build &&
    cmake .. && 
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/ymuduo, so库拷贝到 /usr/lib, 用户使用头文件时加 ymuduo/XXX
if [ ! -d /usr/include/ymuduo ]; then
    mkdir /usr/include/ymuduo
fi

for header in `ls *.h`
do 
    cp $header /usr/include/ymuduo
done 

cp `pwd`/lib/libymuduo.so /usr/lib

ldconfig
