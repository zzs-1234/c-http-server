#!/bin/bash

# 编译http_server.c
gcc -o http_server http_server.c

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "编译成功，生成的可执行文件为: http_server"
else
    echo "编译失败"
fi 