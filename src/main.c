#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <sys/wait.h>

#include "main.h"
#include "canmap.h"

uint8_t verbose; /* defines if program runs in verbose mode */
uint8_t run_daemon; /* defines if program runs in daemon mode */
uint8_t virtualcan; /* defines if virtual can vcan0 is used */
uint8_t rec_filter; /* fiter ID for receiving stuff */
const char *listenport; /* port where daemon listen for messages TCP->CAN */
const char *device; /* CAN device */

int process_connection(int websocket);
void sig_term(int sig);
void print_helptext();
void * can2tcp(void *arg);
void * canmap_gc(void *arg);

pid_t pid, sid, connection;

struct connection_data {
    pthread_mutex_t canlock;
    int cansocket;
    int websocket;
    struct sockaddr_in webclient;
    struct sockaddr_in webserver;
};

struct connection_data conn;

const char DAEMON_NAME[] = "canmapd";
const char DAEMON_VERSION[] = "0.3";

void log_and_print(const int priority, const char *msg) {
    printf("%s\n", msg);
    syslog(priority, "%s", msg);
}

void sig_chld(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void sig_term(int sig) {
    /* shutting down program properly */
    log_and_print(LOG_INFO, "Sigterm received - shutting down");

    /* send sigterm to children */
    kill(0, SIGTERM);

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
    printf("    --filter  (-f) set filter for incoming messages (default 0)\n");
    printf("    --help    (-h) prints this helptext\n");
    exit(EXIT_SUCCESS);
}

/*
    main loop
*/
int main(const int argc, const char *argv[]) {
    /* init vars */
    struct sockaddr_in webclient, webserv;
    /*
       run code
    */
    /* static init for CLP */
    verbose = 0;
    run_daemon = 0;
    virtualcan = 0;
    rec_filter = 0;
    listenport = "25025";
    device = "can0";
    rec_filter = 0x00;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v")) {
            verbose = 1;
        }
        else if (!strcmp(argv[i], "--daemon") || !strcmp(argv[i], "-d")) {
            run_daemon = 1;
        }
        else if (!strcmp(argv[i], "--device") || !strcmp(argv[i], "-D")) {
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
            print_helptext();
            return 0;
        }
    }

    signal(SIGCHLD, sig_chld);

    /*
       register signal handlers
    */
    if (run_daemon) {
        /* fork off parent */
        pid = fork();
        /* not good */
        if (pid < 0) {
            printf("[daemon] error! exiting\n");
            exit(EXIT_FAILURE);
        }
        /* good */
        if (pid > 0) {
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

    if (run_daemon) {
        sid = setsid();
        if (sid < 0) {
            log_and_print(LOG_ERR, "was not able to get session id");
            exit(EXIT_FAILURE);
        }
    }

    /**
     * Mutex
     **/
    pthread_mutex_init(&conn.canlock, NULL);

    /*
       WEBSOCKET
    */
    conn.websocket = socket(AF_INET, SOCK_STREAM, 0);
    if (conn.websocket < 0) {
        log_and_print(LOG_ERR, "was not able to initiate websocket");
        exit(EXIT_FAILURE);
    }
    /* bind server */
    const int opt = 1;
    setsockopt(conn.websocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&webserv, 0, sizeof(webserv));
    webserv.sin_family = AF_INET;
    webserv.sin_addr.s_addr = inet_addr("127.0.0.1");
    webserv.sin_port = htons(atoi(listenport));
    if (bind(conn.websocket, (struct sockaddr*)&webserv, sizeof(webserv)) < 0) {
        log_and_print(LOG_ERR, "was not able to bind webserver");
        exit(EXIT_FAILURE);
    }
    if (listen(conn.websocket, 9) < 0) {
        log_and_print(LOG_ERR, "not able to register listen");
        exit(EXIT_FAILURE);
    }

    pid = getpid();

    /* install sighandle for main process */
    signal(SIGTERM, sig_term);
    signal(SIGINT, sig_term);
    char buffer[256];
    snprintf(buffer, sizeof(buffer),"%s (%s - built %s %s) started", DAEMON_NAME, DAEMON_VERSION, __DATE__, __TIME__);
    log_and_print(LOG_INFO, buffer);

    if (verbose) {
        printf("%s (%s - built %s %s) started %d\n", DAEMON_NAME, DAEMON_VERSION, __DATE__, __TIME__, (int)pid);
        printf("ip:port %s:%d\n", inet_ntoa(webserv.sin_addr), ntohs(webserv.sin_port));
        printf("receive: %d\n", rec_filter);
    }
    socklen_t len = sizeof(webclient);
    while (1) {
        const int newsock = accept(conn.websocket, (struct sockaddr*)&webclient, &len);
        if (newsock > 0) {
            connection = fork();
            if (connection == 0) {
                /* child */
                pid = getpid();
                if (verbose) {
                    printf("connection from %s forked into pid %d\n", inet_ntoa(webclient.sin_addr), (int)pid);
                }
                conn.webclient = webclient;
                conn.webserver = webserv;
                process_connection(newsock);
                exit(EXIT_SUCCESS);
            }
            /* error */
            log_and_print(LOG_ERR, "was not able to create process for connection");
            close(newsock);
        }
    }
}

int process_connection(const int websocket) {
    int cansocket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct canmap_frame sendframe;
    pthread_t can2tcpthread, cangc;
    char webbuff[WEBSOCK_MAX_RECV];

    /* open CAN Socket */
    /*

       CANSOCKET
    */
    cansocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (cansocket < 0) {
        log_and_print(LOG_ERR, "was not able to init cansock");
        exit(EXIT_FAILURE);
    }
    strcpy(ifr.ifr_name, device);
    ioctl(cansocket, SIOCGIFINDEX, &ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    const int bound = bind(cansocket, (struct sockaddr*)&addr, sizeof(addr));
    if (bound < 0) {
        printf("Could not bind can socket for connection\n");
        exit(EXIT_FAILURE);
    }

    conn.cansocket = cansocket;
    conn.websocket = websocket;

    /* initialize websocket mutex */
    /* open thread for can_send */
    if (0 != pthread_create(&can2tcpthread, NULL, can2tcp, &conn)) {
        perror("canthread");
        return 0;
    }
    /* open thread for canmap_gc */
    if (0 != pthread_create(&cangc, NULL, canmap_gc, &conn)) {
        perror("can_gc_thread");
        return 0;
    }
    int running = 1;
    while (running) {
        ssize_t webbuffsize = recv(conn.websocket, webbuff, WEBSOCK_MAX_RECV, MSG_PEEK);
        if (webbuffsize < 0) {
            printf("error in websock recv\n");
            running = 0;
        }
        else if (webbuffsize == 0) {
            printf("Client disconnected... shutdown\n");
            running = 0;
        }
        else {
            webbuffsize = recv(conn.websocket, webbuff, WEBSOCK_MAX_RECV, 0);
            webbuff[webbuffsize] = '\0';
            if (strcmp("<exit>\n", webbuff) == 0) {
                running = 0;
            }
            if (canmap_str2fr(webbuff, &sendframe) > 0) {
                pthread_mutex_lock(&(conn.canlock));
                canmap_send_frame(&cansocket, &sendframe);
                if (verbose) {
                    printf("[Master -> Slave] send msg: %s\n", webbuff);
                }
                canmap_reset_frame(&sendframe);
                pthread_mutex_unlock(&(conn.canlock));
            }
        }
    }

    /* close */
    close(conn.cansocket);
    close(conn.websocket);
    return 1;
}

void * can2tcp(void *arg) {
    /* experimental for second receiving socket */
    int cansocket;
    struct sockaddr_can addr = {0};
    struct ifreq ifr = {0};
    struct can_filter rfilter = {0};

    char sock_send[10000] = {0};
    struct can_frame frame = {0};
    struct canmap_frame isoframe = {0};

    /* experimental for second receiving socket */
    cansocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (cansocket < 0) {
        log_and_print(LOG_ERR, "was not able to init cansock");
        exit(EXIT_FAILURE);
    }
    strcpy(ifr.ifr_name, device);
    ioctl(cansocket, SIOCGIFINDEX, &ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    rfilter.can_id = rec_filter;
    rfilter.can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_SFF_MASK);
    setsockopt(cansocket, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
    const int bound = bind(cansocket, (struct sockaddr*)&addr, sizeof(addr));
    if (bound < 0) {
        printf("Could not bind can socket");
        exit(EXIT_FAILURE);
    }

    canmap_init();
    /* empty */
    struct connection_data *conn = arg;
    while (1) {
        /* if websocket is free */
        pthread_mutex_lock(&(conn->canlock));
        const ssize_t nbytes = recv(cansocket, &frame, sizeof(struct can_frame), 0);
        if (nbytes == sizeof(struct can_frame)) {
            const int status = canmap_compute_frame(&(cansocket), &frame);
            if (status == CANMAP_COMPRET_COMPLETE) {
                if (canmap_get_frame(&isoframe)) {
                    memset(sock_send, 0, sizeof(sock_send));
                    canmap_fr2str(sock_send, &isoframe);
                    if (verbose) {
                        printf("[Slave -> Master] send msg: %s", sock_send);
                    }
                    send(conn->websocket, sock_send, strlen(sock_send), 0);
                    canmap_reset_frame(&isoframe);
                }
            }
            else if (status == CANMAP_COMPRET_ERROR) {
                /* msg still in transmission */
            }
        }
        pthread_mutex_unlock(&(conn->canlock));
    }
}

/*
    Cleans up unused fields in canmap data structure.
    */
void * canmap_gc(void *arg) {
    struct connection_data *conn = arg;
    char sock_send[512];
    struct timespec waittime;
    waittime.tv_sec = CANMAP_GC_REFRESH;
    waittime.tv_nsec = 0;

    while (1) {
        nanosleep(&waittime, NULL);
        const int id = canmap_clean_garbage();
        if (id >= 0) {
            /* gc happened */
            sprintf(sock_send, "> [error] buffer reset in field %d\n", id);
            send(conn->websocket, sock_send, strlen(sock_send), 0);
        }
    }
}
