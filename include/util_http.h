#ifndef _UTIL_HTTP_H
#define _UTIL_HTTP_H
struct request {
    char REQUEST_METHOD[4];
    char SCRIPT_NAME[64];
    char SERVER_PROTOCOL[8];
    char SERVER_NAME[32];
    int SERVER_PORT;
    char QUERY_STRING[2048];
    char REMOTE_ADDR[16];
    char CONTENT_TYPE[16];
    char POST_DATA[2048];
    int CONTENT_LENGTH;
} request;

void parse_line(char *line, struct request *header);

void parse_request(char *http_request, struct request *cgi_request);

#endif