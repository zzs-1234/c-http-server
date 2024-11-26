#include "http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

static int server_socket = -1;

void handle_request(int client_socket) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char *method, *path, *version;
    
    // 读取客户端请求
    read(client_socket, buffer, BUFFER_SIZE);
    
    // 解析客户端请求头相关信息，并打印出来
    method = strtok(buffer, " ");
    path = strtok(NULL, " ");
    version = strtok(NULL, "\r\n");

    printf("请求方法: %s\n", method);
    printf("请求路径: %s\n", path);
    printf("HTTP版本: %s\n", version);

    if (strcmp(method, "GET") == 0) {
        // 处理 GET 请求
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "\r\n"
                 "<html><body><h1>Hello, World!</h1></body></html>\r\n");
    } else if (strcmp(method, "POST") == 0) {
        // 处理 POST 请求
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) {
            body += 4; // 跳过 "\r\n\r\n"
            printf("POST 数据: %s\n", body);
        }
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "\r\n"
                 "<html><body><h1>POST Received</h1></body></html>\r\n");
    } else {
        // 处理其他请求方法
        snprintf(response, sizeof(response),
                 "HTTP/1.1 405 Method Not Allowed\r\n"
                 "Content-Type: text/html\r\n"
                 "\r\n"
                 "<html><body><h1>Method Not Allowed</h1></body></html>\r\n");
    }

    // 发送响应
    write(client_socket, response, strlen(response));
    close(client_socket);
}

void send_request(const char *method, const char *path, const char *data) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char request[BUFFER_SIZE];

    // 创建套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("创建套接字失败");
        return;
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY; // 这里可以替换为服务器的 IP 地址

    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("连接失败");
        close(sock);
        return;
    }

    // 构建请求
    if (strcmp(method, "GET") == 0) {
        snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", path);
    } else if (strcmp(method, "POST") == 0) {
        snprintf(request, sizeof(request), "POST %s HTTP/1.1\r\nHost: localhost\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n%s", path, strlen(data), data);
    }

    // 发送请求
    send(sock, request, strlen(request), 0);

    // 接收响应
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // 添加字符串结束符
        printf("响应:\n%s\n", buffer);
    }

    close(sock);
}

int http_server_init(int port) {
    struct sockaddr_in server_addr;
    
    // 创建服务器套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("创建套接字失败");
        return -1;
    }
    
    // 设置套接字选项
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("设置套接字选项失败");
        return -1;
    }
    
    // 设置服务器地址结构
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // 绑定套接字
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("绑定失败");
        return -1;
    }
    
    // 监听连接
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("监听失败");
        return -1;
    }
    
    printf("HTTP 服务器正在运行，端口: %d\n", port);
    return 0;
}

void http_server_deinit() {
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
        printf("HTTP 服务器已关闭。\n");
    }
}

int main() {
    // 初始化服务器
    if (http_server_init(8080) != 0) {
        return 1;
    }

    // 发送 GET 请求示例
    send_request("GET", "/", NULL);
    
    // 发送 POST 请求示例
    send_request("POST", "/", "key=value");

    // 处理客户端请求
    fd_set readfds;
    int client_sockets[MAX_CLIENTS] = {0}; // 存储客户端套接字

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        int max_sd = server_socket;

        // 添加客户端套接字到集合
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // 等待活动
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select 错误");
        }

        // 检查服务器套接字是否有新连接
        if (FD_ISSET(server_socket, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
            if (client_socket < 0) {
                perror("接受连接失败");
                continue;
            }

            // 将新套接字添加到数组
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_socket;
                    break;
                }
            }
        }

        // 检查所有客户端套接字是否有数据
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                handle_request(sd);
                client_sockets[i] = 0; // 处理完后清除套接字
            }
        }
    }

    // 清理服务器资源
    http_server_deinit();
    return 0;
} 