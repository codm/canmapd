#include "main.h"
/*
   init global vars
*/
int run_debug = 0;


/* 
    main loop
*/
int main(int argc, const char* argv[]) {
    /* init vars */
    int i = 0; 
    int run_as_daemon = 0;

    /* run code */
    for(i=0; i < argc; i++) {
        printf("arg %d: %s \n", i, argv[i]);
        if(!strcmp(argv[i], "--debug")) {
                run_debug = 1;
                printf("debug mode on\n");
        }
        else if (!strcmp(argv[i], "--daemon")) {
                run_as_daemon = 1;
                printf("daemon mode on\n");
        }
    }

    if(run_debug) {
        printf("ibdoor Daemon starting... \n");
    }
    if(run_as_daemon) {
        printf("daemon mode on - fork into new process");
    }

    return 0;
}
