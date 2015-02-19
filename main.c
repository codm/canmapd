#include "main.h"
/*
   init global vars
*/
int verbose = 0;
int run_daemon = 0;

pid_t pid, sid;
/*
    main loop
*/
int main(int argc, const char* argv[]) {
    /* init vars */
    int i = 0;

    /* run code */
    for(i=0; i < argc; i++) {
        printf("arg %d: %s \n", i, argv[i]);
        if(!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v")) {
                verbose = 1;
        }
        else if (!strcmp(argv[i], "--run_daemon") || !strcmp(argv[i], "-d")) {
                run_daemon = 1;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
        }
    }

    if(run_daemon) {
        printf("daemon mode - ");
        /* fork off parent */
        pid = fork();
        /* not good */
        if(pid < 0) {
            printf("error! exiting\n");
            exit(EXIT_FAILURE);
        }
        /* good */
        if(pid > 0) {
            printf("forked into pid %d\n", pid);
            exit(EXIT_SUCCESS);
        }
    }
    /* set filemode creation mask to 0777 */
    umask(0);

    /* open syslog for daemon */
    openlog("accessm", 0, LOG_USER);


    sid = setsid();
    if(sid < 0) {
        syslog(LOG_ERR, "%s", "was not able to get session id");
        exit(EXIT_FAILURE);
    }

    /* do ibdoor init stuff */
    syslog(LOG_INFO, "%s", "daemon successfully started");
    while(1) {
        /* main program loop */
        sleep(20);
        break;
    }
    syslog(LOG_INFO, "%s", "daemon successfully shut down");

    return 0;
}
