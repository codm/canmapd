#include "globals.h"
#include "isotprecv.h"

extern int cansocket, websocket;
int nbytes;

struct can_frame frame;
struct isotp_frame isoframe;

void isotprecv_sighand(int sig) {
    syslog(LOG_INFO, "%s", "can - SIGTERM - shutting down");
    close(cansocket);
    closelog();
    exit(EXIT_SUCCESS);
}


void *isotprecv_run(void) {
    /* TODO:
        acceptable signal handling !!! */
    syslog(LOG_INFO, "canreceive process started");
    signal(SIGTERM, isotprecv_sighand);

    isotp_init();


    while(1) {
        nbytes = read(cansocket, &frame, sizeof(struct can_frame));
        if(nbytes < 0) {
            /* nothing */
        }
        else if (nbytes == sizeof(struct can_frame)) {
            int status;
            printf("+ id: %X l: %d data: %X %X %X %X %X %X %X %X \n",
                (frame.can_id & CAN_SFF_MASK), frame.can_dlc,
                frame.data[0], frame.data[1], frame.data[2], frame.data[3],
                frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
            status = isotp_compute_frame(&frame);
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
