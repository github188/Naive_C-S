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

void serv_test(int sockfd)
{   
    int     n;
    char    buf[MAXLINE];
    char    str[MAXLINE] = "talk is cheap";
    struct Net_Info sndinfo;
    sndinfo.type = 3;
    strcpy(sndinfo.senderID, "Device No.1");
    strcpy(sndinfo.content.loginfo.module_name, "DMM");
    strcpy(sndinfo.content.loginfo.log_content, "Core Dump!");
    sndinfo.content.loginfo.occur_time = time(NULL);
    memcpy(buf, &sndinfo, sizeof(sndinfo));

    do {
        send(sockfd, buf, sizeof(sndinfo), 0);
        sleep(3);
    } while(1);

    close(sockfd);
}

static void *pthread_unixdg(void *arg)
{
/* TODO: recv udp resend to bsm */
    struct pthread_arg *targ = arg;
    int     n;
    char    revbuf[BUFFSIZE];
    int     tcp_sockfd = targ->tcp_sockfd;
    int     udp_sockfd = targ->udp_sockfd;
    printf("Thread %d: tcpsock is %d, udpsock is %d\n",
            targ->thread_id, targ->tcp_sockfd, targ->udp_sockfd);
    pthread_detach(targ->thread_id);
    //serv_test(tcp_sockfd);
    for ( ; ; ) {
        printf("recv waiting\n");
        if ((n = recvfrom(udp_sockfd, revbuf, BUFFSIZE, 0, NULL, NULL)) <= 0)
            handle_error("recv error");
        printf("recv %d Bytes from somewhere: %s\n", n, revbuf);
    }
    return(NULL);
}

void serv_req(int tcp_sockfd, int udp_sockfd)
{
    char    revbuf[BUFLEN];
    int     len = 0;
    struct Net_Info P4MsgInfo, *tmp ;
    struct pthread_arg targ;
    targ.tcp_sockfd = tcp_sockfd;
    targ.udp_sockfd = udp_sockfd;

    if (pthread_create(&targ.thread_id, NULL, &pthread_unixdg, &targ) != 0)
        handle_error("pthread error");

    size_t slen = sizeof(P4MsgInfo);
    for ( ; ; ) {
        memset(&P4MsgInfo, 0, slen);    
        if ( (len = recv(tcp_sockfd, revbuf, BUFLEN, 0)) < 0)
            oops("Recv from BSM");
        if (len == 0) {
            printf("Connection has been shutdown\n");
            break;
        }
        printf("recv len is: %d", len);
        printf("P4MsgInfo.type's size is: %d\n", sizeof(P4MsgInfo.type));
        printf("P4MsgInfo.senderID's size is: %d\n", sizeof(P4MsgInfo.senderID));
        printf("P4MsgInfo.info_length's size is: %d\n", sizeof(P4MsgInfo.info_length));
        write(STDOUT_FILENO, revbuf, len);
        tmp = (struct Net_Info *)revbuf;
        printf("revbuf: %s\n", tmp->content.adverinfo);
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
    /* addrinfo for TCP */
    struct addrinfo     *ailist, *aip;
    struct addrinfo     hint;
    /* sockaddr for unix domain */
    struct sockaddr_un  servaddr;

    /* Init first for Unix Domain UDP */
    if ((udp_sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
        handle_error("UDP socket");
    unlink(UNIX_PATH_NM);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, UNIX_PATH_NM);
    /* bind UDP_sockfd with sockaddr_un */
    if (bind(udp_sockfd, (SA *) &servaddr, sizeof(servaddr)) != 0)
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

