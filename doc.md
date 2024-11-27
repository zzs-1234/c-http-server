## ffmpeg 编译

```bash
#! /bin/bash
./configure \
    --disable-everything \
    --enable-avcodec \
    --enable-avformat \
    --enable-demuxer=mp4 \
    --enable-muxer=mp4 \
    --enable-parser=aac,h264 \
    --enable-decoder=aac,h264 \
    --enable-encoder=aac,h264 \
    --enable-protocol=file \
    --enable-shared \
    --disable-debug \
    --prefix=$(pwd)/../dist
make -j4
make install

```
