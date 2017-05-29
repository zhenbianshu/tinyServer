#include "./include/util_http.h"
#include <stdio.h>
#include <string.h>

void parse_request(char *http_request, struct request *cgi_request) {
    char request_str[2048];
    char top_body[2048];
    int header_len;
    char *delimiter = "\n";
    char *line;

    strcpy(request_str, http_request);

    line = strtok(http_request, delimiter);
    sscanf(line, "%s%s%s", cgi_request->REQUEST_METHOD, top_body, cgi_request->SERVER_PROTOCOL);

    memset(cgi_request->SCRIPT_NAME, 0, strlen(cgi_request->SCRIPT_NAME));
    for (int i = 0; i < strlen(top_body); i++) {
        if (top_body[i] == '?') {
            strncpy(cgi_request->SCRIPT_NAME, top_body, i);
            strncpy(cgi_request->QUERY_STRING, top_body + i + 1, strlen(top_body) - i);
            break;
        }
    }

    if (0 == strlen(cgi_request->SCRIPT_NAME)) {
        strcpy(cgi_request->SCRIPT_NAME, top_body);
        memset(cgi_request->QUERY_STRING, 0, strlen(cgi_request->QUERY_STRING));
    }

    while ((line = strtok(NULL, delimiter))) {
        parse_line(line, cgi_request);
    }
}

void parse_line(char *line, struct request *cgi_request) {
    if (strlen(line) == 0) {
        return;
    }

    // 一旦POST数据内有内容，则停止读取行
    if(strlen(cgi_request->POST_DATA) != 0){
        // 原换行符会扔掉content内的换行符，这里加上
        strcat(cgi_request->POST_DATA, "\n");
        cgi_request->CONTENT_LENGTH = strlen(cgi_request->POST_DATA) + strlen(line);
        strcat(cgi_request->POST_DATA, line);
        return;
    }

    char attr_name[32];
    char attr_value[128];

    int is_content = 1;
    for (int i = 0; i < strlen(line); i++) {
        if (line[i] == ':') {
            is_content = 0;
            strncpy(attr_name, line, i);
            attr_name[strlen(attr_name) - 2] = 0;
            if (line[i + 1] == ' ') {
                strncpy(attr_value, line + i + 2, strlen(line) - i);
            } else {
                strncpy(attr_value, line + i + 1, strlen(line) - i);
            }
        }
    }
    if (is_content == 1) {
        strcpy(cgi_request->POST_DATA, line);
        cgi_request->CONTENT_LENGTH = strlen(line);
        return;
    }

    if (strcmp(attr_name, "Host") == 0) {
        strcpy(cgi_request->SERVER_NAME, attr_value);
    } else if (strcmp(attr_name, "Content-type") == 0) {
        strcpy(cgi_request->CONTENT_TYPE, attr_value);
    }

    return;
}