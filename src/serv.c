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
#include <mysql/mysql.h>
#include "../lib/p4net.h"

#define BUFLEN  128
#define QLEN    8

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

//static void *servit(void *);
static void * servit(void *arg)
{
    int connfd;

    connfd = *((int *) arg);
    free(arg);

    printf("servit pthread id is %d\n",pthread_self());
    pthread_detach(pthread_self());
    printf("someone knock it\n");
    char str[20] = "welcome!\n";
    do {
            send(connfd, str, sizeof(str), 0);
            sleep(5);
    } while(1);

    close(connfd);
    return(NULL);
}

void serve(int listenfd)
{
    int             *connfd;
    socklen_t       alen, blen;
    struct  addrinfo cliaddr;
    char    buff[BUFLEN];
    char    clihost[BUFLEN];
    char    cliserv[BUFLEN];
    MYSQL           *conn;
    pthread_t       tid;
    MYSQL_RES       *res;
    MYSQL_ROW       row;

    /* 调用自定义包裹函数db_init进行数据库连接初始化 */

    if(db_init(&conn))
        printf("init error\n");

    printf("sql in serve:\n");

    if (mysql_query(conn, "SELECT * FROM `adverinfo`")) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
    
    res = mysql_use_result(conn);
    while ((row = mysql_fetch_row(res)) != NULL)
        printf("%s\n", row[1]);
    mysql_free_result(res);
    mysql_close(conn);

    for ( ; ; ) 
    {
        alen = sizeof(cliaddr);
        memset(&cliaddr, 0, alen);
        
        /* 为了将connfd值传递给子线程，将connfd设置为指针 */
        if ((connfd = malloc(sizeof(int))) < 0)
            handle_error("malloc");

        if ((*connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &alen)) < 0) {
            syslog(LOG_ERR, "serv: accept error: %s", strerror(errno));
            exit(1);
        }
        printf("accept success\n");      
        pthread_create(&tid, NULL, &servit, connfd);
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
    int             listenfd;
    int             hostlen;   
    char           *host;
    socklen_t       addrlenp;
    MYSQL          *conn;
    MYSQL_RES      *res;
    MYSQL_ROW       row; 
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
    /* 调用serv_init函数获取listen socket */
    if ((listenfd = serv_init(host, SERV_PORT_STR, &addrlenp)) < 0)
       oops("serv_init");
   
    serve(listenfd);

    exit(0); 
}
