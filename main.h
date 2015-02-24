#define MAIN

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <mysql.h>
#include <signal.h>


#include "globals.h"
#include "utils/mysql.c"

/* Important defines */
#define DAEMON_VERSION "0.3"
#define DAEMON_NAME "accessm"

#define WEBSOCK_MAX_RECV 1024
