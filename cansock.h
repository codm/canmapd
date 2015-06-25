#ifndef _CANSOCK_H
#define _CANSOCK_H

#include "globals.h"

#include <linux/can.h>
#include <linux/if.h>
#include <sys/ioctl.h>

#include "isotp.h"

extern void cansock_sighand(int sig);
extern void *cansock_run();

#endif

