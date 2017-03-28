#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define PORT 8080
#define HOST "127.0.0.1"
#define BUFF_SIZE 1024
#define REQUEST_QUEUE_LENGTH 10
#define FD_SIZE 1024
#define MAX_EVENTS 256

char *header_tmpl = "HTTP/1.1 200 OK\r\n"
        "Server: ZBS's Server V1.0\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Content-Type: text/html\r\n\r\n";

int server_start();

char *deal_request(char *http_request);

char *exec_php(char *args);

void epoll_register(int epoll_fd, int fd, int state);

void epoll_cancel(int epoll_fd, int fd, int state);

void accept_client(int server_fd, int epoll_fd);

void deal_client(int client_fd, int epoll_fd);

/**
 * 创建一个server socket
 */
int server_start() {
    int sock_fd;
    struct sockaddr_in server_addr;
    int flags;

    server_addr.sin_family = AF_INET; // 协议族是IPV4协议
    server_addr.sin_port = htons(PORT); // 将主机的无符号短整形数转换成网络字节顺序
    server_addr.sin_addr.s_addr = inet_addr(HOST); // 用inet_addr函数将字符串host转为int型

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 获取服务器socket的设置，并添加"不阻塞"选项
    flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags|O_NONBLOCK);

    if (bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind_error");
        exit(1);
    }

    if (listen(sock_fd, REQUEST_QUEUE_LENGTH) == -1) {
        perror("listen error");
        exit(1);
    }

    return (sock_fd);
}

/**
 * 处理请求参数
 *
 * @param request_content
 */
char *deal_request(char *request_content) {
    static char *result;
    char *first_line = strtok(request_content, "\n"); // 获取第一行 结构类似 "GET /path VERSION"
    char *method = strtok(first_line, " ");  // 获取到请求方法
    if (strcmp(method, "GET") != 0) {
        result = "只支持HTTP GET 方法！";
    } else {
        char *param = strtok(NULL, " ");  // 获取到参数
        result = exec_php(param);
    }

    return result;
}

/**
 * 执行PHP脚本以返回执行结果
 *
 * @param args
 * @return
 */
char *exec_php(char *args) {
    // 这里不能用变长数组，需要给command留下足够长的空间，以存储args参数，不然拼接参数时会栈溢出
    char command[BUFF_SIZE] = "php /tmp/index.php ";
    FILE *fp;
    static char buff[BUFF_SIZE]; // 声明静态变量以返回变量指针地址
    char line[BUFF_SIZE];
    strcat(command, args);
    memset(buff, 0, BUFF_SIZE); // 静态变量会一直保留，这里初始化一下
    if ((fp = popen(command, "r")) == NULL) {
        strcpy(buff, "服务器内部错误");
    } else {
        // fgets会在获取到换行时停止，这里将每一行拼接起来
        while (fgets(line, BUFF_SIZE, fp) != NULL) {
            strcat(buff, line);
        };
    }

    return buff;
}

/**
 * 注册epoll事件
 *
 * @param epoll_fd  epoll句柄
 * @param fd        socket句柄
 * @param state    监听状态
 */
void epoll_register(int epoll_fd, int fd, int state) {
    struct epoll_event event;
    event.events = state;
    event.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

/**
 * 注销epoll事件
 *
 * @param epoll_fd  epoll句柄
 * @param fd        socket句柄
 * @param state    监听状态
 */
void epoll_cancel(int epoll_fd, int fd, int state) {
    struct epoll_event event;
    event.events = state;
    event.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
}

/**
 * 受理客户端连接
 *
 * @param server_fd
 * @param epoll_fd
 */
void accept_client(int server_fd, int epoll_fd) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t len_client_addr = sizeof(client_addr);

    client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &len_client_addr);
    epoll_register(epoll_fd, client_fd, EPOLLIN);
}

/**
 * 处理客户端的请求信息
 *
 * @param client_fd
 * @param epoll_fd
 */
void deal_client(int client_fd, int epoll_fd) {
    char *response_content;
    char http_request[BUFF_SIZE], response_header[BUFF_SIZE], http_response[BUFF_SIZE];
    memset(http_request, 0, BUFF_SIZE); // 清空缓冲区内容
    recv(client_fd, http_request, BUFF_SIZE, 0);

    // 如果收到请求内容为空的，表示客户端正常断开的连接
    if (strlen(http_request) == 0) {
        epoll_cancel(epoll_fd, client_fd, EPOLLIN);
        return;
    }

    response_content = deal_request(http_request);
    sprintf(response_header, header_tmpl, strlen(response_content));
    sprintf(http_response, "%s%s", response_header, response_content);
    send(client_fd, http_response, sizeof(http_response), 0);
}

int main() {
    int i;
    int event_num;
    int server_fd;
    int epoll_fd;
    int fd;
    struct epoll_event events[MAX_EVENTS];

    server_fd = server_start();
    epoll_fd = epoll_create(FD_SIZE);
    epoll_register(epoll_fd, server_fd, EPOLLIN|EPOLLET);// 这里注册socketEPOLL事件为ET模式

    while (1) {
        event_num = epoll_wait(epoll_fd, events, MAX_EVENTS, 0);
        for (i = 0; i < event_num; i++) {
            fd = events[i].data.fd;
            if ((fd == server_fd) && (events[i].events == EPOLLIN)){
                accept_client(server_fd, epoll_fd);
            } else if (events[i].events == EPOLLIN){
                deal_client(fd, epoll_fd);
            } else if (events[i].events == EPOLLOUT)
                // todo 数据过大，缓冲区不足的情况待处理
                continue;
        }
    }
}