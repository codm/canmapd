#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/if.h>
#include <netinet/in.h>

#include "main.h"
#include "canblocks.h"

int process_connection(int socket);
void sig_term(int sig);
void print_helptext();
int process_connection();
void *can2tcp(void *arg);
void *canblocks_gc(void *arg);

pid_t pid, sid, connection;

struct connection_data {
    int cansocket;
    int websocket;
    struct sockaddr_in webclient;
    struct sockaddr_in webserver;
    pthread_mutex_t cansocket_mutex;
};

struct connection_data conn;

const char DAEMON_NAME[] = "canblocksd";
const char DAEMON_VERSION[] = "0.2";

void sig_term(int sig) {
    /* shutting down program properly */
    syslog(LOG_INFO, "%s", "Sigterm received - shutting down");

    /* send sigterm to children */
    kill(0, SIGTERM);

    close(conn.cansocket);
    close(conn.websocket);

    /* closing log */
    syslog(LOG_INFO, "%s", "Shutdown complete");
    closelog();

    exit(EXIT_SUCCESS);
}

void print_helptext() {
    printf("%s Version %s\n", DAEMON_NAME, DAEMON_VERSION);
    printf("Built %s %s\n", __DATE__, __TIME__);
    printf("(c) cod.m GmbH\n");
    printf("    --verbose (-v) Prints additional output to stdout\n");
    printf("    --device  (-D) sets can device\n");
    printf("    --port    (-p) Port where daemon listens\n");
    printf("    --daemon  (-d) start as a daemon\n");
    printf("    --filter  (-h) set filter for incoming messages (default 0)\n");
    printf("    --help    (-h) prints this helptext\n");
    exit(EXIT_SUCCESS);
}

/*
    main loop
*/
int main(int argc, const char* argv[]) {
    /* init vars */
    struct sockaddr_in webclient, webserv;
    socklen_t len;
    int newsock, i;

    /*
       run code
    */
    /* static init for CLP */
    verbose = 0;
    run_daemon = 0;
    virtualcan = 0;
    rec_filter = 0;
    listenport = "25005";
    device = "can0";
    rec_filter = 0x00;
    for(i=0; i < argc; i++) {
        if(!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v")) {
            verbose = 1;
        }
        else if (!strcmp(argv[i], "--daemon") || !strcmp(argv[i], "-d")) {
            run_daemon = 1;
        }
        else if (!strcmp(argv[i], "--device") || !strcmp(argv[i], "-V")) {
            i++;
            device = argv[i];
        }
        else if (!strcmp(argv[i], "--port") || !strcmp(argv[i], "-p")) {
            i++;
            listenport = argv[i];
        }
        else if (!strcmp(argv[i], "--filter") || !strcmp(argv[i], "-f")) {
            i++;
            rec_filter = (uint8_t)strtol(argv[i], NULL, 10);
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            i++;
            print_helptext();
            return 0;
        }
    }

    /*
       register signal handlers
    */
    if(run_daemon) {
        /* fork off parent */
        pid = fork();
        /* not good */
        if(pid < 0) {
            printf("[daemon] error! exiting\n");
            exit(EXIT_FAILURE);
        }
        /* good */
        if(pid > 0) {
            printf("[daemon] forked into pid %d\n", pid);
            exit(EXIT_SUCCESS);
        }
    }

    /*
       set filemode creation mask to 0777
    */
    umask(0);

    /*
        open syslog for daemon
    */
    openlog(DAEMON_NAME, 0, LOG_USER);

    if(run_daemon) {
        sid = setsid();
        if(sid < 0) {
            syslog(LOG_ERR, "%s", "was not able to get session id");
            exit(EXIT_FAILURE);
        }
    }

    /*
       WEBSOCKET
    */
    conn.websocket = socket(AF_INET, SOCK_STREAM, 0);
    if(conn.websocket < 0) {
        syslog(LOG_ERR, "was not able to initiate websocket");
        exit(EXIT_FAILURE);
    }
    /* bind server */
    memset(&webserv, 0, sizeof(webserv));
    webserv.sin_family = AF_INET;
    webserv.sin_addr.s_addr = inet_addr("127.0.0.1");
    webserv.sin_port = htons(atoi(listenport));
    if(bind(conn.websocket, (struct sockaddr*)&webserv, sizeof(webserv)) < 0) {
        syslog(LOG_ERR, "was not able to bind webserver");
        exit(EXIT_FAILURE);
    }
    if(listen(conn.websocket, 9) < 0) {
        syslog(LOG_ERR, "not able to register listen");
        exit(EXIT_FAILURE);
    }

    pid = getpid();

    /* install sighandle for main process */
    signal(SIGTERM, sig_term);
    signal(SIGINT, sig_term);
    syslog(LOG_INFO, "%s (%s) started", DAEMON_NAME, DAEMON_VERSION);
    if(verbose) {
        printf("%s (%s) started %d\n", DAEMON_NAME, DAEMON_VERSION, (int)pid);
        printf("ip:port %s:%d\n", inet_ntoa(webserv.sin_addr), ntohs(webserv.sin_port));
        printf("receive: %d\n", rec_filter);
    }
    len = sizeof(webclient);
    while(1) {
        newsock = accept(conn.websocket, (struct sockaddr*)&webclient, &len);
        if(newsock > 0) {
            connection = fork();
            if(connection == 0) {
                /* child */
                pid = getpid();
                if(verbose)
                    printf("connection from %s forked into pid %d\n",
                            inet_ntoa(webclient.sin_addr), (int)pid);
                conn.webclient = webclient;
                conn.webserver = webserv;
                process_connection(newsock);
                exit(EXIT_SUCCESS);
            }
            else if(connection < 0) {
                /* error */
                syslog(LOG_ERR, "was not able to create process for connection");
            }
            /* parent */
            close(newsock);
        }
    }
    syslog(LOG_INFO, "%s", "daemon successfully shut down");
    return 0;
}

int process_connection(int websock) {
    int cansocket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct canblocks_frame sendframe;
    pthread_t can2tcpthread, cangc;
    char webbuff[WEBSOCK_MAX_RECV];
    int webbuffsize, running;

    /* open CAN Socket */
    /*

       CANSOCKET
    */
    cansocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(cansocket < 0) {
        syslog(LOG_ERR, "was not able to init cansock");
        exit(EXIT_FAILURE);
    }
    strcpy(ifr.ifr_name, device);
    ioctl(cansocket, SIOCGIFINDEX, &ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(cansocket, (struct sockaddr *)&addr, sizeof(addr));

    conn.cansocket = cansocket;
    conn.websocket = websock;

    /* initialize websocket mutex */
    pthread_mutex_init(&(conn.cansocket_mutex), NULL);
    /* open thread for can_send */
    if(0 != pthread_create(&can2tcpthread, NULL, can2tcp, &conn)) {
        perror("canthread");
        return 0;
    }
    /* open thread for canblocks_gc */
    if(0 != pthread_create(&cangc, NULL, canblocks_gc, &conn)) {
        perror("can_gc_thread");
        return 0;
    }
    running = 1;
    write(conn.websocket, "> hi\n", 6);
    while(running) {
        if((webbuffsize = recv(conn.websocket, webbuff, WEBSOCK_MAX_RECV,0)) < 0)
            printf("Fehler in websock recv");
        webbuff[webbuffsize] = '\0';
        if(strcmp("<exit>\n", webbuff) == 0) {
            running = 0;
        }
        if(canblocks_str2fr(webbuff, &sendframe) > 0) {
            /* printf("msg: %s \n", webbuff); */

            /* block websocket */
            pthread_mutex_lock(&conn.cansocket_mutex);
            /* send frame */
            canblocks_send_frame(&cansocket, &sendframe);
            /* unblock websocket */
            pthread_mutex_unlock(&conn.cansocket_mutex);
            /* reset sendframe */
            canblocks_reset_frame(&sendframe);
        };
        /* send(conn.websocket, webbuff, strlen(webbuff), 0); */
    }

    /* close */
    close(conn.cansocket);
    close(conn.websocket);
    return 1;
}

void *can2tcp(void *arg) {
    int nbytes;
    struct connection_data* conn;
    char sock_send[9000];
    struct can_frame frame;
    struct canblocks_frame isoframe;
    struct timespec waittime;
    waittime.tv_sec = 0;
    waittime.tv_nsec = 10000000L;

    canblocks_init();
    /* empty */
    conn = (struct connection_data*)arg;
    while(1) {
        nanosleep(&waittime, NULL);
        memset(sock_send, 0, sizeof(sock_send));
        /* if websocket is free */
        if(pthread_mutex_trylock(&conn->cansocket_mutex) != EBUSY) {
            nbytes = recv(conn->cansocket, &frame, sizeof(struct can_frame), MSG_DONTWAIT);
            if(nbytes < 0) {
                /* nothing */
            }
            else if (nbytes == sizeof(struct can_frame)) {
                int status;
                status = canblocks_compute_frame(&(conn->cansocket), &frame);
                if(status == CANBLOCKS_COMPRET_COMPLETE) {
                    if(canblocks_get_frame(&isoframe)) {
                        canblocks_fr2str(sock_send, &isoframe);
                        /* printf("%s\n", printarray); */
                        send(conn->websocket, sock_send, strlen(sock_send), 0);
                        canblocks_reset_frame(&isoframe);
                    }
                }
                else if(status == CANBLOCKS_COMPRET_ERROR) {
                    /* msg still in transmission */
                }
            }
            pthread_mutex_unlock(&conn->cansocket_mutex);
        }
    }
}

/*
    Cleans up unused fields in canblocks data structure.
    */
void *canblocks_gc(void *arg) {
    struct timespec waittime;
    struct connection_data* conn;
    int id;

    waittime.tv_sec = CANBLOCKS_GC_REFRESH;
    waittime.tv_nsec = 0;
    conn = (struct connection_data*)arg;
    char sock_send[512];

    while(1) {
        nanosleep(&waittime, NULL);
        id = canblocks_clean_garbage();
        if(id >= 0) { /* gc happened */
            sprintf(sock_send, "> [error] buffer reset in field %d\n", id);
            send(conn->websocket, sock_send, strlen(sock_send), 0);
        }
    }
}
