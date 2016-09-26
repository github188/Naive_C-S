/********************************************************
 * Author :     wding  @streamax
 * filename :   clit.c
 * date:        2016 - 8 - 27
 * Description: NetModule program of P4 system
 *******************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include "p4net.h"

#define MAXSLEEP        64
#define BUFLEN          1024
#define SERV_ADDR_STR   "192.168.51.15"

struct pthread_arg {
    pthread_t   thread_id;
    int         tcp_sockfd;
    int         udp_sockfd;
};

/* try to connect with exponenital backoff */
int connect_retry(int domain, int type, int protocol,
                const struct sockaddr *addr, socklen_t alen)
{
    int numsec, fd;
    
    for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
        if ((fd = socket(domain, type, protocol)) < 0)
            oops("socket");
        
        if (connect(fd, addr, alen) == 0) {
            /* connection accepted */
            return(fd);
        }
        close(fd);

        /* delay before re-connect */
        if (numsec <= MAXSLEEP/2)
            sleep(numsec);
    }
    return(-1);
}

//void serv_test(int sockfd)
//{   
//    int     n;
//    char    buf[MAXLINE];
//    char    str[MAXLINE] = "talk is cheap";
//    struct Net_Info sndinfo;
//    sndinfo.type = 3;
//    strcpy(sndinfo.senderID, "Device No.1");
//    strcpy(sndinfo.content.loginfo.module_name, "DMM");
//    strcpy(sndinfo.content.loginfo.log_content, "Core Dump!");
//    sndinfo.content.loginfo.occur_time = time(NULL);
//    memcpy(buf, &sndinfo, sizeof(sndinfo));
//
//    do {
//        send(sockfd, buf, sizeof(sndinfo), 0);
//        sleep(3);
//    } while(1);
//
//    close(sockfd);
//}

static void *pthread_transmit(void *arg)
{
/* Have Done: recv udp resend to bsm */
    struct pthread_arg *targ = arg;
    int     n;
    char    revbuf[BUFFSIZE];
    int     tcp_sockfd = targ->tcp_sockfd;
    int     udp_sockfd = targ->udp_sockfd;
    printf("Thread %lu start\n", targ->thread_id);
    pthread_detach(targ->thread_id);
    for ( ; ; ) {
        int snd_frq = 0;
        printf("recv waiting\n");
        if ((n = recvfrom(udp_sockfd, revbuf, BUFFSIZE, 0, NULL, NULL)) <= 0)
            handle_error("recv error");
        printf("recv %d Bytes from somewhere: %s\n", n, revbuf);
        /* Send GPS info to Server */
        for ( ; ; ) {
            int flag_snd;
            /* set fd for non_block while send */
            if ((flag_snd = send(tcp_sockfd, revbuf, n, MSG_DONTWAIT)) > 0) 
                break;
            else if (flag_snd == 0) {
                printf("Server has been shutdown\n");
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    if (snd_frq < 10) {
                        usleep(1000);
                        snd_frq += 1;   /* 判断重发次数 */
                        continue;
                    } else {
                        printf("send timeout\n");
                        break;
                    }
                }
                else {
                    perror("Send");
                    break;
                }
            }
        }
    }
    return(NULL);
}

/* 该函数向中心模块注册服务，在本线程内接受服务器广告信息，并发送至对应模块 */
void serv_req(int tcp_sockfd, int udp_sockfd)
{
    char    revbuf[BUFFSIZE];
    int     len = 0;
    struct Net_Info P4MsgInfo, *tmp ;
    struct pthread_arg targ;
    struct sockaddr_un dest_addr;   /* 需求服务的模块域套接字地址 */
    targ.tcp_sockfd = tcp_sockfd;
    targ.udp_sockfd = udp_sockfd;

    if (pthread_create(&targ.thread_id, NULL, &pthread_transmit, &targ) != 0)
        handle_error("pthread error");

    /* TODO: 在此处注册服务，获取对方域套接字地址 */
    bzero(&dest_addr, sizeof(struct sockaddr_un));
    dest_addr.sun_family = AF_LOCAL;
    strcpy(dest_addr.sun_path, UNIX_PATH_DM);

    size_t slen = sizeof(P4MsgInfo);
    for ( ; ; ) {
        bzero(&P4MsgInfo, slen);
        if ( (len = recv(tcp_sockfd, revbuf, BUFFSIZE, 0)) < 0)
            handle_error("Recv from BSM");
        if (len == 0) {
            printf("Connection has been shutdown\n");
            break;
        }
        //write(STDOUT_FILENO, revbuf, len);
        tmp = (struct Net_Info *)revbuf;
        printf("revbuf: %s\n", tmp->content.adverinfo);
//        if (sendto(udp_sockfd, revbuf, len, 0, (SA *)&dest_addr, sizeof(struct sockaddr_un)) <0 )
//            handle_error("Sendto");
        sendto(udp_sockfd, revbuf, len, 0, (SA *)&dest_addr, sizeof(struct sockaddr_un));
        //memcpy(&P4MsgInfo, revbuf, slen);
        //printf("rev from serv which type is %d, senderID is %s\n, adverinfo is %s,here we go\n",
        //        P4MsgInfo.type, P4MsgInfo.senderID, P4MsgInfo.content.adverinfo);
        //printf("adverinfo snd_len is: %d\n", P4MsgInfo.info_length); 
        //printf("adverinfo rec_len is: %d\n", strlen(P4MsgInfo.content.adverinfo));
    }
    close(tcp_sockfd);
}

int main(int argc, const char **argv)
{
    int                 tcp_sockfd, udp_sockfd, err;
    /* 初始化TCP连接的地址结构 */
    struct addrinfo     *ailist, *aip;
    struct addrinfo     hint;
    /* 初始化域套接字的地址结构 */
    struct sockaddr_un  NM_addr;

    /* Init first for Unix Domain UDP */
    if ((udp_sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
        handle_error("UDP socket");
    unlink(UNIX_PATH_NM);
    bzero(&NM_addr, sizeof(struct sockaddr_un));
    NM_addr.sun_family = AF_LOCAL;
    strcpy(NM_addr.sun_path, UNIX_PATH_NM);
    /* bind UDP_sockfd with sockaddr_un */
    if (bind(udp_sockfd, (SA *) &NM_addr, sizeof(struct sockaddr_un)) != 0)
        handle_error("UDP Bind");

    /* Then init for TCP between BSM & NM */
    bzero(&hint, sizeof(hint));
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    /* Map host name and service name into addrinfo */
    if ((err = getaddrinfo(SERV_ADDR_STR, SERV_PORT_STR, &hint, &ailist)) != 0)
        syslog(LOG_ERR, "client: getaddrinfo error: %s\n", 
                gai_strerror(err));

    /* Connect to the server with the addrinfo */
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((tcp_sockfd = connect_retry(aip->ai_family, SOCK_STREAM, 0,
            aip->ai_addr, aip->ai_addrlen)) < 0) {
                continue;       /* connect fail, try the next address */
        } else {
            serv_req(tcp_sockfd, udp_sockfd);   /* connect success */
            exit(0);
        }
    }

    if (aip == NULL)
        handle_error("Cannot connect to the server");
    exit(0);
}

