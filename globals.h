#ifndef GLOBALS_H
#define GLOBALS_H

#define _POSIX_SOURCE
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
const char DAEMON_NAME[] = "isotpd";
const char DAEMON_VERSION[] = "0.1";
#else
#define EXT extern
extern const char DAEMON_NAME[];
extern const char DAEMON_VERSION[];
#endif

#define GL_OWN_CAN_ID 0x00

EXT uint8_t verbose; /* defines if program runs in verbose mode */
EXT uint8_t run_daemon; /* defines if program runs in daemon mode */
EXT uint8_t virtualcan; /* defines if virtual can vcan0 is used */

#endif

