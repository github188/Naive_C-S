/*
 * Author : wding @streamax
 * Date : Aug.25 '16
 * Description: initialize the server of TCP connection
 * return the listen socket if function runs succeed
 */
#include "../lib/p4net.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int serv_init(const char *host, const char *serv, socklen_t *addrlenp)
{
    int             listenfd, err;
    const  int      optval = 1;
    struct addrinfo *aip, *ailist;
    struct addrinfo hints;

    /* hint setting */
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family   = AF_UNSPEC;         /* set for IPv4 internet domin */
    hints.ai_flags    = AI_PASSIVE;      /* return a listen socket  */
    hints.ai_socktype = SOCK_STREAM;     /* TCP for default */

    /* transform the hostname and servicename into an address struct */
    if ( (err = getaddrinfo(host, serv, &hints, &ailist)) != 0) {
        syslog(LOG_ERR, "server: getaddrinfo error: %s", gai_strerror(err));
        exit(1);
    }

    aip = ailist;   /* get the header of the addr list */

    do {
        listenfd = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol);
        if (listenfd < 0)
            continue;           /* ignore this addr */
        /* socket option setting: addr_reuse */
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
            close(listenfd);    /* close the socket if fails */
            continue;
        }
        /* Then ,bind the address and socket */
        if (bind(listenfd, aip->ai_addr, aip->ai_addrlen) == 0) 
            break;              /* success, goto listen */ 
        
        close(listenfd);        /* bind error, close listenfd and try next addr */
    } while ( (aip = aip->ai_next) != NULL);

    /* errno from final addr */
    if (aip == NULL) 
        handle_error("serv_init");
    /* listen from the listen socket */    
    if ( (err = listen(listenfd, LISTENQ)) < 0) {
        close(listenfd);
        handle_error_en(err,"listen");
    }
    
    if (addrlenp)
        *addrlenp = aip->ai_addrlen;

    freeaddrinfo(aip);
    
    return(listenfd);
}
        

