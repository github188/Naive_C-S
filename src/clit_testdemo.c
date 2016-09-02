#include <../lib/p4net.h>
#include <../lib/p4basic.h>

#define MAXN    16384

int main(int argc, char **argv)
{
    int     i ,j;   /* int only for loop */
    int     sockfd, nchildren, nloops, nbytes;
    pid_t   pid;
    ssize_t n;
    char    request[MAXLINE], reply[MAXN];

    if (argc != 6) {
        printf("usage: client <hostname or IPaddr> <port> <#children> "
                "<#loops/child> <#bytes/request>");
    }
    
    nchildren = atoi(argv[3]);
    nloops    = atoi(argv[4]);
    nbytes    = atoi(argv[5]);

    snprintf(request, sizeof(request), "%d\n", nbytes);  /* newline at end */

    for (i = 0; i < nchildren; i++)
        if ( (pid = fork()) == 0) { /* child process */
            sockfd = 

    
