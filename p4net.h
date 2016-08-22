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

#define MAXLINE     1024
#define BUFFSIZE    4096

#define SERV_PORT       9527
#define SERV_PORT_STR   "9527"

typedef enum
{
    SER_ADV = 1,
    CNT_GPS,
    CNT_LOG
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

