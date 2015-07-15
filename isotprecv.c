#include "globals.h"
#include "isotprecv.h"

extern int cansocket;
int websocket;
int nbytes;

struct can_frame frame;
struct isotp_frame isoframe, isoframe2;

void isotprecv_sighand(int sig) {
    syslog(LOG_INFO, "%s", "can - SIGTERM - shutting down");
    close(cansocket);
    closelog();
    exit(EXIT_SUCCESS);
}


void *isotprecv_run(void) {
    /* TODO:
        acceptable signal handling !!! */
    struct sockaddr_in webclient, webserv;
    char sock_send[9000], sock_recv[8];
    syslog(LOG_INFO, "receive process started");
    signal(SIGTERM, isotprecv_sighand);

    isotp_init();
    /*
       WEBSOCKET
    */
    websocket = socket(AF_INET, SOCK_STREAM, 0);
    if(websocket < 0) {
        syslog(LOG_ERR, "was not able to initiate websocket");
        exit(EXIT_FAILURE);
    }

    memset(&webserv, 0, sizeof(webserv));
    webserv.sin_family = AF_INET;
    webserv.sin_addr.s_addr = inet_addr("127.0.0.1");
    webserv.sin_port = htons(25005);

    while(1) {
        nbytes = read(cansocket, &frame, sizeof(struct can_frame));
        if(nbytes < 0) {
            /* nothing */
        }
        else if (nbytes == sizeof(struct can_frame)) {
            int status;
            status = isotp_compute_frame(&cansocket, &frame);
            if(status == ISOTP_COMPRET_COMPLETE) {
                if(isotp_get_frame(&isoframe)) {
                    isotp_fr2str(sock_send, &isoframe);
                    /* printf("%s\n", printarray); */
                    if(connect(websocket, (struct sockaddr*)&webserv, sizeof(webserv))) {
                        send(websocket, sock_send, strlen(sock_send), 0);
                        recv(websocket, sock_recv, sizeof(sock_recv), 0);
                        close(websocket);
                        printf("answer: %s\n", sock_recv);
                    }
                }
            }
            else if(status == ISOTP_COMPRET_ERROR) {
                printf("ERROR id: %X l: %d data: %.*x \n",
                    (frame.can_id & CAN_SFF_MASK), frame.can_dlc,
                    8, frame.data[0]);
                /* ISO-TP msg still in transmission */
            }
        }
    }
    return 0;
}
