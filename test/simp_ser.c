#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <syslog.h>
#include <errno.h>      /* perror */
#include <string.h>     /* bzero */

#define BUFLEN          1024
#define PORTNUM_STR     "9527"	/* 服务器端口号 */
#define QLEN    10              /* 定义listen函数backlog值 */

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256     /* 主机名最大位数 */
#endif

/* 错误处理函数 */
#define handle_error_en(en, msg) \
            do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
            do { perror(msg); exit(EXIT_FAILURE); } while (0)


/* 该函数用来分配、初始化、设置套接字，将套接字与地址关联 */
int initserver(int type, const struct sockaddr *addr, 
            socklen_t alen, int qlen)
{   
    int listenfd;		/* 用于监听连接请求的套接字 */
    int reuse = 1;

    if ((listenfd = socket(addr->sa_family, type, 0)) < 0)
        return(-1);		/* 创建一个套接字（用于监听的主动套接字）*/
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
            sizeof(reuse)) < 0)				/*套接字设置，SO_REUSEADDR选项 */
        handle_error("Setsockopt");                 /* 该函数中报错后应关闭listenfd */
    if (bind(listenfd, addr, alen) < 0)		/* 将套接字与主机地址绑定 */
        handle_error("Bind");
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)  /* 对于TCP和SCTP需建立连接 */
        if (listen(listenfd, qlen) < 0) 	/* 使服务器准备接受该套接字上的连接请求 */
            handle_error("listen");
    return(listenfd);
}

/* 该函数用来处理客户端的连接请求，完成与客户端的信息交互 */
void serve(int listenfd)
{
    int              connfd;			    /* 已连接套接字 */
	int 			 reclen;
    socklen_t        addrlen;			
    struct  addrinfo cliaddr;           /* 用于存放主机地址信息 */
    char    recbuff[BUFLEN];
    char    sndbuff[BUFLEN];
    char    clihost[BUFLEN];
    char    cliserv[BUFLEN];

    for (;;) 
    {
        addrlen = sizeof(cliaddr);
        bzero(&cliaddr, addrlen);
        bzero(&sndbuff, BUFLEN);

        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &addrlen)) < 0) {
            handle_error("Accept");     /* accept可以通过值-结果形式返回所连接客户端的地址信息 */
        } 
        /* 此处listenfd在完成accept操作后应该设置为close-on-exec (本程序中省略) */
    
        /* 将主机地址映射回主机名和服务名 */
        if ((getnameinfo((const struct sockaddr *)&cliaddr, addrlen,
                            clihost, BUFLEN, cliserv, BUFLEN, NI_NUMERICSERV||NI_NUMERICHOST)) != 0)
            handle_error("Getnameinfo");

        /* 阻塞于connfd，用于读取客户端传来数据 */
        if ((reclen = recv(connfd, recbuff, BUFLEN, 0)) < 0)
            perror("Read");

        else if (reclen == 0)    /* read返回0表示对端关闭了套接字 */
            printf("Connection closed by client\n");
        else {
            snprintf(sndbuff, BUFLEN, "Connection from: %s, port: %s,\nContent from client: %s\n", 
                            clihost, cliserv, recbuff);	/* 将返回给客户端的数据存入缓冲区 */
            send(connfd, sndbuff, BUFLEN, 0);	/* 将处理后数据返回给客户端 */
        }
        close(connfd);  /* 本次连接结束 */
    }
}

int main(void)
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int             listenfd, err, hostlen;
    char            *host;

    if ((hostlen = sysconf(_SC_HOST_NAME_MAX)) < 0)
        hostlen = HOST_NAME_MAX;
    if ((host = malloc(hostlen)) == NULL)
        handle_error("malloc");
    if (gethostname(host, hostlen) < 0)
        handle_error("gethostname");

    bzero(&hint, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;			/* 套接字将被作为被动监听套接字--bind() */
    hint.ai_socktype = SOCK_STREAM;		/* socket type SOCK_STREAM默认情况下为TCP */		
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(host, PORTNUM_STR, &hint, &ailist)) != 0) {
        syslog(LOG_ERR, "serv: getaddrinfo error: %s", gai_strerror(err));
        exit(1);						/* 将主机名和服务名映射为地址 */
    }
	/* getaddrinfo可能返回多个地址（地址链表），逐个尝试地址用于监听连接请求 */
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {	
        if ((listenfd = initserver(SOCK_STREAM, aip->ai_addr, 
                    aip->ai_addrlen, QLEN)) < 0) {
            continue;
        } else {
            serve(listenfd);
            exit(0);
        }
    }
    handle_error("Server Init");    
}

