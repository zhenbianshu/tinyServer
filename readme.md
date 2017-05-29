#使用说明：
- 首先要有一个 cli 程序，如 本服务器使用的 php
- 修改源码中的 shell command 变量为自己使用的shell路径

```$xslt
gcc -c util_http.c -fpic -o util_http.so -std=c99
gcc -c cJSON.c -fpic -o cjson.so -std=c99
gcc -o server -L . util_http.so cjson.so server.c
./server

curl -i 127.0.0.1:8080/index.php?name=zbs
```
