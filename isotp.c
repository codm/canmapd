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
  @date    25.6.2015
  */

#include "isotp.h"

/**
  ISOTP-Buffer
  this is not a ringbuffer, because the creation of one buffer element and
  it's completion / deletion depends on more than 1 CAN messages, which will
  be send asynchronously. so the buffer functions have to "search" the whole
  array everytime.
  */
typedef struct {
    struct isotp_frame frame;
    uint8_t finished;
    uint8_t free;
} isotpbuff_t;

isotpbuff_t isotp_buffer[ISOTP_BUFFER_SIZE];

int _buff_get_next_free(isotpbuff_t **dst);
int _buff_get_finished(isotpbuff_t **dst);

/**
  Program Code
  */

void isotp_init(void) {
    int i;
    for(i = 0; i < ISOTP_BUFFER_SIZE; i++) {
        isotp_buffer[i].finished = 0;
        isotp_buffer[i].free = 1;
    }
}

int isotp_compute_frame(struct can_frame *frame) {
    uint8_t i, status, dl;
    uint8_t *dataptr;
    isotpbuff_t *dst;

    status = (frame->data[1] & 0xF0) >> 4;
    switch(status) {
        case ISOTP_STATUS_SF:
            /* if single frame */
            if(_buff_get_next_free(&dst)) {
                dst->free = 0;
                dst->frame.sender = (frame->can_id & CAN_SFF_MASK);
                dst->frame.rec = frame->data[0];
                dl = (frame->data[1] & 0x0F);
                dst->frame.dl = dl;
                dst->frame.data = malloc(dl * sizeof(uint8_t));
                dataptr = dst->frame.data;
                for(i = 2; i < (dl+2); i++) {
                    *dataptr = frame->data[i];
                    dataptr++;
                }
                dst->finished = 1;
                return 1;
            }
            else {
                /* no free buffer */
                return -1;
            }
        case ISOTP_STATUS_FF:
            /* if first frame */
            if(_buff_get_next_free(&dst)) {
                dst->free = 0;
                dst->frame.sender = (frame->can_id & CAN_SFF_MASK);
                dst->frame.rec = frame->data[0];
                dl = ((frame->data[1] & 0x0F) << 8) + frame->data[2];
                dst->frame.dl = dl;
                dst->frame.data = malloc(dl * sizeof(uint8_t));
                dataptr = dst->frame.data;
                for(i = 2; i < (dl+2); i++) {
                    *dataptr = frame->data[i];
                    dataptr++;
                }
                return 0;
            }
            else {
                /* no free buffer */
                return -1;
            }
        case ISOTP_STATUS_CF:
            /* if first frame */
            return 0;
    }
    /* if the code comes till here, something is very broken */
    return -1;
}

int isotp_get_frame(struct isotp_frame *dst) {
    isotpbuff_t *fin = NULL;
    if(_buff_get_finished(&fin)) {

        /* copy whole struct to dst */
        dst->sender = fin->frame.sender;
        dst->rec = fin->frame.rec;
        dst->dl = fin->frame.dl;
        dst->data = malloc(fin->frame.dl * sizeof(uint8_t));
        memcpy(dst->data, fin->frame.data, fin->frame.dl * sizeof(uint8_t));

        /* uninit struct */
        fin->frame.sender = 0;
        fin->frame.rec = 0;
        fin->frame.dl = 0;
        free(fin->frame.data);
        fin->finished = 0;
        fin->free = 1;
        return 1;
    }
    else {
        return 0;
    }
}

int _buff_get_next_free(isotpbuff_t **dst) {
    int i;
    for(i = 0; i < ISOTP_BUFFER_SIZE; i++) {
        if(isotp_buffer[i].free) {
            *dst = &isotp_buffer[i];
            return 1;
        }
    }
    return 0;
}

int _buff_get_finished(isotpbuff_t **dst) {
    int i;
    for(i = 0; i < ISOTP_BUFFER_SIZE; i++) {
        if(isotp_buffer[i].finished) {
            *dst = &isotp_buffer[i];
            return 1;
        }
    }
    return 0;
}

