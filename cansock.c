#include "cansock.h"
int cansock;
int nbytes;

struct can_frame frame;
struct isotp_frame isoframe;

void cansock_sighand(int sig) {
    exit(EXIT_SUCCESS);
}


void *cansock_run() {
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

    /* init ISOTP lib */
    isotp_init();


    while(1) {
        nbytes = read(cansock, &frame, sizeof(struct can_frame));
        if(nbytes < 0) {
            /* nothing */
        }
        else if (nbytes == sizeof(struct can_frame)) {
            /* printf("+ id: %X l: %d data: %X %X %X %X %X %X %X %X \n",
                (frame.can_id & CAN_SFF_MASK), frame.can_dlc,
                frame.data[0], frame.data[1], frame.data[2], frame.data[3],
                frame.data[4], frame.data[5], frame.data[6], frame.data[7]); */
            int status = isotp_compute_frame(&frame);
            if(status > 0) {
                if(isotp_get_frame(&isoframe)) {
                    printf("+ iso send:%x rec:%x l:%d data: %x %x %x %x %x %x \n",
                        isoframe.sender, isoframe.rec, isoframe.dl,
                        isoframe.data[0], isoframe.data[1], isoframe.data[2],
                        isoframe.data[3], isoframe.data[4], isoframe.data[5]);
                }
            }
            else if (status < 0) {
                /* no ISO-TP Frame */
            }
            else {
                /* ISO-TP msg still in transmission */
            }
        }
    }
    return 0;
}
