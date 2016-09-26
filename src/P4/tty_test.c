#include "include/p4net.h"
#include "include/p4basic.h"
#include <fcntl.h>
#include <termios.h>

int main(int argc, char** argv)
{
    int     tty_fd, flag, rd_num;
    struct  termios term;
    char    revbuf[1024*16];
    speed_t baud_rate_i, baud_rate_o;
    if ((tty_fd = open("/dev/ttyACM0", O_RDWR)) < 0)
        handle_error("Open");
    else
        printf("Open MCU_Serial ttyACM0 success, fd is %d\n", tty_fd);

    flag = tcgetattr(tty_fd, &term);
    baud_rate_i = cfgetispeed(&term);
    baud_rate_o = cfgetospeed(&term);
    printf("baudrate of in is: %d, out is %d\n",
                 baud_rate_i, baud_rate_o);
    
    //cfsetospeed(&term, B115200);
    //cfsetispeed(&term, B115200);

    /* set cflag for: Ignore modem control line & Enable receiver */
    //term.c_cflag |= (CLOCAL | CREAD);
    while(1) {
        int i;
        rd_num = 0;
        bzero(revbuf, sizeof(revbuf));
        rd_num = read(tty_fd, revbuf, sizeof(revbuf));
        if(rd_num >= 0) {
            printf("\n read: \n  %s  \nfrom ttyACM0, total %d characters\n",
                      revbuf, rd_num);
            for(i = 0; i < rd_num; ++i)
                printf("%02x ", revbuf[i]);
        }
        else
            handle_error("Read");
        sleep(3);
    }
    return 0;
}    
