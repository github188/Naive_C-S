#include "p4net.h"
#include "p4basic.h"

int main(int argc, const char **argv)
{
    int     sockfd, n;
    struct  sockaddr_un servaddr, cliaddr;
    char sndbuf[BUFFSIZE] = "domain unix works";

    if ((sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
        handle_error("socket");

    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    strcpy(cliaddr.sun_path, tmpnam(NULL));

    if (bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr)) != 0)
        handle_error("bind");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, UNIX_PATH_NM);

    if ((n = sendto(sockfd, sndbuf, BUFFSIZE, 0, (SA *) &servaddr, sizeof(servaddr))) < 0)
        handle_error("Sendto");
    printf("%d bytes has sent\n", n);
    exit(0);
}
