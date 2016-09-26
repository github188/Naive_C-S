/********************************************************
 * Author :     wding  @streamax
 * filename :   dev_mod.c
 * date:        2016 - 9 - 13
 * Description: Device Module program of P4 system
 *******************************************************/
#include "p4net.h"
#include "p4basic.h"
#include "nmea/nmea.h"
#include "p4led.h"
#include "iconv.h"

#include <fcntl.h>
#include <termios.h>

/* 为了方便代码补全 */
#include "./include/p4net.h"
#include "./include/p4basic.h"
#include "./include/nmea/nmea.h"
#include "./include/p4led.h"
#include "./include/iconv.h"

int  ttys_date_analysis(const char*, const size_t, struct Net_Info *);
int  nmea_gps_convert(const char*, const size_t, struct Net_Info *);
static void *pthread_advrev(void *);

struct pthread_arg {
    pthread_t   thread_id;
    int         udp_sockfd;
};

void Time_sys(char *sys_time)
{
    time_t  now;
    struct tm *timenow;
    time(&now);
    timenow = localtime(&now);
    sprintf(sys_time, "%04d-%02d-%02d %02d:%02d:%02d\n",
                timenow->tm_year + 1900,
                timenow->tm_mon  + 1,
                timenow->tm_mday,
                timenow->tm_hour,
                timenow->tm_min,
                timenow->tm_sec);
}

int main()
{
    int     mcu_fd, sockfd;
    struct  sockaddr_un dest_addr, DM_addr;  /* addr struct for UnixSocket */
    struct  Net_Info    dev_msg;
    struct  pthread_arg p_arg;

    char    revbuf[1024*16];
    
    if ((mcu_fd = open("/dev/ttyACM0", O_RDWR)) < 0)
        handle_error("Open");
    else
        printf("Open serial ttyACM0 success\n");

    if ((sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
        handle_error("Socket");

    /* set dev_mod own path in cliaddr */
    bzero(&DM_addr, sizeof(struct sockaddr_un));
    DM_addr.sun_family = AF_LOCAL;
    unlink(UNIX_PATH_DM);
    strcpy(DM_addr.sun_path, UNIX_PATH_DM);

    if (bind(sockfd, (SA *)&DM_addr, sizeof(struct sockaddr_un)) != 0)
       handle_error("Bind");
    p_arg.udp_sockfd = sockfd;

    /* 创建子线程，用于接受广告信息并传递至LED */
    if (pthread_create(&p_arg.thread_id, NULL, &pthread_advrev, &p_arg) != 0)
        handle_error("Pthread");

    /* TODO: 向中心模块注册服务，获取目的模块的套接字地址 */

    /* set dest serv path in servaddr */
    bzero(&dest_addr, sizeof(struct sockaddr_un));
    dest_addr.sun_family = AF_LOCAL;
    strcpy(dest_addr.sun_path, UNIX_PATH_NM);

//    char sndbuf[BUFFSIZE];
//    strcpy(sndbuf, "Gps info is coming\n");
//    sendto(sockfd, sndbuf, BUFFSIZE, 0, (SA *)&servaddr, sizeof(servaddr));
 
    bzero(&dev_msg, sizeof(struct Net_Info));

    /* use epoll in later */
    while(1)
    {
        int rd_num = 0, flag;
        if ((rd_num = read(mcu_fd, revbuf, sizeof(revbuf))) < 0)
            handle_error("Read");
        else
            printf("\nRead %d character from ttyACM0\n", rd_num);
        flag = ttys_date_analysis(revbuf, rd_num, &dev_msg);
        switch(flag) {
            case S_NOGPS:
                printf("No complete GPS info\n");
                break;
            case S_INVALID:
                printf("Invalid GPS info\n");
                break;
            case S_ERROR:
                printf("NMEA convert error\n");
                break;
            case 0:
                printf("gps_time is %s", dev_msg.content.gpsinfo.gps_time);
                printf("sys time is %s", dev_msg.content.gpsinfo.sys_time);
                printf("gps_log  is %s", dev_msg.content.gpsinfo.longitude);
                printf("gps_lat  is %s", dev_msg.content.gpsinfo.latitude);
                sendto(sockfd, &dev_msg, sizeof(dev_msg), 0, (SA *)&dest_addr, sizeof(struct sockaddr_un));
                bzero(&dev_msg, sizeof(struct Net_Info));
                break;
        };
        sleep(1);
    }
    exit(0);
}

/*********************************************************************
 * 在接受到的串口数据中提取GPS数据,                                                 
 * case 1: 读取的数据中无完整的GPS信息，程序返回S_NOGPS
 * case 2: 读取到无效的GPS信息，程序返回S_INVALID
 * case 3: GPS信息转化失败，程序返回S_ERROR
 * case 4: 读取到有效的GPS信息，调用转换函数....
 ********************************************************************/
int ttys_date_analysis(const char *ttys_date, const size_t ttys_size, struct Net_Info *dev_msg)
{
    int     g_size = 0;
    int     i;
    int     nmea_err_flag = 0;      /* 用于标识NMEA转化失败状态 */
    int     nmea_invalid_flag = 0;  /* 用于标识GPS数据无效状态  */
    char gps_data[ttys_size];       /* 用于存储提取的GPS信息    */

    for(i = 0; i < ttys_size; ++i)
    {
        /* 判断帧头 */
        if (ttys_date[i] != 0x7e)
            continue;
        else {
        /* 获得帧头，接下来通过设备ID来判断数据类型 */
            if (ttys_date[i+2] != 0x05) {
                i = i + 3;
                continue;
            } else {    /* 处理GPS数据 */
                i = i + 4;  /* 从设备数据第一位开始读 */
                while((i < ttys_size)&(ttys_date[i] != 0x7f)) {
                    gps_data[g_size++] = ttys_date[i++];
                }
                if(i >= ttys_size) { /* 说明读到缓存区尾部尚未读到帧尾符 */
                    if (nmea_invalid_flag == 1)
                        return S_INVALID;
                    else if ((nmea_invalid_flag == 0)&nmea_err_flag)
                        return S_ERROR;
                    else 
                        return S_NOGPS;
                }
                else if (ttys_date[i] == 0x7f) {
                    gps_data[g_size] = 0x7f;
                    int flag;
                    flag = nmea_gps_convert(gps_data, g_size, dev_msg);
                    switch(flag) {
                        case NM_FAULT :
                            nmea_err_flag = 1;
                            g_size = 0; /* NMEA转化失败后读取下一帧继续尝试转化 */
                            continue;   
                        case NM_INVALID :
                            nmea_invalid_flag = 1;
                            g_size = 0;
                            continue;
                        case 0 :
                            return 0;
                            break;
                    };
                }
            }
        }
    }
    if (nmea_invalid_flag == 1)
        return S_INVALID;
    else if ((nmea_invalid_flag == 0)&nmea_err_flag)
        return S_ERROR;
    else 
        return S_NOGPS;
}

/**************************************************************************
 * 将从串口获取的nmea语义格式的gps信号转化为可用形式，存入Net_Info结构中
 * case 1: 转化失败，返回NM_FAULT
 * case 2: 获取的gps信号为无效信息，返回NM_INVALID
 * case 3: 获取gps信号成功并存入对应结构体，返回0
 **************************************************************************/
int nmea_gps_convert(const char *nmea_info, const size_t nmea_size, struct Net_Info *dev_msg)
{
    /* nmealib库初始化 */
    nmeaINFO info;
    nmeaGPGGA gpgga_info;
    nmeaPARSER parser;

    nmea_zero_INFO(&info);
    nmea_zero_GPGGA(&gpgga_info);
    if (!nmea_parser_init(&parser)) {
        printf("init error\n");
        return NM_FAULT;
    }
 
    if (!nmea_parse_GPGGA(nmea_info, (int)strlen(nmea_info), &gpgga_info)) {
        printf("GPGGA error\n");
        return NM_FAULT;
    }

    if (!nmea_parse(&parser, nmea_info, (int)strlen(nmea_info), &info)) {
        printf("parse error\n");
        return NM_FAULT;
    }

    if (!info.sig)
        return NM_INVALID;  /* 如果GPS信息无效，返回NM_INVALID */

    dev_msg->type = 2;  /* TYPE: GPS info */
    strcpy(dev_msg->senderID, "192.168.51.26");

    /* 将NMEA中的UTC时间转化为date-time形式，并转为GMT+8 */
    char gps_time[50];
    sprintf(gps_time, "%04d-%02d-%02d %02d:%02d:%02d\n",
                info.utc.year + 1900,
                info.utc.mon + 1,
                info.utc.day,
                info.utc.hour + 8,
                info.utc.min,
                info.utc.sec);
    printf("gps_time in utc is %s\n", gps_time);
    strcpy(dev_msg->content.gpsinfo.gps_time, gps_time);
 
    /* 获得纬度(latitude)信息，并将度分表达式转化为常见的以度为单位 */
    char lat_info[32];
    float lat_val = (info.lat/100 + (info.lat - (info.lat/100) * 100)/60);
    sprintf(lat_info, "%.7f%c\n", lat_val, gpgga_info.ns);
    strcpy(dev_msg->content.gpsinfo.latitude, lat_info);
    
    /* 获得当前系统时间 */
    char sys_time[50];
    Time_sys(sys_time);
    strcpy(dev_msg->content.gpsinfo.sys_time, sys_time);

    /* 获得经度(longitude)信息，并转化 */
    char lon_info[32];
    float lon_val = (info.lon/100 + (info.lon - (info.lon/100) * 100)/60);
    sprintf(lon_info, "%.7f%c\n", lon_val, gpgga_info.ew);
    strcpy(dev_msg->content.gpsinfo.longitude, lon_info);

    char altit_info[32];
    sprintf(altit_info, "%.7f\n", info.elv);
    strcpy(dev_msg->content.gpsinfo.altitude, altit_info);

    nmea_parser_destroy(&parser);
    return 0;
}

static void *pthread_advrev(void *arg)
{
    struct  pthread_arg *p_arg = arg;
    struct  Net_Info    *tmp;
    char    revbuf[BUFFSIZE];
    int     udp_sockfd = p_arg->udp_sockfd;
    printf("Pthread start\n");
    for ( ; ;) {
        int     n;
        if ((n = recvfrom(udp_sockfd, revbuf, BUFFSIZE, 0, NULL, NULL)) < 0)
            handle_error("Recvfrom");
        tmp = (struct Net_Info *)revbuf;
        printf("recv %d Bytes from serv_provider, msg is %s\n", n, tmp->content.adverinfo);
        led_serial_init();
        p4_led(tmp->content.adverinfo, strlen(tmp->content.adverinfo), ZN_FLAG);  
    }
    return(NULL);
}

