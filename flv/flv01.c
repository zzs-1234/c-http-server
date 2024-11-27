#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#define PORT 8080

// 处理客户端连接
void on_client_connect(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ctx) {
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    // 发送 HTTP 响应头
    const char *response_header = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: video/x-flv\r\n"
                                  "Connection: keep-alive\r\n"
                                  "Transfer-Encoding: chunked\r\n"
                                  "\r\n";
    bufferevent_write(bev, response_header, strlen(response_header));

    // 初始化 FFmpeg
    av_register_all();
    AVFormatContext *fmt_ctx = avformat_alloc_context();
    
    // 打开 FLV 流
    if (avformat_open_input(&fmt_ctx, "your_flv_file.flv", NULL, NULL) < 0) {
        perror("avformat_open_input()");
        bufferevent_free(bev);
        return;
    }

    // 读取数据并发送给客户端
    AVPacket pkt;
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        // 发送数据包
        bufferevent_write(bev, pkt.data, pkt.size);
        av_packet_unref(&pkt);
    }

    // 关闭流
    avformat_close_input(&fmt_ctx);
    // 关闭连接
    bufferevent_free(bev);
}

// 主函数
int main(int argc, char **argv) {
    struct event_base *base;
    struct evconnlistener *listener;
    struct sockaddr_in sin;

    // 创建事件基础设施
    base = event_base_new();
    if (!base) {
        perror("event_base_new()");
        return 1;
    }

    // 设置监听地址和端口
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    // 创建监听器
    listener = evconnlistener_new_bind(base, on_client_connect, NULL,
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                                       (struct sockaddr*)&sin, sizeof(sin));

    if (!listener) {
        perror("evconnlistener_new_bind()");
        return 1;
    }

    // 进入事件循环
    event_base_dispatch(base);

    // 清理资源
    evconnlistener_free(listener);
    event_base_free(base);

    return 0;
}
