#include "main.h"
/*
   init global vars
*/
pid_t pid, sid;
MYSQL *db;

void sig_term(int sig) {
    /* shutting down program properly */
    syslog(LOG_INFO, "%s", "Sigterm received - shutting down");

    /* kill mySQL Conn */
    if(db != NULL) {
        mysql_close(db);
    }

    /* closing log */
    syslog(LOG_INFO, "%s", "Shutdown complete");
    closelog();

    exit(EXIT_SUCCESS);
}

/*
    main loop
*/
int main(int argc, const char* argv[]) {
    /* init vars */
    int i = 0;
    int websock, conn;
    struct sockaddr_in webserv, webclient;
    socklen_t len;

    /* run code */
    for(i=0; i < argc; i++) {
        if(!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v")) {
                verbose = 1;
        }
        else if (!strcmp(argv[i], "--run_daemon") || !strcmp(argv[i], "-d")) {
                run_daemon = 1;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("%s daemon helptext. ", DAEMON_NAME);
            printf("Daemon version %s. ", DAEMON_VERSION);
            printf("Built %s %s\n", __DATE__, __TIME__);
            printf("    --verbose (-v) Prints additional output to stdout\n");
            printf("    --daemon  (-d) start as a daemon\n");
            printf("    --help    (-h) prints this helptext\n");
            exit(EXIT_SUCCESS);
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
    acm_mysql_connect(db);


    /* init websocket */
    websock = socket( AF_INET, SOCK_STREAM, 0 );
    /* websock = socket( AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0 );*/
    if(websock < 0) {
        syslog(LOG_ERR, "was not able to initiate Websocket");
        exit(EXIT_FAILURE);
    }
    memset(&webserv, 0, sizeof(webserv));
    webserv.sin_family = AF_INET;
    webserv.sin_addr.s_addr = inet_addr("127.0.0.1");
    webserv.sin_port = htons(25005);
    if(bind(websock, (struct sockaddr*)&webserv, sizeof(webserv)) < 0) {
        syslog(LOG_ERR, "was not able to bind Webserver");
        exit(EXIT_FAILURE);
    }

    if(listen(websock, 9) < 0) {
        syslog(LOG_ERR, "not able to register Listen");
        exit(EXIT_FAILURE);
    }

    /* TODO */
    syslog(LOG_INFO, "%s", "daemon successfully started");
    while(1) {
        char webbuff[WEBSOCK_MAX_RECV];
        int webbuffsize;

        /* main program loop */
        len = sizeof(webclient);
        conn = accept(websock, (struct sockaddr*)&webclient, &len);
        if(conn < 0) {
        } else {
            printf("connection from: %s\n", inet_ntoa(webclient.sin_addr));
            if((webbuffsize = recv(conn, webbuff, WEBSOCK_MAX_RECV,0)) < 0)
                printf("Fehler in websock recv");
            webbuff[webbuffsize] = '\0';
            printf("msg: %s \n", webbuff);
        }
    }
    mysql_close(db);
    syslog(LOG_INFO, "%s", "daemon successfully shut down");

    return 0;
}
