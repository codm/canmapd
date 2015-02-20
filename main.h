#define MAIN

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <mysql.h>
#include <signal.h>


#include "globals.h"
#include "mysql.c"

/* Important defines */
#define DAEMON_VERSION "0.3"
#define DAEMON_NAME "accessm"
