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
#include <signal.h>
#include <pthread.h>
#include <linux/can.h>
#include <linux/if.h>
#include <sys/ioctl.h>

#ifdef MAIN_GL
#define EXT
#else
#define EXT extern
#endif

#define GL_OWN_CAN_ID 0x00

EXT uint8_t verbose; /* defines if program runs in verbose mode */
EXT uint8_t run_daemon; /* defines if program runs in daemon mode */



#endif
