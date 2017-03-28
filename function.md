###socket
    int socket(int af, int type, int protocol);
     
    af (adress faimly), 协议族， 有AF_INET(IPV4) AF_INET6(IPV6)等，也可用 PF(protocal family) 为前缀
         
    type 数据传输方式，有 SOCK_STREAM（流套接字）、SOCK_DGRAM（数据报套接字）等
    
    protocal 协议类型， 有 IPPROTO_TCP（TCP协议）、IPPROTO_UDP(UDP协议), 将此位置0，系统自己选择使用哪种协议

###bind
        int bind(int sock, struct sockaddr *addr, socklen_t addrlen)
        sock 即一开始创建的socket
        sockaddr_in类型的结构：
        struct sockaddr_in{
             sa_family_t     sin_family;   //地址族（Address Family），也就是地址类型
             uint16_t        sin_port;     //16位的端口号
             struct in_addr  sin_addr;     //32位IP地址
             char            sin_zero[8];  //不使用，一般用0填充
         };
        sockaddr类型的地址结构：
        struct sockaddr{
             sa_family_t  sin_family;   //地址族（Address Family），也就是地址类型
             char         sa_data[14];  //IP地址和端口号
        }
        sockaddr 和 sockaddr_in 的长度相同，都是16字节，只是将IP地址和端口号合并到一起，用一个成员 sa_data 表示。
        要想给 sa_data 赋值，必须同时指明IP地址和端口号，例如”127.0.0.1:80“，
        没有相关函数将这个字符串转换成需要的形式，也就很难给 sockaddr 类型的变量赋值，所以使用 sockaddr_in 来代替。
        这两个结构体的长度相同，强制转换类型时不会丢失字节，也没有多余的字节。
        
###listen
    
     开始监听服务器端口
     当套接字正在处理客户端请求时，如果有新的请求进来，套接字是没法处理的，只能把它放进缓冲区，待当前请求处理完毕后，再从缓冲区中读取出来处理。
     如果不断有新的请求进来，它们就按照先后顺序在缓冲区中排队，直到缓冲区满。这个缓冲区，就称为请求队列（Request Queue）
 
###accept

         受理客户端的请求
         int accept(int sock, struct sockaddr *addr, socklen_t *addrlen);
         sock 服务器套接字
         accept() 会阻塞程序执行

###spintf
    int sprintf(char *str, char * format [, argument, ...]) 以format将参数格式化，返回str的长度

###epoll_wait
    int epoll_wait(int epfd, struct epoll_event *events,
                      int maxevents, int timeout);

       epfd epoll句柄
       events  触发事件
       maxevents  最大事件数量
       timeout 超时时间  -1会使wait一直阻塞，而0则会立刻返回

       epoll_event 
           typedef union epoll_data {
               void    *ptr;
               int      fd;
               uint32_t u32;
               uint64_t u64;
           } epoll_data_t;
           
           struct epoll_event {
               uint32_t     events;    /* Epoll events */
               epoll_data_t data;      /* User data variable */
           };
