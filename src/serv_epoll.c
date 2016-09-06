/*******************************
 * Author :     wding  @streamax
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
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "../lib/p4net.h"

#define BUFLEN          128
#define QLEN            8
#define MAXEVENTS       64

/* 数据库配置信息 */
#define DBHOST          "localhost"
#define DBUSER          "admin"
#define DBPWD           "admin"
#define DBNAME          "P4db"
#define DBPORT          3306

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

static int  client[MAXEVENTS];
static int  flag_advpthread = 1;
static int  maxi = -1;
static int  p_epollfd;

/*
 * 子线程：用于定时给所有已连接设备发送广告信息
 *         仅在第一个连接建立时启动，
 * TODO: 包裹数据库函数，优化查询循环
 */
static void *pthr_adver_boardcast(void *arg)
{
    pthread_detach(pthread_self());
    int          connfd, pfds;
    char sendbuf[BUFFSIZE];
    struct epoll_event events[MAXEVENTS];

    MYSQL       *db_mysql;
    MYSQL_RES   *db_result;
    MYSQL_ROW    db_row;
    MYSQL_FIELD *db_field;

    unsigned int timeout = 3000;    /* mysqlopt timeout val */
    unsigned int num_fields;        /* fields numbers of query result */
    unsigned int i;                 /* int for loop */
    const char *pQuery_init = "SET NAMES UTF8";     /* set mysql for utf8 */
    const char *pQuery      = "SELECT * FROM `adverinfo`";  /* select table of advertise info */

    /* database init */
    mysql_thread_init();
    if ( NULL == (db_mysql = mysql_init(NULL))) {
        printf("mysql init error\n");
        return(NULL);
    }
    
//    mysql_options(db_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    if (!mysql_real_connect(db_mysql, DBHOST, DBUSER, DBPWD, 
                        DBNAME, 0, NULL, 0))
        handle_mysql_error("connect failed: %s\n", db_mysql);
    /* set mysql names for UTF8 */
    if (0 != mysql_real_query(db_mysql, pQuery_init, strlen(pQuery_init)))
        handle_mysql_error("utf8 init failded: %s\n", db_mysql);

    while(1) {  /* get adverinfo for loop */
        if (0 != mysql_real_query(db_mysql, pQuery, strlen(pQuery)))
            handle_mysql_error("query for adverinfo error: %s\n", db_mysql);
    
        if ( NULL == (db_result = mysql_store_result(db_mysql)))
            handle_mysql_error("fetch result failed: %s\n", db_mysql);

        while (NULL != (db_row = mysql_fetch_row(db_result)))
        {
            memset(sendbuf, 0, sizeof(sendbuf));
            sprintf(sendbuf, "Adver: %s\n", db_row[1]);
            
            if((pfds = epoll_wait(p_epollfd, events, MAXEVENTS, -1)) == -1)
                handle_error("pthread epoll_wait");
            printf("pfds is %d\n", pfds);
            for (i = 0; i < pfds; i++)
            {
                if(events[i].events & EPOLLOUT) {
                    printf("send to connfd: %d\n", events[i].data.fd);
                    send(events[i].data.fd, sendbuf, sizeof(sendbuf), 0);
                }
            }
            sleep(20);
        }
        free(db_row);

        mysql_free_result(db_result);
    }
    mysql_close(db_mysql);
    
    mysql_thread_end();
    mysql_library_end();

    return(NULL);
}

int date_insert(const char *buf, MYSQL *db_insert)
{
    printf("data ready\n");
    char mysql_buf[BUFLEN];
    bzero(mysql_buf, BUFLEN);

    sprintf(mysql_buf, "INSERT INTO `loginfo` values(NULL, '%s');", buf);
     
    if (0 != mysql_real_query(db_insert, mysql_buf, strlen(mysql_buf)))
            handle_mysql_error("query for adverinfo error: %s\n", db_insert);

    write(STDOUT_FILENO, mysql_buf, sizeof(mysql_buf));

    return 0;
}

static int set_non_blocking(int sockfd)
{
    int flags;
    if ((flags = fcntl(sockfd, F_GETFL, 0)) < 0)
        handle_error("F_GETFL");
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) < 0)
        handle_error("F_SETFL");
    return 0;
}

void serve(int listenfd)
{
    int             i, connfd, sockfd, j, epollfd, nfds;
    ssize_t         n;
    pthread_t       tid;
    socklen_t       alen, blen;
    struct  addrinfo cliaddr;
    fd_set  rset, allset;
    char    buf[MAXLINE];
    char    clihost[BUFLEN];
    char    cliserv[BUFLEN];
    const char *query_init = "SET NAMES UTF8";

    struct epoll_event event, events[MAXEVENTS];

    MYSQL   *db_insert;
 
    for (i = 0; i < MAXEVENTS; i++)
        client[i] = -1;
       
    mysql_library_init(0, NULL, NULL);

    if ( NULL == (db_insert = mysql_init(NULL))) {
        printf("mysql init error\n");
        exit(1);
    }

    if (!mysql_real_connect(db_insert, DBHOST, DBUSER, DBPWD, 
                        DBNAME, 0, NULL, 0))
        handle_mysql_error("connect failed: %s\n", db_insert);
    /* set mysql names for UTF8 */
    if (0 != mysql_real_query(db_insert, query_init, strlen(query_init)))
        handle_mysql_error("utf8 init failded: %s\n", db_insert);

    if ((epollfd = epoll_create1(0)) == -1)
        handle_error("epoll_create");

    if ((p_epollfd = epoll_create1(0)) == -1)
        handle_error("epoll_create");

    printf("epollfd is %d, p_epollfd is %d\n", epollfd, p_epollfd);

    event.events = EPOLLIN;
    event.data.fd = listenfd;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event) < 0)
        handle_error("epoll_ctl");

    for ( ; ; ) 
    {
        if ((nfds = epoll_wait(epollfd, events, MAXEVENTS, -1)) == -1)
            handle_error("epoll_wait");

        for (j = 0; j < nfds; j++) {
            if (listenfd == events[j].data.fd)
            { /* new connection */
                alen = sizeof(cliaddr);
                memset(&cliaddr, 0, alen);
                if ((connfd = accept(listenfd, (SA *)&cliaddr, &alen)) < 0) {
                    syslog(LOG_ERR, "serv: accept error: %s", strerror(errno));
                    exit(1);
                }
                
                if (getnameinfo((SA *)&cliaddr, alen, clihost, BUFLEN,
                            cliserv, BUFLEN, NI_NUMERICHOST|NI_NUMERICSERV) == 0)
                    printf("Accepted connection on descriptor %d, host: %s, port: %s\n",
                            connfd, clihost, cliserv);

//                if (set_non_blocking(connfd) != 0)
//                    handle_error("setnonblocking");

                event.data.fd = connfd;
                event.events = EPOLLIN;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &event) < 0)
                    handle_error("epoll_ctl");
                
                event.events = EPOLLOUT;
                if (epoll_ctl(p_epollfd, EPOLL_CTL_ADD, connfd, &event) < 0)
                    handle_error("p_epoll_ctl");

                for (i = 0; i < MAXEVENTS; i++)
                    if (client[i] < 0) {
                        client[i] = connfd;     /* save fd in client[] */
                        break;
                    }
                if (i > maxi)
                    maxi = i;
                if (flag_advpthread) {
                    pthread_create(&tid, NULL, pthr_adver_boardcast, NULL);
                    flag_advpthread = 0;
                }

                continue;
            }
            else if(events[j].events & EPOLLIN) {
                /* data on sockfd waiting to be read */
                if ((n = read(events[j].data.fd, buf, MAXLINE)) == 0) 
                    close(events[j].data.fd);
                else
                    write(STDOUT_FILENO, buf ,n);
            }
//            else if(events[j].events & EPOLLOUT)
//                printf("write ready\n");
//            else 
//                printf("error?\n");
        }
    }

//        for (i = 0; i <= maxi; i++) {       /* check all clients for data */
//            if ( (sockfd = client[i]) < 0)
//                continue;
//
//            if (FD_ISSET(sockfd, &rset)) {
//                if ((n = read(sockfd, buf, MAXLINE)) == 0) {
//                    /* Connection closed by client */
//                    close(sockfd);
//                    FD_CLR(sockfd, &allset);
//                    client[i] = -1;
//                } else
//                    date_insert(buf, db_insert);
//                    //write(STDOUT_FILENO, buf, n);
//
//                if (--nready <= 0)
//                    break;
//            }
//        }
//    }
    mysql_close(db_insert);
    mysql_library_end();
}


int main(void)
{
    int             listenfd;
    int             hostlen;   
    char           *host;
    socklen_t       addrlenp;
 
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
