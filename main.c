#include "main.h"
/*
   init global vars
*/
pid_t pid, sid, isotprecvpid, isotpsendpid;
int cansocket, websocket;

void sig_term(int sig) {
    /* shutting down program properly */
    syslog(LOG_INFO, "%s", "Sigterm received - shutting down");

    /* send sigterm to children */
    kill(0, SIGTERM);

    /* closing log */
    syslog(LOG_INFO, "%s", "Shutdown complete");
    closelog();

    exit(EXIT_SUCCESS);
}

/*
   print help text
   */
void print_helptext() {
    printf("%s daemon helptext. ", DAEMON_NAME);
    printf("Daemon version %s. ", DAEMON_VERSION);
    printf("Built %s %s\n", __DATE__, __TIME__);
    printf("(c) cod.m GmbH\n");
    printf("    --verbose (-v) Prints additional output to stdout\n");
    printf("    --vcan0   (-V) sets can device to vcan0 instead of can0");
    printf("    --daemon  (-d) start as a daemon\n");
    printf("    --help    (-h) prints this helptext\n");
    exit(EXIT_SUCCESS);
}

/*
    main loop
*/
int main(int argc, const char* argv[]) {
    /* init vars */
    struct sockaddr_can addr;
    struct ifreq ifr;
    int i = 0;

    /*
       run code
    */
    for(i=0; i < argc; i++) {
        if(!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v")) {
            verbose = 1;
        }
        else if (!strcmp(argv[i], "--run_daemon") || !strcmp(argv[i], "-d")) {
            run_daemon = 1;
        }
        else if (!strcmp(argv[i], "--vcan0") || !strcmp(argv[i], "-V")) {
            virtualcan = 1;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            print_helptext();
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

    /**
      * Open Global Sockets
      *
      * Mainsocket = AF_INET Socket for interprocess communication
      * CANSocket = AF_CAN Socket for CAN Bus Communication
      *
      */

    /*
       CANSOCKET
    */
    cansocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(cansocket < 0) {
        syslog(LOG_ERR, "was not able to init cansock");
        exit(EXIT_FAILURE);
    }
    if(virtualcan) {
        strcpy(ifr.ifr_name, "vcan0" );
    } else {
        strcpy(ifr.ifr_name, "can0" );
    }
    ioctl(cansocket, SIOCGIFINDEX, &ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(cansocket, (struct sockaddr *)&addr, sizeof(addr));



    /**
      * FORK OFF PROCESSES
      *
      * CANPID = CANSocket Polling Process
      * WEBPID = WebSocket Polling Process
      * EVALPID = Evaluation and Sending Process
      *
      */

    isotprecvpid = fork();
    if(isotprecvpid == 0) {
        /* child */
        close(websocket);
        isotprecv_run();
        exit(EXIT_SUCCESS);
    }
    else if(isotprecvpid < 0) {
        /* error */
        syslog(LOG_ERR, "was not able to create canpid");
    }

    isotpsendpid = fork();
    if(isotpsendpid == 0) {
        /* child */
        isotpsend_run();
        exit(EXIT_SUCCESS);
    }
    else if(isotpsendpid < 0) {
        /* error */
        syslog(LOG_ERR, "was not able to create webpid");
        exit(EXIT_FAILURE);
    }

    /**
      * Write Syslog and go into main loop
      */
    pid = getpid();

    /* install sighandle for main process */
    signal(SIGTERM, sig_term);
    syslog(LOG_INFO, "%s (%s) started", DAEMON_NAME, DAEMON_VERSION);
    if(verbose > 0) {
        printf("PIDs: main: %d / isotprecv: %d / isotpsend: %d\n", pid, isotprecvpid, isotpsendpid);
    }
    while(1) {
        sleep(3);
    }
    syslog(LOG_INFO, "%s", "daemon successfully shut down");
    return 0;
}
