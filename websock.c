#include "globals.h"
#define WEBSOCK_MAX_RECV 1024

int websock, conn;

void websock_sig(int sig) {
    close(websock);
    close(conn);
    exit(EXIT_SUCCESS);
}

static void *websock_run() {
    /* TODO:
       acceptable signal handling 
    */
    /* Declarations */
    struct sockaddr_in webserv, webclient;
    socklen_t len;

    /* add signal handler websock_sig
    signal(SIGTERM, websock_sig); */

    /* initiate websock */
    websock = socket(AF_INET, SOCK_STREAM, 0);
    if(websock < 0) {
        syslog(LOG_ERR, "was not able to initiate websocket");
        exit(EXIT_FAILURE);
    }

    /* bind server */
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
            printf("sending answer: OK\n");
            send(conn, "OK", 2, 0);
            close(conn);
        }
    }
    return 0;
}
