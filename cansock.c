#include "globals.h"
#include <linux/can.h>
#include <linux/if.h>
#include <sys/ioctl.h>

int cansock;
int nbytes;

struct can_frame frame;

void cansock_sighand(int sig) {
    exit(EXIT_SUCCESS);
}


static void *cansock_run() {
    struct sockaddr_can addr;
    struct ifreq ifr;
    /* TODO:
        acceptable signal handling !!! */
    syslog(LOG_INFO, "cansock thread started");
    /* signal(SIGTERM, cansock_sighand); */

    /* register socket */
    cansock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(cansock < 0) {
        syslog(LOG_ERR, "was not able to init cansock");
        exit(EXIT_FAILURE);
    }

    strcpy(ifr.ifr_name, "can0" );
    ioctl(cansock, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    bind(cansock, (struct sockaddr *)&addr, sizeof(addr));


    while(1) {
        nbytes = read(cansock, &frame, sizeof(struct can_frame));
        if(nbytes < 0) {
            /* nothing */
        }
        else if (nbytes == sizeof(struct can_frame)) {
            printf("+ id: %X l: %d data: %X %X %X %X %X %X %X %X \n",
                (frame.can_id & CAN_SFF_MASK), frame.can_dlc,
                frame.data[0], frame.data[1], frame.data[2], frame.data[3],
                frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
        }
    }
    return 0;
}
