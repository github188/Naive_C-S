#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>

#define BUFLEN      1024
#define MAXSLEEP    64      /* 最大重连间隔时间 */

/* 错误处理函数 */
#define handle_error_en(en, msg) \
            do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
            do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* 带有重连的connect，间隔时间使用指数补偿 */
int connect_retry(int domain, int type, int protocol,
                const struct sockaddr *addr, socklen_t alen)
{
    int numsec, sockfd;
    
    for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
        if ((sockfd = socket(domain, type, protocol)) < 0)
            handle_error("Socket");
        
        if (connect(sockfd, addr, alen) == 0) {
            /* 连接建立成功 */
            return(sockfd);
        }

        /* 连接失败后延迟一段时间后重连 */
        close(sockfd);
        if (numsec <= MAXSLEEP/2)
            sleep(numsec);
    }
    return(-1);
}

void serv_request(int sockfd)
{
    int     snd, rec;
    char    sndbuf[BUFLEN];
    char    recbuf[BUFLEN];
    bzero(&sndbuf, BUFLEN);
    bzero(&recbuf, BUFLEN);
    /* 将输入的内容发送至服务器 */
    printf("Enter the content you wanna to send\n");
    if ( (snd = read(STDIN_FILENO, sndbuf, BUFLEN)) < 0)
        handle_error("Read stdin");
    send(sockfd, sndbuf, snd, 0);
    /* 获取服务器返回的信息 */
    printf("message from server:\n");
    while ((rec = recv(sockfd, recbuf, BUFLEN, 0)) > 0)
        write(STDOUT_FILENO, recbuf, rec);
    if (rec < 0)
        handle_error("Recv");

    exit(0);
}

int main(int argc, char *argv[])
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int             sockfd, err;

    if (argc != 3) {
        printf("usage: ServerInfo Needed");
        exit(-1);
    }
    bzero(&hint, sizeof(hint));
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(argv[1], argv[2], &hint, &ailist)) != 0)
        syslog(LOG_ERR, "Client: getaddrinfo error: %s\n", 
                gai_strerror(err));
    
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = connect_retry(aip->ai_family, SOCK_STREAM, 0,
            aip->ai_addr, aip->ai_addrlen)) < 0) {
                continue;
        } else {
            serv_request(sockfd);
            exit(0);
        }
    }
    handle_error("Can't connect to the server");
}

