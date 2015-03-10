#include "main.h"
/*
   init global vars
*/
pid_t pid, sid;
pthread_t webthread;
pthread_t canthread;

void sig_term(int sig) {
    /* shutting down program properly */
    syslog(LOG_INFO, "%s", "Sigterm received - shutting down");

    /* kill mySQL Conn */
    if(db != NULL) {
        mysql_close(db);
        free(db);
    }

    /* signal queue threads *
    pthread_kill(canthread, SIGTERM);
    pthread_kill(webthread, SIGTERM);
    */

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
    printf("    --daemon  (-d) start as a daemon\n");
    printf("    --help    (-h) prints this helptext\n");
    exit(EXIT_SUCCESS);
}

/*
    main loop
*/
int main(int argc, const char* argv[]) {
    /* init vars */
    int i = 0;
    int rc;
    MYSQL_RES *res;
    CANFRAME dummy;
    ACM_CANMESSAGE msg;

    dummy.can_id = 0xFE;
    dummy.can_dlc = 8;
    dummy.data[0] = 0x01;
    dummy.data[1] = 0x01;
    dummy.data[2] = 0xBE;
    dummy.data[3] = 0xEF;
    dummy.data[4] = 0xDE;
    dummy.data[5] = 0xAD;
    dummy.data[6] = 0xBE;
    dummy.data[7] = 0xF0;

    translate_frame(&dummy, &msg);

    printf("can: id: %d; data: %X %X %X %X %X %X %X %X \n", dummy.can_id, dummy.data[0], dummy.data[1], dummy.data[2], 
            dummy.data[3], dummy.data[4], dummy.data[5], dummy.data[6], dummy.data[7]);

    printf("can: sender: %d; receiver: %X; command: %X; data: %X %X %X %X %X %X \n", msg.sender, msg.receiver, 
            msg.command, msg.data[0], msg.data[1], msg.data[2], msg.data[3], msg.data[4], msg.data[5]);
    /* run code */
    for(i=0; i < argc; i++) {
        if(!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v")) {
            verbose = 1;
        }
        else if (!strcmp(argv[i], "--run_daemon") || !strcmp(argv[i], "-d")) {
            run_daemon = 1;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            print_helptext();
        }
    }

    /* register signal handlers */
    signal(SIGTERM, sig_term);

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
    /* set filemode creation mask to 0777 */
    umask(0);

    /* open syslog for daemon */
    openlog("accessm", 0, LOG_USER);

    if(run_daemon) {
        sid = setsid();
        if(sid < 0) {
            syslog(LOG_ERR, "%s", "was not able to get session id");
            exit(EXIT_FAILURE);
        }
    }

    /* do ibdoor init stuff */
    /* init mysql daemon */
    /*acm_mysql_connect(db);*/
    db = (MYSQL *) malloc(sizeof(MYSQL));
    res = (MYSQL_RES *) malloc(sizeof(MYSQL_RES));
    acm_mysql_connect(db);
    acm_mysql_getUsers(db, res);


    /* init websocket */
    rc = pthread_create(&webthread, NULL, &websock_run, NULL);
    if(rc < 0) {
        printf("Not able to initiale Websock thread");
    }


    rc = pthread_create(&canthread, NULL, &cansock_run, NULL);
    if(rc < 0) {
        printf("Not able to initiale cansock thread");
    }

    syslog(LOG_INFO, "%s (%s) %s", DAEMON_NAME, DAEMON_VERSION, "started");
    while(1) {
        sleep(3);
    }
    mysql_close(db);
    syslog(LOG_INFO, "%s", "daemon successfully shut down");

    return 0;
}
