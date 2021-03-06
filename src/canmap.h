

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


   \brief CANMAP library for SocketCAN
   This is a CANMAP extended adress implementation for SocketCAN.

   @author  Tobias Schmitt
   @email   tobias.schmitt@codm.de
   @date    24.6.2015
*/

#ifndef _CANMAP_H
#define _CANMAP_H

/*
   Defines
*/

#define CANMAP_BUFFER_SIZE        64      /* Buffer size can be chosen freely */
#define CANMAP_BLOCKSIZE          12       /* Maximum 16 Blocks  */
#define CANMAP_MIN_SEP_TIME       0      /* Min 10ms Seperation time  */
#define CANMAP_BROADCAST          0xFF    /* Broadcast Adress */
#define CANMAP_MAXFRAMESIZE       4096    /* Maximum canmap frame size */

#define CANMAP_STATUS_SF          0x00    /* Single Frame */
#define CANMAP_STATUS_FF          0x01    /* First Frame */
#define CANMAP_STATUS_CF          0x02    /* Consecutive Frames */
#define CANMAP_STATUS_FC          0x03    /* Flow Control Frame */

#define CANMAP_COMPRET_COMPLETE   1       /* Transmission Complete */
#define CANMAP_COMPRET_TRANS      0       /* Transmission pending... */
#define CANMAP_COMPRET_ERROR      -1      /* No ISO-TP Frame or no fre buffer */

#define CANMAP_FLOWSTAT_CLEAR     0
#define CANMAP_FLOWSTAT_WAIT      1
#define CANMAP_FLOWSTAT_OVERFLOW  2

#define CANMAP_GC_TIMEOUT         20
#define CANMAP_GC_REFRESH         60

/**
  \brief Abstract struct of a ISO-TP frame
  this is a typical ISO-TP frame for extended CAN Addressing
*/
struct canmap_frame {
    uint8_t sender; /**< Sender-ID of ISO-TP Frame */
    uint8_t rec; /**< Receiver-ID of ISO-TP Frame */
    uint16_t dl; /**< Length of ISO-TP Frame */
    uint8_t* data; /**< Data Pointer of ISO-TP Frame */
};

/**
  \brief !MANDATORY! init the needed canblocks data structs
*/


void canmap_init();
/**
  \brief computes can_frame into internal buffer
  This function computes a can_frame into its internal iso_tp
  framebuffer. If this was the last frame of an ISO-TP message,
  the funtions returns status > 0. If there are still messages missing
  the function returns 0 and -1 if there was an error executing

  @param[0] int *socket        pointer to an open CAN_SOCKET
  @param[0] can_frame *frame can frame to be processed

  @return < 0 for error, 0 if there are still messages to come
            1 if the canmap_frame is finished and ready to get
*/
int canmap_compute_frame(int *socket, struct can_frame *frame);

/**
  \brief sends an canmap frame over socket

  @param[0] int *socket        pointer to an open CAN_SOCKET
  @param[0] canmap_frame *frame frame to be sent

  @return EXIT_SUCCESS for success
          EXIT_FAILURE for failure

*/
int canmap_send_frame(int *socket, struct canmap_frame *frame);

/**
  \brief gets a finished ISO-TP frame for further computation

  @param[0] canmap_frame **dst pointer to a frame which should be written

  @return EXIT_SUCCESS for success
          EXIT_FAILURE for failure
*/
int canmap_get_frame(struct canmap_frame *dst);

void canmap_reset_frame(struct canmap_frame *dst);

int canmap_fr2str(char *dst, struct canmap_frame *src);
int canmap_str2fr(char *src, struct canmap_frame *dst);
int canmap_clean_garbage(void);

#endif
