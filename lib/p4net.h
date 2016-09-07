/*******************************************
 * Copyright (C), 2002-2016,SZ-STREAMAX Tech. Co., Ltd.
 * Filename :
 *              p4net.h
 * Author :     wding
 * Description: header for network module of P4 system
 */
#ifndef __P4NET_H_
#define __P4NET_H_
#include <sys/types.h>      /* basic system data types */
#include <sys/socket.h>     /* basic socket definitions */
#include <sys/time.h>       /* timeval{} for select */
#include <time.h>           /* timespec{} for pselect */
#include <arpa/inet.h>      /* inet(3) functions */
#include <pthread.h>
#include <netdb.h>
#include <sys/un.h>
#include <syslog.h>
#include <errno.h>
#include <sys/un.h>         /* Unix domain socket */

#ifndef AF_LOCAL
#define AF_LOCAL    AF_UNIX
#endif

#ifndef PF_LOCAL
#define PF_LOCAL    PF_UNIX
#endif

#define LISTENQ     1024        /* listen queue */
#define MAXLINE     4096
#define BUFFSIZE    8192

#define SERV_PORT       9527
#define SERV_PORT_STR   "9527"      /* port used in BSM&NM */

#define UNIX_PATH_NM    "/tmp/unix_path.dg"     /* Unix domain datagram path */

#define SA  struct sockaddr

/* error handle function */
#define handle_error_en(en, msg) \
            do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg) \
            do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define oops(msg) {perror(msg); exit(1); }

#define handle_mysql_error(msg, mysql) \
            do { printf(msg, mysql_error(mysql)); \
                mysql_close(mysql); \
                mysql_library_end(); \
                exit(EXIT_FAILURE); \
            } while(0)

/* serv_int fun defined in P4NetInit.c */
int serv_init(const char *, const char *, socklen_t *);

/* protocol struct used in server/netmodule of P4system */
typedef enum
{
    SER_ADV = 1,    /* advertise infomation */
    CNT_GPS,        /* GPS info */
    CNT_LOG         /* syslog info */
}type_t;

typedef struct gps_info {
    char            info_type[6];
    char            local_time[50];
    char            Latitude[20];
    char            Longitude[20];
}gps_info_t;

typedef union info_content {
    gps_info_t      gps;
    char            adverinfo[256];
    char            loginfo[128];
}info_content_t;

#ifndef HAVE_NETMODULEINFO_STRUCT
struct Net_Info {
    type_t              type;
    char                senderID[20];
    size_t              info_length;
    info_content_t      content;
};
#endif

#endif

