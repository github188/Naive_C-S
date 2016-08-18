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
#define MAXLINE 4096
#define QLEN    10

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

#define oops(msg)   { perror(msg), exit(1); }

void str_echo(int sockfd)
{
    ssize_t     n;
    char        buf[BUFLEN];

again:
    while ( (n = read(sockfd, buf, BUFLEN)) > 0) {
        printf("message get");
        write(STDOUT_FILENO, buf, n);
        write(sockfd, buf, n);
    }
    if (n < 0 && errno == EINTR)
        goto again;
    else if (n < 0)
        oops("read");
}


int initserver(int type, const struct sockaddr *addr, 
            socklen_t alen, int qlen)
{   
    int listenfd, err;
    int reuse = 1;

    if ((listenfd = socket(addr->sa_family, type, 0)) < 0)
        return(-1);
    printf("socket succeed, listenfd is: %d\n", listenfd);
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
            sizeof(int)) < 0)
        oops("setsockopt");
    if (bind(listenfd, addr, alen) < 0)
        oops("bind");
    printf("bind succeed!");
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        if (listen(listenfd, qlen) < 0)
            goto errout;
    return(listenfd);

errout:
    err = errno;
    close(listenfd);
    errno = err;
    return(-1);
}

void serve(int listenfd)
{
    printf("listenfd is %d\n", listenfd);
    printf("come on wding\n");

    int             connfd;
    int             childpid;
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
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &alen)) < 0) {
            syslog(LOG_ERR, "serv: accept error: %s", strerror(errno));
            exit(1);
        } 
        //printf("connection from where?\n");
        
        if ( (childpid = fork()) == 0) { /* child process */
            close(listenfd);
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
            write(connfd, buff, strlen(buff));
            //close(connfd);
            printf("reading from client...");
            str_echo(connfd);
            exit(0);
        }
        close(connfd);
    }

}

int main(void)
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int             listenfd, err, n;
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
        if ((listenfd = initserver(SOCK_STREAM, aip->ai_addr, 
                    aip->ai_addrlen, QLEN)) >= 0) {
            serve(listenfd);
            exit(0);
        }
    }
    exit(1);
}

