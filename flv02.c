#include <event2/event.h>
#include <librtmp/rtmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define RING_BUFFER_SIZE 8192

// 环形缓冲区结构
typedef struct {
    char data[RING_BUFFER_SIZE];
    size_t head;
    size_t tail;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} RingBuffer;

RingBuffer ring_buffer = {
    .head = 0,
    .tail = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

// 向环形缓冲区写入数据
void write_to_ring_buffer(const char *data, size_t size) {
    pthread_mutex_lock(&ring_buffer.lock);
    for (size_t i = 0; i < size; ++i) {
        ring_buffer.data[ring_buffer.tail] = data[i];
        ring_buffer.tail = (ring_buffer.tail + 1) % RING_BUFFER_SIZE;
        if (ring_buffer.tail == ring_buffer.head) {
            // 缓冲区满，覆盖旧数据
            ring_buffer.head = (ring_buffer.head + 1) % RING_BUFFER_SIZE;
        }
    }
    pthread_cond_signal(&ring_buffer.cond);
    pthread_mutex_unlock(&ring_buffer.lock);
}

// 从环形缓冲区读取数据
size_t read_from_ring_buffer(char *buffer, size_t size) {
    pthread_mutex_lock(&ring_buffer.lock);
    while (ring_buffer.head == ring_buffer.tail) {
        // 缓冲区为空，等待数据
        pthread_cond_wait(&ring_buffer.cond, &ring_buffer.lock);
    }
    size_t bytes_read = 0;
    while (bytes_read < size && ring_buffer.head != ring_buffer.tail) {
        buffer[bytes_read++] = ring_buffer.data[ring_buffer.head];
        ring_buffer.head = (ring_buffer.head + 1) % RING_BUFFER_SIZE;
    }
    pthread_mutex_unlock(&ring_buffer.lock);
    return bytes_read;
}

// RTMP 接收回调函数
void rtmp_receive_callback(RTMP *rtmp) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = RTMP_Read(rtmp, buffer, BUFFER_SIZE)) > 0) {
        write_to_ring_buffer(buffer, bytes_read);
    }

    if (bytes_read < 0) {
        fprintf(stderr, "Error reading RTMP stream\n");
    }
}

// HTTP-FLV 推流回调函数
void http_flv_stream_callback(evutil_socket_t fd, short event, void *arg) {
    char buffer[BUFFER_SIZE];
    size_t bytes_to_send = read_from_ring_buffer(buffer, BUFFER_SIZE);
    if (bytes_to_send > 0) {
        send(fd, buffer, bytes_to_send, 0);
    }
}

// 创建并绑定 HTTP 服务器 socket
evutil_socket_t create_http_server_socket(int port) {
    evutil_socket_t fd;
    struct sockaddr_in server_addr;

    // 创建 socket
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    // 设置 socket 选项
    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    // 初始化服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // 绑定 socket
    if (bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    // 监听 socket
    if (listen(fd, 10) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

int main() {
    // 初始化 libevent
    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }

    // 初始化 RTMP
    RTMP *rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);
    RTMP_SetupURL(rtmp, "rtmp://127.0.0.1:1935/flv");
    RTMP_EnableWrite(rtmp);

    if (!RTMP_Connect(rtmp, NULL)) {
        fprintf(stderr, "RTMP connect failed!\n");
        return 1;
    }

    // 启动 RTMP 接收线程
    pthread_t rtmp_thread;
    pthread_create(&rtmp_thread, NULL, (void *(*)(void *))rtmp_receive_callback, rtmp);

    // 创建 HTTP-FLV 服务器事件
    struct event *http_event;
    evutil_socket_t http_fd = create_http_server_socket(8080);
    if (http_fd < 0) {
        fprintf(stderr, "Could not create HTTP server socket!\n");
        return 1;
    }
    http_event = event_new(base, http_fd, EV_READ | EV_PERSIST, http_flv_stream_callback, NULL);
    event_add(http_event, NULL);

    // 进入事件循环
    event_base_dispatch(base);

    // 清理
    RTMP_Close(rtmp);
    RTMP_Free(rtmp);
    event_free(http_event);
    event_base_free(base);
    close(http_fd);

    return 0;
}
