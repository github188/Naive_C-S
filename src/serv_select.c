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

//static void *servit(void *);
static void * sndto(void *arg)
{
    int          connfd;

    connfd = *((int *) arg);
    
    MYSQL_RES   *db_result;
    MYSQL_ROW    db_row;
    MYSQL_FIELD *db_field;

    unsigned int timeout = 3000;    /* mysqlopt timeout val */
    unsigned int num_fields;        /* fields numbers of query result */
    unsigned int i;                 /* int for loop */
    const char *pQuery_init = "SET NAMES UTF8";
    const char *pQuery      = "SELECT * FROM `adverinfo`";

    mysql_thread_init();
    MYSQL *mysql = mysql_init(NULL);

    if (mysql == NULL) {
        printf("mysql init error\n");
        return(NULL);
    }

//    mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    if (!mysql_real_connect(mysql, DBHOST, DBUSER, DBPWD, 
                        DBNAME, 0, NULL, 0))
    {
        printf("connect failed: %s\n", mysql_error(mysql));
        mysql_close(mysql);
        mysql_library_end();
        return 0;
    }

    printf("connection success\n");

    if (0 != mysql_real_query(mysql, pQuery_init, strlen(pQuery_init)))
    {
        printf("query failed: %s\n", mysql_error(mysql));
        mysql_close(mysql);
        mysql_library_end();
        return 0;
    }

    if (0 != mysql_real_query(mysql, pQuery, strlen(pQuery)))
    {
        printf("query failed: %s\n", mysql_error(mysql));
        mysql_close(mysql);
        mysql_library_end();
        return 0;
    }

    db_result = mysql_store_result(mysql);
    
    if (db_result == NULL)
    {
        printf("fetch result failed: %s\n", mysql_error(mysql));
        mysql_close(mysql);
        mysql_library_end();
        return 0;
    }

    num_fields = mysql_num_fields(db_result);
    printf("numbers of result: %d\n", num_fields);
    
    while (NULL != (db_row = mysql_fetch_row(db_result)))
    {
        unsigned long *lengths;
        lengths = mysql_fetch_lengths(db_result);

        for (i = 0; i < num_fields; i++)
            printf("{%.*s}", (int) lengths[i], db_row[i] ? db_row[i] : "NULL");
        printf("\n");
    }

    mysql_free_result(db_result);
    mysql_close(mysql);
//    mysql_thread_end();
//    mysql_library_end();

    free(arg);

    

    printf("servit pthread id is %d\n", pthread_self());
    pthread_detach(pthread_self());
    char str[20] = "welcome!\n";
    do {
            send(connfd, str, sizeof(str), 0);
            sleep(5);
    } while(1);

    close(connfd);
    
    mysql_thread_end();
    mysql_library_end();

    return(NULL);
}

static void * recfm(void *arg)
{
    int     connfd;
    connfd = *((int *) arg);
//    free(arg);
    printf("recfrm pthread id is %d\n", pthread_self());
    pthread_detach(pthread_self());
    char str[20] = "streamax\n";
    do {
        send(connfd, str, sizeof(str), 0);
        sleep(1);
    } while(1);

    close(connfd);
    return(NULL);
}

void serve(int listenfd)
{
    int             i, connfd, sockfd;
    int             maxi, maxfd, nready;
    int             client[FD_SETSIZE];
    ssize_t         n;
    pthread_t       tid;
    socklen_t       alen, blen;
    struct  addrinfo cliaddr;
    fd_set  rset, allset;
    char    buf[MAXLINE];
    char    clihost[BUFLEN];
    char    cliserv[BUFLEN];
    
    maxfd = listenfd;
    maxi  = -1;

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
                    write(STDOUT_FILENO, buf, n);

                if (--nready <= 0)
                    break;
            }
        }
    }
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
