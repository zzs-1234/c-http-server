#!/bin/bash

# 设置编译器
CC=gcc

# 设置编译选项
CFLAGS="-I$PWD/ffmpeg/include -I$PWD/libevent/include"
LDFLAGS="-L$PWD/ffmpeg/lib \
         -L$PWD/libevent/lib
         -lavformat \
         -lavcodec \
         -lavutil \
         -levent \
         -lm \
         -lz \
         -lpthread"

# 源文件
SRC="flv.c"

# 输出文件
OUTPUT="flv_server"

# 编译
$CC -static $CFLAGS $SRC -o $OUTPUT $LDFLAGS

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "编译成功: $OUTPUT"
else
    echo "编译失败"
fi