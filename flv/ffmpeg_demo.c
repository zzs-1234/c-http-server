#include <libavformat/avformat.h>

int main(int argc, char *argv[]) {
    const char *input_file = "./input.mp4";
    AVFormatContext *fmt_ctx = avformat_alloc_context();
    int ret = -1;

    // 打开输入文件
    if ((ret = avformat_open_input(&fmt_ctx, input_file, NULL, NULL)) < 0) {
        printf("avformat_open_input file %s error %s\n", input_file, av_err2str(ret));
        return -1;
    }

    // 读取文件信息
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        printf("Could not find stream information\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    // 打印文件信息
    av_dump_format(fmt_ctx, 0, input_file, 0);

    // 释放资源
    avformat_close_input(&fmt_ctx);
    return 0;
}
