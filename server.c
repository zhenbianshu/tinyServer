#include "./include/server.h"
#include "./include/util_http.h"
#include "./include/cJSON.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

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
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

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
char *deal_request(char *request_content, int client_fd) {
    struct sockaddr_in client_addr;
    socklen_t len_client_addr = sizeof(client_addr);
    struct request cgi_request;

    parse_request(request_content, &cgi_request);
    cgi_request.SERVER_PORT = PORT;
    getpeername(client_fd, (struct sockaddr *) &client_addr, &len_client_addr);
    inet_ntop(AF_INET, &client_addr.sin_addr, cgi_request.REMOTE_ADDR, sizeof(cgi_request.REMOTE_ADDR));

    char *request_json = create_json(&cgi_request);
    char *response_json = exec_php(request_json);

    // todo parse json and build http_response
    return response_json;
}

char *create_json(struct request *cgi_request) {
    cJSON *root;
    root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "REQUEST_METHOD", cgi_request->REQUEST_METHOD);
    cJSON_AddStringToObject(root, "SCRIPT_NAME", cgi_request->SCRIPT_NAME);
    cJSON_AddStringToObject(root, "SERVER_PROTOCOL", cgi_request->SERVER_PROTOCOL);
    cJSON_AddStringToObject(root, "SERVER_NAME", cgi_request->SERVER_NAME);
    cJSON_AddStringToObject(root, "QUERY_STRING", cgi_request->QUERY_STRING);
    cJSON_AddStringToObject(root, "REMOTE_ADDR", cgi_request->REMOTE_ADDR);
    cJSON_AddStringToObject(root, "CONTENT_TYPE", cgi_request->CONTENT_TYPE);
    cJSON_AddStringToObject(root, "POST_DATA", cgi_request->POST_DATA);
    cJSON_AddNumberToObject(root, "SERVER_PORT", cgi_request->SERVER_PORT);
    cJSON_AddNumberToObject(root, "CONTENT_LENGTH", cgi_request->CONTENT_LENGTH);

    char *json = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);

    return json;
}

/*
 * todo define response struct and build response header
   void parse_json(char *response_json, struct response *cgi_response) {
    cJSON *json = cJSON_Parse(response_json);

    cgi_response.CONTENT_TYPE = cJSON_GetObjectItem(cJSON, "CONTENT_TYPE");
    cgi_response.HTTP_CODE = cJSON_GetObjectItem(cJSON, "HTTP_CODE");
    cgi_response.RESPONSE_CONTENT = cJSON_GetObjectItem(cJSON, "RESPONSE_CONTENT");

    return;
}
*/

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

    response_content = deal_request(http_request, client_fd);
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
    epoll_register(epoll_fd, server_fd, EPOLLIN | EPOLLET);// 这里注册socketEPOLL事件为ET模式

    while (1) {
        event_num = epoll_wait(epoll_fd, events, MAX_EVENTS, 0);
        for (i = 0; i < event_num; i++) {
            fd = events[i].data.fd;
            if ((fd == server_fd) && (events[i].events == EPOLLIN)) {
                accept_client(server_fd, epoll_fd);
            } else if (events[i].events == EPOLLIN) {
                deal_client(fd, epoll_fd);
            } else if (events[i].events == EPOLLOUT)
                // todo 数据过大，缓冲区不足的情况待处理
                continue;
        }
    }
}