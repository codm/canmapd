#ifndef _ISOTPRECV_H
#define _ISOTPRECV_H

#include "globals.h"
#include "isotp.h"

extern void isotprecv_sighand(int sig);
extern void *isotprecv_run(void);

#endif

