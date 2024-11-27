import http.server
import socketserver

# 定义端口
PORT = 8080

# FLV 文件路径
FLV_FILE_PATH = 'input.mp4'

class FLVRequestHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/live1':
            self.send_response(200)
            self.send_header('Content-type', 'video/x-flv')
            self.end_headers()
            
            # 打开 FLV 文件并读取数据
            with open(FLV_FILE_PATH, 'rb') as flv_file:
                while True:
                    data = flv_file.read(1024)
                    if not data:
                        break
                    self.wfile.write(data)
        else:
            self.send_error(404, "File Not Found")

# 创建 HTTP 服务器
with socketserver.TCPServer(("", PORT), FLVRequestHandler) as httpd:
    print(f"Serving on port {PORT}")
    httpd.serve_forever()