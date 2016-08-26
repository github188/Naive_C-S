/*******************************************
 * Copyright (C), 2002-2016,SZ-STREAMING Tech. Co., Ltd.
 * Filename :
 *              P4NetModule.h
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
#include <errno.h>
#include <syslog.h>

#define LISTENQ     1024
#define MAXLINE     1024
#define BUFFSIZE    4096

#define SERV_PORT       9527
#define SERV_PORT_STR   "9527"

#define SA  struct sockaddr

/* error handle function */
#define handle_error_en(en, msg) \
            do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg) \
            do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define oops(msg) {perror(msg); exit(1); }

/* serv_int fun defined in P4NetInit.c */
int serv_init(const char *, const char *, socklen_t *);

/* protocol struct used in server/netmodule of P4system */
typedef enum
{
    SER_ADV = 1,    /* advertise infomation */
    CNT_GPS,        /* GPS info */
    CNT_LOG         /* syslog info */
}TYPE;

#ifndef HAVE_NETMODULEINFO_STRUCT
struct Net_Info {
    TYPE            type;
    char            senderID[20];
    size_t          info_length;
    char            info_content[BUFFSIZE];
};
#endif

#endif

