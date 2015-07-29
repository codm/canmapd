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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <linux/can.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <string.h>

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
    uint8_t *data_ptr; /* pointer to place in frame.data where to write */
    uint16_t data_iter; /* count of written data */
    struct can_frame canframes[ISOTP_BLOCKSIZE];
    uint8_t block_counter;
} isotpbuff_t;

isotpbuff_t isotp_buffer[ISOTP_BUFFER_SIZE];

int _buff_get_next_free(isotpbuff_t **dst);
int _buff_get_finished(isotpbuff_t **dst);
int _buff_get_pending(isotpbuff_t **dst, uint8_t senderID);
void _buff_transfer_canframes(isotpbuff_t *dst);
void _buff_reset_field(isotpbuff_t *dst);
void _buff_reset_canframes(isotpbuff_t *dst);

/**
  Program Code
  */

void isotp_init(void) {
    int i;
    for(i = 0; i < ISOTP_BUFFER_SIZE; i++) {
        _buff_reset_field(&isotp_buffer[i]);
    }
}

int isotp_compute_frame(int *socket, struct can_frame *frame) {
    uint8_t i, status, sender, receiver;
    uint16_t dl;
    uint8_t *dataptr;
    isotpbuff_t *dst;
    struct can_frame flowcontrol;

    status = (frame->data[1] & 0xF0) >> 4;
    sender = frame->data[0];
    receiver = (frame->can_id & CAN_SFF_MASK);

    flowcontrol.can_id = sender;
    flowcontrol.can_dlc = 4;
    flowcontrol.data[0] = receiver;
    flowcontrol.data[1] = ((ISOTP_STATUS_FC << 4)|ISOTP_FLOWSTAT_CLEAR);
    flowcontrol.data[2] = ISOTP_BLOCKSIZE;
    flowcontrol.data[3] = ISOTP_MIN_SEP_TIME;

    switch(status) {
        case ISOTP_STATUS_SF:
            /* if single frame */
            if(_buff_get_next_free(&dst)) {
                dst->free = 0;
                dst->finished = 1;
                dst->frame.sender = sender;
                dst->frame.rec = receiver;
                dl = (frame->data[1] & 0x0F);
                dst->frame.dl = dl;
                dst->frame.data = malloc(dl * sizeof(uint8_t));
                dataptr = dst->frame.data;
                for(i = 2; i < (dl+2); i++) {
                    *dataptr = frame->data[i];
                    dataptr++;
                }
                dst->finished = 1;
                return ISOTP_COMPRET_COMPLETE;
            }
            else {
                /* no free buffer */
                return ISOTP_COMPRET_ERROR;
            }
        case ISOTP_STATUS_FF:
            /* if first frame */
            if(_buff_get_next_free(&dst)) {
                dst->free = 0;
                dst->finished = 0;
                dst->frame.sender = sender;
                dst->frame.rec = receiver;
                dl = ((frame->data[1] & 0x0F) << 8) + frame->data[2];
                dst->frame.dl = dl;
                dst->frame.data = malloc(dl * sizeof(uint8_t));
                dst->data_ptr = dst->frame.data;
                memcpy(&dst->canframes[0], frame, sizeof(struct can_frame));
                dst->data_iter = 5;
                dst->block_counter = 1;
                if(write(*socket, &flowcontrol, sizeof(struct can_frame)) < 1) {
                    return ISOTP_COMPRET_ERROR;
                }
                return ISOTP_COMPRET_TRANS;
            }
            else {
                /* no free buffer */
                return ISOTP_COMPRET_ERROR;
            }
        case ISOTP_STATUS_CF:
            /* if first frame */
            if(_buff_get_pending(&dst, sender)) {
                /* copy frame to canframes at right point */
                int sequenceNr = frame->data[1] & 0x0F;
                memcpy(&dst->canframes[sequenceNr], frame, sizeof(struct can_frame));
                dst->data_iter += frame->can_dlc - 2;
                dst->block_counter++;
                if(dst->data_iter == dst->frame.dl) {
                    /* transmission finished */
                    dst->finished = 1;
                    /* copy canframes to frame->data */
                    _buff_transfer_canframes(dst);
                    return ISOTP_COMPRET_COMPLETE;
                }
                if(dst->block_counter >= ISOTP_BLOCKSIZE) {
                    /* maximum blocks saved */
                    /* copy canframes to frame->data */
                    _buff_transfer_canframes(dst);
                    _buff_reset_canframes(dst);
                    dst->block_counter = 0;
                    /* send flowcontrol */
                    if(write(*socket, &flowcontrol, sizeof(struct can_frame)) < 1) {
                        return ISOTP_COMPRET_ERROR;
                    }
                }
                return ISOTP_COMPRET_TRANS;
            }
            else {
                /* no buffer found */
                return ISOTP_COMPRET_ERROR;
            }
    }
    /* if the code comes till here, something is very broken */
    return ISOTP_COMPRET_ERROR;
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
        _buff_reset_field(fin);
        return 1;
    }
    else {
        return 0;
    }
}

int isotp_send_frame(int *socket, struct isotp_frame *frame) {

    struct can_frame sframe;
    unsigned int i, r, lencnt;
    uint8_t *datainc;
    fd_set rfds;
    struct timeval tv;

    /* zero rfds and 10000 usec wait */
    FD_ZERO(&rfds);
    FD_SET(*socket, &rfds);
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    sframe.can_id = frame->rec;
    datainc = frame->data;
    lencnt = frame->dl;
    /* single frame */
    if(frame->dl <= 6) {
        sframe.can_dlc = frame->dl + 2;
        sframe.data[0] = frame->sender;
        sframe.data[1] = (ISOTP_STATUS_SF << 4) | frame->dl;
        for(i = 0; i < frame->dl; i++)
            sframe.data[i + 2] = *datainc++;
        r = write(*socket, &sframe, sizeof(struct can_frame));
        if(r > 0)
            return 1;
        else
            return 0;
    }

    /* build first frame */
    sframe.can_dlc = 8;
    sframe.data[0] = frame->sender;
    sframe.data[1] = (ISOTP_STATUS_FF << 4) | ((frame->dl & 0x0F00) >> 8);
    sframe.data[2] = frame->dl & 0x00FF;
    for(i = 3; i < 8; i++) {
        sframe.data[i] = *datainc++;
        lencnt--;
    }

    r = write(*socket, &sframe, sizeof(struct can_frame));
    /* wait for FC with timeout */
    r = select(*socket + 1, &rfds, NULL, NULL, &tv);
    if(r <= 0) {
        perror("select()");
    }

    /* something went wrong */
    return 0;
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

int _buff_get_pending(isotpbuff_t **dst, uint8_t senderID) {
    int i;
    for(i = 0; i < ISOTP_BUFFER_SIZE; i++) {
        if(!isotp_buffer[i].finished && !isotp_buffer[i].free
                && isotp_buffer[i].frame.sender == senderID) {
            *dst = &isotp_buffer[i];
            return 1;
        }
    }
    return 0;
}

void _buff_transfer_canframes(isotpbuff_t *dst) {
    int i = 0, k = 0;
    for(i = 0; i < ISOTP_BLOCKSIZE; i++) {
        if(dst->canframes[i].can_dlc > 0) {
            if((dst->canframes[i].data[1] >> 4) == ISOTP_STATUS_FF)
                k = 3;
            else
                k = 2;
            for(;k < dst->canframes[i].can_dlc; k++) {
                *(dst->data_ptr) = dst->canframes[i].data[k];
                dst->data_ptr++;
            }
        }
    }
}

void _buff_reset_field(isotpbuff_t *dst) {
    dst->finished = 0;
    dst->free = 1;
    dst->data_iter = 0;
    dst->block_counter = 0;
    dst->frame.sender = 0;
    dst->frame.rec = 0;
    dst->frame.dl = 0;
    free(dst->frame.data);
    _buff_reset_canframes(dst);
}

void _buff_reset_canframes(isotpbuff_t *dst) {
    int i,k;
    for(i = 0; i < ISOTP_BLOCKSIZE; i++) {
        dst->canframes[i].can_id = 0;
        dst->canframes[i].can_dlc = 0;
        for(k = 0; k < 8; k++) {
            dst->canframes[i].data[k] = 0;
        }
    }
}

int isotp_fr2str(char *dst, struct isotp_frame *src) {
    char *buffer = dst;
    int i, n;
    n = sprintf(buffer, "%02x;", src->sender);
    buffer = buffer+n;
    n = sprintf(buffer, "%02x;", src->rec);
    buffer = buffer+n;
    n = sprintf(buffer, "%04u;", src->dl);
    buffer = buffer+n;
    for(i = 0; i < src->dl; i++) {
        n = sprintf(buffer, "%02x", src->data[i]);
        buffer = buffer+n;
    }
    *buffer = '\n';
    return 1;
}

int isotp_str2fr(char *src, struct isotp_frame *dst) {
    unsigned int i, sender, rec, dl;
    uint8_t *bufdst;
    char buffer[2*4096]; /* 4096 uint8_t a 2 characters */
    char *bufbuff = buffer;
    /* TODO: Secure this input via regex */
    if(sscanf(src, "%02x;%02x;%04u;%s", &sender, &rec, &dl, buffer) < 1) {
        return 0;
    };
    dst->sender = (uint8_t)sender;
    dst->rec = (uint8_t)rec;
    dst->dl = (uint8_t)dl;
    dst->data = malloc(sizeof(uint8_t) * dst->dl);
    bufdst = dst->data;
    for(i = 0; i < dst->dl; i++) {
        sscanf(bufbuff, "%02x", &rec); /* read 2 chars put into byte */
        *bufdst = (uint8_t)rec;
        bufdst++; /* iterate over array */
        bufbuff = bufbuff + 2; /* iterate over 2 chars in string */
    }
    return 1;
}


