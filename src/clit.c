#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include "../lib/p4net.h"

#define BUFLEN      128
#define MAXSLEEP    128

//#define oops(msg)   { perror(msg), exit(1); }

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;
    ptr = vptr;
    for(n = 1; n < maxlen; n++){
        again:
            if((rc = read(fd, &c, 1)) == 1){
                *ptr++ = c;
                if(c == '\n')
                    break;
            }else if(rc == 0){
                *ptr = 0;
                return (n-1);
            }else{
                if(errno == EINTR)
                    goto again;
                return -1;
            }
    }
    *ptr = 0;
    return n;
}

void str_cli(FILE *fp, int sockfd)
{
    char    sendline[MAXLINE], recvline[MAXLINE];
    
    while (fgets(sendline, MAXLINE, fp) != NULL) {
        write(sockfd, sendline, strlen(sendline));

        if (readline(sockfd, recvline, MAXLINE) == 0)
            oops("client readline");

        fputs(recvline, stdout);
    }
}

/* try to connect with exponenital backoff */
int connect_retry(int domain, int type, int protocol,
                const struct sockaddr *addr, socklen_t alen)
{
    int numsec, fd;
    
    for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
        if ((fd = socket(domain, type, protocol)) < 0)
            oops("socket");
        
        if (connect(fd, addr, alen) == 0) {
            /* connection accepted */
            return(fd);
        }
        close(fd);

        /* delay before re-connect */
        if (numsec <= MAXSLEEP/2)
            sleep(numsec);
    }
    return(-1);
}

void serv_req(int sockfd)
{
//    int     n;
//    char    buf[BUFLEN];
    
    struct CS_MsgInfo
    {
//        char    type;               /* info type, 'd' for daily, 'g' for GPS */
        char    send_ID[20];        /* ID of sender */
        size_t  info_length;        /* length of info content */
        char    info_content[1024]; /* info content */
    };

    // struct CS_MsgInfo msginfo;
    struct Net_Info P4MsgInfo;
    size_t slen = sizeof(P4MsgInfo);
    memset(&P4MsgInfo, 0, slen);
    printf("Enter the content you wanna to send\n");
    //msginfo.send_ID = "192.168.37.130";   /* wrong method */
    strcpy(P4MsgInfo.senderID, "192.168.37.130");

    if ( (P4MsgInfo.info_length = read(STDIN_FILENO, 
                   P4MsgInfo.info_content, 1024) - 1) < 0)
       oops("read stdin");
    
    char *snd_buf = (char*)malloc(slen);
    memset(snd_buf, 0, slen);
    memcpy(snd_buf, &P4MsgInfo, slen);
    
    int pos = 0, len = 0;
    while (pos < slen)
    {
        if ( (len = send(sockfd, snd_buf, slen, 0)) <= 0)
            oops("struct send");
        pos += len;
    }
    free(snd_buf);
    close(sockfd);
    printf("Send over");
    
    exit(0);
}

    //send(sockfd, snf_buf, slen, 0);
    
    //str_cli(stdin, sockfd);
//    while ((n = recv(sockfd, buf, BUFLEN, 0)) > 0)
//        write(STDOUT_FILENO, buf, n);
//    if (n < 0) {
//        printf("recv error");
//        exit(-1);
//    }
    //printf("recv succeed");
    //str_cli(stdin, sockfd);

void serv_test(int sockfd)
{   
    int     n;
    char    buf[MAXLINE];
    char    str[MAXLINE] = "talk is cheap";

    do {
        write(sockfd, str, strlen(str));
        sleep(5);
    } while(1);

    close(sockfd);
    exit(0);
}

int main(int argc, char *argv[])
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int             sockfd, err;

    if (argc != 3) {
        printf("usage: ServerInfo Needed");
        exit(-1);
    }
    memset(&hint, 0, sizeof(hint));
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(argv[1], argv[2], &hint, &ailist)) != 0)
        syslog(LOG_ERR, "client: getaddrinfo error: %s\n", 
                gai_strerror(err));
    
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = connect_retry(aip->ai_family, SOCK_STREAM, 0,
            aip->ai_addr, aip->ai_addrlen)) < 0) {
                continue;
        } else {
            serv_test(sockfd);
            exit(0);
        }
    }

    if (aip == NULL)
        handle_error("Cannot connect to the server");
}

