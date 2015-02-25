void cansock_sighand(int sig) {
    exit(EXIT_SUCCESS);
}


static void *cansock_run() {
    /* TODO: Stuff */
    syslog(LOG_INFO, "cansock thread started");
    signal(SIGTERM, cansock_sighand);
    while(1) {
        sleep(10);
    }
    return 0;
}
