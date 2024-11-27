#!/bin/bash

# 设置编译器
CC=gcc

# 设置编译选项
CFLAGS="-I$PWD/librtmp/include \
        -I$PWD/libevent/include \
        -I$PWD/ffmpeg/include"
LDFLAGS="-L$PWD/librtmp/lib \
         -L$PWD/libevent/lib \
         -L$PWD/ffmpeg/lib \
         -lrtmp \
         -lavformat \
         -lavcodec \
         -lavutil \
         -levent \
         -lswresample \
         -lx264 \
         -lm \
         -lz \
         -ldl \
         -lpthread"
# CFLAGS="-I$PWD/ffmpeg/include -I$PWD/libevent/include"
# LDFLAGS="-L$PWD/ffmpeg/lib \
#          -L$PWD/libevent/lib
#          -lavformat \
#          -lavcodec \
#          -lavutil \
#          -levent \
#          -lm \
#          -lz \
#          -lpthread"

case $1 in
    fg_demo)
        SRC="ffmpeg_demo.c"
        OUTPUT="ffmpeg_demo"
        ;;
    flv)
        SRC="flv.c"
        OUTPUT="flv_server"
        ;;
esac

# 编译
$CC -static $CFLAGS $SRC -o $OUTPUT $LDFLAGS

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "编译成功: $OUTPUT"
else
    echo "编译失败"
fi