#ifndef GLOBALS_H
#define GLOBALS_H

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
#include <pthread.h>

#ifdef MAIN_GL
#define EXT
#else
#define EXT extern
#endif

EXT short verbose; /* defines if program runs in verbose mode */
EXT short run_daemon; /* defines if program runs in daemon mode */

EXT MYSQL *db;

#endif
