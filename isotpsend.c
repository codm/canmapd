#include "globals.h"
#include "isotpsend.h"

extern int websocket, cansocket;
int conn;

void isotpsend_sig(int sig) {
    syslog(LOG_INFO, "%s", "isotpsend - SIGTERM - shutting down");
    close(cansocket);
    close(websocket);
    close(conn);
    closelog();
    exit(EXIT_SUCCESS);
}

void *isotpsend_run() {
    /* Declarations */
    struct sockaddr_in webclient;
    socklen_t len;

    /* add signal handler websock_sig */
    syslog(LOG_INFO, "isotpsend process started");
    signal(SIGTERM, isotpsend_sig);

    while(1) {
        char webbuff[WEBSOCK_MAX_RECV];
        int webbuffsize;

        /* main program loop */
        len = sizeof(webclient);
        conn = accept(websocket, (struct sockaddr*)&webclient, &len);
        if(conn < 0) {
        } else {
            printf("connection from: %s\n", inet_ntoa(webclient.sin_addr));
            if((webbuffsize = recv(conn, webbuff, WEBSOCK_MAX_RECV,0)) < 0)
                printf("Fehler in websock recv");
            webbuff[webbuffsize] = '\0';
            printf("msg: %s \n", webbuff);
            printf("sending answer: OK\n");
            send(conn, "OK", 2, 0);
            close(conn);
        }
    }
    return 0;
}
