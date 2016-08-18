#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>

#define BUFLEN 128
#define MAXSLEEP 128

#define oops(msg)   { perror(msg), exit(1); }

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
    int     n;
    char    buf[BUFLEN];
    
    while ((n = recv(sockfd, buf, BUFLEN, 0)) > 0)
        write(STDOUT_FILENO, buf, n);
    if (n < 0) {
        printf("recv error");
        exit(-1);
    }
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
                err = errno;
        } else {
            serv_req(sockfd);
            exit(0);
        }
    }
    oops("can't connect to the server");
}

