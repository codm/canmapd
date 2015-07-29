#include "globals.h"
#include "isotpsend.h"
#include "isotp.h"

extern int cansocket;
int websocket;
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
    struct sockaddr_in webclient, webserv;
    struct isotp_frame sendframe;
    socklen_t len;

    /*
       WEBSOCKET
    */
    websocket = socket(AF_INET, SOCK_STREAM, 0);
    if(websocket < 0) {
        syslog(LOG_ERR, "was not able to initiate websocket");
        exit(EXIT_FAILURE);
    }
    /* bind server */
    memset(&webserv, 0, sizeof(webserv));
    webserv.sin_family = AF_INET;
    webserv.sin_addr.s_addr = inet_addr("127.0.0.1");
    webserv.sin_port = htons(25005);
    if(bind(websocket, (struct sockaddr*)&webserv, sizeof(webserv)) < 0) {
        syslog(LOG_ERR, "was not able to bind webserver");
        exit(EXIT_FAILURE);
    }
    if(listen(websocket, 9) < 0) {
        syslog(LOG_ERR, "not able to register listen");
        exit(EXIT_FAILURE);
    }

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
            if(isotp_str2fr(webbuff, &sendframe) > 0) {
                printf("msg: %s \n", webbuff);
                isotp_send_frame(&cansocket, &sendframe);
            };
            close(conn);
        }
    }
    return 0;
}
