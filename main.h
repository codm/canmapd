#ifndef MAIN_H
#define MAIN_H

#define MAIN_GL
#include "globals.h"
#undef MAIN_GL

#include "websock.c"
#include "cansock.c"
#include "utils/mysql.c"
#include "utils/can.c"

/* Important defines */
#define DAEMON_VERSION "0.1"
#define DAEMON_NAME "accessm"


static void *websock_run();
static void *cansock_run();
#endif
