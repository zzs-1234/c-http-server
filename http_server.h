#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

// 初始化服务器
int http_server_init(int port);

// 清理服务器资源
void http_server_deinit();

// 发送 HTTP 请求
void send_request(const char *method, const char *path, const char *data);

#ifdef __cplusplus
}
#endif

#endif // HTTP_SERVER_H 