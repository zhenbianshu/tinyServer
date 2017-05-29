#include "util_http.h"

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

char *deal_request(char *http_request, int client_fd);

char *create_json(struct request *cgi_request);

char *exec_php(char *args);

void epoll_register(int epoll_fd, int fd, int state);

void epoll_cancel(int epoll_fd, int fd, int state);

void accept_client(int server_fd, int epoll_fd);

void deal_client(int client_fd, int epoll_fd);
