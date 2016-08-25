#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

#define BUFLEN          128
#define PORTNUM_STR     "9527"	/* 服务器端口号 */
#define QLEN    10

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

#define oops(msg)   { perror(msg), exit(1); }	/* 错误处理函数 */

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
        oops("setsockopt");                 /* 该函数中报错后应关闭listenfd */
    if (bind(listenfd, addr, alen) < 0)		/* 将套接字与主机地址绑定 */
        oops("bind");
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        if (listen(listenfd, qlen) < 0) 	/* 调用该函数表示服务器愿意接受连接请求 */
            oops("listen");
    return(listenfd);
}

/* 该函数用来处理客户端的连接请求，完成与客户端的信息交互 */
void serve(int listenfd)
{
    int             connfd;			    /* 已连接套接字 */
	int 			reclen;
    socklen_t       addrlen;			
    struct  addrinfo cliaddr;           /* 用于存放主机地址信息 */
    char    recbuff[BUFLEN];
    char    sndbuff[BUFLEN];
    char    clihost[BUFLEN];
    char    cliserv[BUFLEN];

    for (;;) 
    {
        addrlen = sizeof(cliaddr);
        memset(&cliaddr, 0, addrlen);
        memset(&sndbuff, 0, BUFLEN);
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &addrlen)) < 0) {
            syslog(LOG_ERR, "serv: accept error: %s", strerror(errno));
            exit(1);					/* accept函数可以返回所连接客户端的地址信息 */
        } 
        /* 此处listenfd在完成accept操作后应该设置为close-on-exec (本程序中省略) */
        if ((getnameinfo((const struct sockaddr *)&cliaddr, addrlen,
                            clihost, BUFLEN, cliserv, BUFLEN, NI_NUMERICSERV||NI_NUMERICHOST)) != 0)
            oops("getnameinfo");		/* 将主机地址映射回主机名和服务名 */
        if ((reclen = read(connfd, recbuff, BUFLEN)) < 0)
            oops("read");				/* 阻塞于connfd，用于读取客户端传来数据 */
        snprintf(sndbuff, BUFLEN, "Connection from: %s, port: %s,\nContent from client: %s"
                        , clihost, cliserv, recbuff);	/* 将返回给客户端的数据存入缓冲区 */

        write(connfd, sndbuff, BUFLEN);	/* 将处理后数据返回给客户端 */
        close(connfd);
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
        oops("malloc");
    if (gethostname(host, hostlen) < 0)
        oops("gethostname");
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;			/* addrinfo结构标志 */
    hint.ai_socktype = SOCK_STREAM;		/* socket type SOCK_STREAM默认情况下为TCP */		
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(host, PORTNUM_STR, &hint, &ailist)) != 0) {
        syslog(LOG_ERR, "serv: getaddrinfo error: %s", gai_strerror(err));
        exit(1);						/* 将主机名和服务名映射为地址 */
    }
	/* getaddrinfo可能返回多个地址（地址链表），此处简单地选取第一个地址用于监听连接请求 */
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {	
        if ((listenfd = initserver(SOCK_STREAM, aip->ai_addr, 
                    aip->ai_addrlen, QLEN)) >= 0) {
            serve(listenfd);
            exit(0);
        }
    }
    exit(1);
}

