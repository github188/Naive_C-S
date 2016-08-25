/*******************************
 * Author :     wding
 * filename :   serv.c
 * date:        2016 - 8 - 19
 * Description: Server program in netmodule of P4 system
 *******************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include "../lib/p4net.h"

#define BUFLEN  128
#define QLEN    8

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

#define oops(msg)   { perror(msg), exit(1); }

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error_en(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

//static void *servit(void *);
static void * servit(void *arg)
{
    pthread_detach(pthread_self());
    printf("someone knock it\n");
    close((int) arg);
    return(NULL);
}

int initserver(int type, const struct sockaddr *addr, 
            socklen_t alen, int qlen)
{   
    int listenfd, err;
    int reuse = 1;

    if ( (listenfd = socket(addr->sa_family, type, 0)) < 0)
        oops("socket");
    printf("\n socket succeed, listenfd is: %d\n", listenfd);
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
            sizeof(int)) < 0)
        oops("setsockopt");
    if (bind(listenfd, addr, alen) < 0)
        oops("bind");
    printf("bind succeed!\n");
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        if (listen(listenfd, qlen) < 0){
            close(listenfd);
            oops("listen");
        }
    return(listenfd);
}

void serve(int listenfd)
{
    printf("listenfd is %d\n", listenfd);

    int             connfd;
    int             childpid;
    socklen_t       alen, blen;
    struct  addrinfo cliaddr;
    //struct  sockaddr_in cliaddr;
    char    buff[BUFLEN];
    char    clihost[BUFLEN];
    char    cliserv[BUFLEN];
    pthread_t       tid;

    for (;;) 
    {
        alen = sizeof(cliaddr);
        printf("alen is: %d\n",alen);
        //memset(&cliaddr, 0, len);
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &alen)) < 0) {
            syslog(LOG_ERR, "serv: accept error: %s", strerror(errno));
            exit(1);
        }
        printf("accept success\n");      
        pthread_create(&tid, NULL, &servit, (void *) connfd);
        //printf("connection from where?\n");
        
//        if ( (childpid = fork()) == 0) { /* child process */
//            close(listenfd);
//            
//            struct Net_Info rev_info;
//            int rlen = sizeof(rev_info);
//            memset(&rev_info, 0 ,rlen);
//
//            char *rev_buff = (char*)malloc(rlen);
//
//            int pos = 0, len = 0;
//            while (pos < rlen)
//            {
//                if ( (len = recv(connfd, rev_buff,
//                                rlen, 0)) < 0)
//                    oops("recv");
//                pos += len;
//            }
//                      
//            //close(connfd);
//            memcpy(&rev_info, rev_buff, rlen);
//
//            printf("recv over sendID = %s, info_length = %d\n, info_content is: %s\n",
//                    rev_info.senderID, rev_info.info_length, rev_info.info_content);
//            free(rev_buff);

            //getnameinfo() inverse of getaddrinfo
            //if ((getnameinfo((struct sockaddr *)&cliaddr, alen,
            //                clihost, BUFLEN, cliserv, BUFLEN, NI_NUMERICSERV||NI_NUMERICHOST)) != 0)
            //    oops("getnameinfo");
            //blen = snprintf(buff, sizeof(buff), "connection from: ", clihost, ", port: ", cliserv);
            //if((blen = snprintf(buff, sizeof(buff), "connection from %s, port: %s\n", 
            //           clihost, cliserv)) < 0)
            //        oops("snprintf");
            //printf(buff, blen);
            //printf("connection from %s, port %s\n",clihost ,cliserv);
            //write(connfd, buff, strlen(buff));
            //close(connfd);
            //printf("reading from client...");
            //str_echo(connfd);
            //close(connfd);
            
        }
    exit(0);
    //close(connfd);
}


int main(void)
{
    struct addrinfo *ailist, *aip;      
    struct addrinfo hint;
    int             sockfd, err;
    int             hostlen;   
    char            *host;
    
    /*
     *  检测系统支持最大的主机名长度
     *  如果获取失败便设置为256 
     */
    if ( (hostlen = sysconf(_SC_HOST_NAME_MAX)) < 0)
        hostlen = HOST_NAME_MAX;
    if ((host = malloc(hostlen)) == NULL)
        oops("malloc");
    
    /*  获取服务器主机名  */
    if (gethostname(host, hostlen) < 0)
        oops("gethostname");
    
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    /* 将主机名和服务名映射为一个地址  */
    if ((err = getaddrinfo(host, SERV_PORT_STR, &hint, &ailist)) != 0) {
        syslog(LOG_ERR, "serv: getaddrinfo error: %s", gai_strerror(err));
        exit(1);
    }

    aip = ailist;   /* aip指向地址链表的首项 */

    do {
        if ((sockfd = initserver(aip->ai_socktype, aip->ai_addr, 
                    aip->ai_addrlen, QLEN)) >= 0) {
            serve(sockfd);
            close(sockfd);
        }
    } while ( (aip = aip->ai_next) != NULL);

    if (aip == NULL)
        oops("aip");

    freeaddrinfo(aip);

    /* simply select first addr to listen ,need to improve here*/
//    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
//        if ((listenfd = initserver(SOCK_STREAM, aip->ai_addr, 
//                    aip->ai_addrlen, QLEN)) >= 0) {
//            serve(listenfd);
//            exit(0);
//        }
//    }
    exit(0);
}

