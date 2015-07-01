/**
The MIT License (MIT)

Copyright (c) 2015, cod.m GmbH

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.


   \brief ISO-TP library for SocketCAN
   This is a ISO-TP extended adress implementation for SocketCAN.

   @author  Tobias Schmitt
   @email   tobias.schmitt@codm.de
   @date    24.6.2015
*/

#ifndef _ISOTP_H
#define _ISOTP_H

#include "globals.h"
#include <linux/can.h>
#include <linux/if.h>
#include <sys/ioctl.h>

/*
   Defines
*/

#define ISOTP_BUFFER_SIZE 20
#define ISOTP_BLOCKSIZE 16

#define ISOTP_STATUS_SF 0x00
#define ISOTP_STATUS_FF 0x01
#define ISOTP_STATUS_CF 0x02
#define ISOTP_STATUS_FC 0x03

/**
  \brief Abstract struct of a ISO-TP frame
  this is a typical ISO-TP frame for extended CAN Addressing
*/
struct isotp_frame {
    uint8_t sender; /**< Sender-ID of ISO-TP Frame */
    uint8_t rec; /**< Receiver-ID of ISO-TP Frame */
    uint16_t dl; /**< Length of ISO-TP Frame */
    uint8_t* data; /**< Data Pointer of ISO-TP Frame */
};

/**
  \brief !MANDATORY! init the needed isotp data structs
*/
void isotp_init();

/**
  \brief computes can_frame into internal buffer
  This function computes a can_frame into its internal iso_tp
  framebuffer. If this was the last frame of an ISO-TP message,
  the funtions returns status > 0. If there are still messages missing
  the function returns 0 and -1 if there was an error executing

  @param[0] can_frame *frame can frame to be processed

  @return < 0 for error, 0 if there are still messages to come
            1 if the isotp_frame is finished and ready to get
*/
int isotp_compute_frame(struct can_frame *frame);

/**
  \brief sends an isotp frame over socket

  @param[0] int *socket pointer to an open CAN_SOCKET
  @param[0] isotp_frame *frame frame to be sent

  @return EXIT_SUCCESS for success
          EXIT_FAILURE for failure

*/
int isotp_send_frame(struct isotp_frame *frame);

/**
  \brief gets a finished ISO-TP frame for further computation

  @param[0] isotp_frame **dst pointer to a frame which should be written

  @return EXIT_SUCCESS for success
          EXIT_FAILURE for failure
*/
int isotp_get_frame(struct isotp_frame *dst);

#endif

