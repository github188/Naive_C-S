#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

#define BUFLEN  128
#define QLEN    10

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

#define oops(msg)   { perror(msg), exit(1); }

int initserver(int type, const struct sockaddr *addr, 
            socklen_t alen, int qlen)
{   
    int fd, err;
    int reuse = 1;

    if ((fd = socket(addr->sa_family, type, 0)) < 0)
        return(-1);
    printf("socket succeed, sockfd is: %d\n", fd);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
            sizeof(int)) < 0)
        oops("setsockopt");
    if (bind(fd, addr, alen) < 0)
        oops("bind");
    printf("bind succeed!");
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        if (listen(fd, qlen) < 0)
            goto errout;
    return(fd);

errout:
    err = errno;
    close(fd);
    errno = err;
    return(-1);
}

void serve(int sockfd)
{
    printf("sockfd is %d\n", sockfd);
    printf("come on wding\n");

    int             clfd;
    socklen_t       alen, blen;
    struct  addrinfo cliaddr;
    //struct  sockaddr_in cliaddr;
    char    buff[BUFLEN];
    char    clihost[BUFLEN];
    char    cliserv[BUFLEN];

    for (;;) {
        alen = sizeof(cliaddr);
        printf("alen is: %d\n",alen);
        //memset(&cliaddr, 0, len);
        if ((clfd = accept(sockfd, (struct sockaddr *)&cliaddr, &alen)) < 0) {
            syslog(LOG_ERR, "serv: accept error: %s", strerror(errno));
            exit(1);
        } 
        printf("connection from where?\n");
        //getnameinfo() inverse of getaddrinfo
        if ((getnameinfo((struct sockaddr *)&cliaddr, alen,
                        clihost, BUFLEN, cliserv, BUFLEN, NI_NUMERICSERV||NI_NUMERICHOST)) != 0)
            oops("getnameinfo");
        //blen = snprintf(buff, sizeof(buff), "connection from: ", clihost, ", port: ", cliserv);
        if((blen = snprintf(buff, sizeof(buff), "connection from %s, port: %s\n", 
                    clihost, cliserv)) < 0)
                oops("snprintf");
        printf(buff, blen);
        //printf("connection from %s, port %s\n",clihost ,cliserv);
        write(clfd, buff, strlen(buff));

        close(clfd);
    }

}

int main(void)
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int             sockfd, err, n;
    char            *host;

    if ((n = sysconf(_SC_HOST_NAME_MAX)) < 0)
        n = HOST_NAME_MAX;
    if ((host = malloc(n)) == NULL)
        oops("malloc");
    if (gethostname(host, n) < 0)
        oops("gethostname");

    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_CANONNAME;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(host, "16000", &hint, &ailist)) != 0) {
        syslog(LOG_ERR, "serv: getaddrinfo error: %s", gai_strerror(err));
        exit(1);
    }

    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = initserver(SOCK_STREAM, aip->ai_addr, 
                    aip->ai_addrlen, QLEN)) >= 0) {
            serve(sockfd);
            exit(0);
        }
    }
    exit(1);
}

