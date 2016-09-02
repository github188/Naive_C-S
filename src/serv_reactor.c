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
#include "../lib/p4net.h"

#define BUFLEN          128
#define QLEN            8

/* 数据库配置信息 */
#define DBHOST          "localhost"
#define DBUSER          "admin"
#define DBPWD           "admin"
#define DBNAME          "P4db"
#define DBPORT          3306

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

static int  client[FD_SETSIZE];
static int  flag_advpthread = 1;
static int  maxi = -1;

/*
 * 子线程：用于定时给所有已连接设备发送广告信息
 *         仅在第一个连接建立时启动，
 * TODO: 包裹数据库函数，优化查询循环
 */
static void *pthr_adver_boardcast(void *arg)
{
    pthread_detach(pthread_self());
    int          connfd;
    char sendbuf[BUFFSIZE];

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
            for (i = 0; i <= maxi; i++)
            {
                connfd = client[i];    
                //send(connfd, sendbuf, sizeof(sendbuf), 0);
            }
            sleep(2);
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

void serve(int listenfd)
{
    int             i, connfd, sockfd;
    int             maxfd, nready;
    ssize_t         n;
    pthread_t       tid;
    socklen_t       alen, blen;
    struct  addrinfo cliaddr;
    fd_set  rset, allset;
    char    buf[MAXLINE];
    char    clihost[BUFLEN];
    char    cliserv[BUFLEN];
    const char *query_init = "SET NAMES UTF8";

    MYSQL   *db_insert;
    
    maxfd = listenfd;

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

    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    for ( ; ; ) 
    {
        rset = allset;
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &rset)) {    /* new connection */
            alen = sizeof(cliaddr);
            memset(&cliaddr, 0, alen);
            if ((connfd = accept(listenfd, (SA *)&cliaddr, &alen)) < 0) {
                syslog(LOG_ERR, "serv: accept error: %s", strerror(errno));
                exit(1);
            }

            for (i = 0; i < FD_SETSIZE; i++)
                if (client[i] < 0) {
                    client[i] = connfd;     /* save descriptor */
                    break;
                }

            if (i == FD_SETSIZE)
                handle_error("too many clients");

            FD_SET(connfd, &allset);        /* add new fd to set */
            
            if (connfd > maxfd)
                maxfd = connfd;

            if (i > maxi)
                maxi = i;                   /* max index in client[] */
            if (flag_advpthread) {
                pthread_create(&tid, NULL, pthr_adver_boardcast, NULL);
                flag_advpthread = 0;
            }

            if (--nready <= 0)              /* if there is only one fd return in select */
                continue;
        }

        for (i = 0; i <= maxi; i++) {       /* check all clients for data */
            if ( (sockfd = client[i]) < 0)
                continue;

            if (FD_ISSET(sockfd, &rset)) {
                if ((n = read(sockfd, buf, MAXLINE)) == 0) {
                    /* Connection closed by client */
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                } else
                    date_insert(buf, db_insert);
                    //write(STDOUT_FILENO, buf, n);

                if (--nready <= 0)
                    break;
            }
        }
    }

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
