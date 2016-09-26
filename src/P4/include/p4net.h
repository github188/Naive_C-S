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
#include <stdint.h>

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

#define UNIX_PATH_NM    "/tmp/net_module.dg"     /* Unix domain datagram path for net_module */
#define UNIX_PATH_DM    "/tmp/dev_module.dg"      /* Unix domain datagram path for dev_module */

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

#pragma pack(1)

/* protocol struct used in server/netmodule of P4system */
typedef enum
{
    SER_ADV = 1,    /* advertise infomation */
    CNT_GPS,        /* GPS info */
    CNT_LOG         /* syslog info */
}type_t;

typedef struct gps_info {
    char            device_ip[32];
    char            gps_time[50];
    char            sys_time[50];
    char            longitude[32];
    char            latitude[32];
    char            altitude[32];
}gps_info_t;

typedef struct log_info {
    char            module_name[32];
    char            log_content[128];
    time_t          occur_time;
}log_info_t;

typedef union info_content {
    gps_info_t      gpsinfo;
    log_info_t      loginfo;
    char            adverinfo[255];
}info_content_t;

#ifndef HAVE_NETMODULEINFO_STRUCT
struct Net_Info {
    type_t              type;
    char                senderID[20];
    uint64_t            info_length;
    info_content_t      content;
};
#endif

#endif

