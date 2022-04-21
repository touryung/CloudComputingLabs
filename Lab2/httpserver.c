#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "threadpool.h"
#include "tools.h"

// 指定界面
// socket 状态码 短语 内容
void send_error(int cfd, int status, char *title, char *text) {
  char buf[1024] = {0};
  char buf2[1024] = {0};

  // 界面
  sprintf(buf2, "<html><head><title>%d %s</title></head>\n", status, title);
  sprintf(buf2 + strlen(buf2),
          "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", status,
          title);
  sprintf(buf2 + strlen(buf2), "%s\n", text);
  sprintf(buf2 + strlen(buf2), "<hr>\n</body>\n</html>\n");

  // 头部
  sprintf(buf, "%s %d %s\r\n", "HTTP/1.1", status, title);
  sprintf(buf + strlen(buf), "Content-Type:%s\r\n", "text/html");
  sprintf(buf + strlen(buf), "Content-Length:%ld\r\n",
          strlen(buf2));  // 这里需要获取html的长度

  send(cfd, buf, strlen(buf), 0);
  send(cfd, "\r\n", 2, 0);
  send(cfd, buf2, strlen(buf2), 0);

  return;
}

// 发送响应头
void send_respond_head(int cfd, int no, const char *desp, const char *type,
                       long len) {
  char buf[1024] = {0};

  // 状态行
  sprintf(buf, "HTTP/1.1 %d %s\r\n", no, desp);
  send(cfd, buf, strlen(buf), 0);

  // 消息报头
  sprintf(buf, "Content-Type:%s\r\n", type);
  sprintf(buf + strlen(buf), "Content-Length:%ld\r\n", len);

  send(cfd, buf, strlen(buf), 0);
  // 空行
  send(cfd, "\r\n", 2, 0);
}

// 发送文件
void send_file(int cfd, const char *filename) {
  // 打开文件
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    send_error(cfd, 404, "Not Found", "NO such file or direntry");
    exit(1);
  }

  // 循环读文件
  char buf[1024] = {0};
  int len = 0, ret = 0;
  while ((len = read(fd, buf, sizeof(buf))) > 0) {
    // 发送读出的数据
    ret = send(cfd, buf, len, 0);
    if (ret == -1) {
      if (errno == EAGAIN) {
        perror("send error:");
        continue;
      } else if (errno == EINTR) {
        perror("send error:");
        continue;
      } else {
        perror("send error:");
        close(fd);
        exit(1);
      }
    }
  }
  if (len == -1) {
    perror("read file error");
    close(fd);
    exit(1);
  }

  close(fd);
}

void http_request(const char *request, int cfd) {
  // 拆分http请求行
  char method[12], path[1024], protocol[12];
  sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol);

  char *file = path + 1;  // 去掉path中的 / 获取访问文件名
  // 获取文件属性
  if (strlen(file) == 0) {
    file = "static/index.html";
  }

  struct stat st;
  int ret = stat(file, &st);
  if (ret == -1) {
    send_error(cfd, 404, "Not Found", "NO such file or direntry");
    return;
  }

  if (S_ISREG(st.st_mode)) {  // 是一个文件
    // 发送消息报头
    send_respond_head(cfd, 200, "OK", get_file_type(file), st.st_size);
    // 发送文件内容
    send_file(cfd, file);
  }
}

