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


  \brief CANBLOCKS library for SocketCAN
  This is a CANBLOCKS extended adress implementation for SocketCAN.

  @author  Tobias Schmitt
  @email   tobias.schmitt@codm.de
  @date    25.6.2015
  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <linux/can.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <string.h>

#include "canblocks.h"
#include "main.h"

/**
  canblocks-Buffer
  this is not a ringbuffer, because the creation of one buffer element and
  it's completion / deletion depends on more than 1 CAN messages, which will
  be send asynchronously. so the buffer functions have to "search" the whole
  array everytime.
  */
typedef struct {
    struct canblocks_frame frame;
    uint8_t finished;
    uint8_t free;
    uint8_t *data_ptr; /* pointer to place in frame.data where to write */
    uint16_t data_iter; /* count of written data */
    struct can_frame canframes[CANBLOCKS_BLOCKSIZE];
    uint8_t block_counter;
} canblocksbuff_t;

canblocksbuff_t canblocks_buffer[CANBLOCKS_BUFFER_SIZE];

int _buff_get_next_free(canblocksbuff_t **dst);
int _buff_get_finished(canblocksbuff_t **dst);
int _buff_get_pending(canblocksbuff_t **dst, uint8_t senderID);
void _buff_transfer_canframes(canblocksbuff_t *dst);
void _buff_reset_field(canblocksbuff_t *dst);
void _buff_reset_canframes(canblocksbuff_t *dst);

/**
  Program Code
  */

void canblocks_init(void) {
    int i;
    for(i = 0; i < CANBLOCKS_BUFFER_SIZE; i++) {
        _buff_reset_field(&canblocks_buffer[i]);
    }
}

int canblocks_compute_frame(int *socket, struct can_frame *frame) {
    uint8_t i, status, sender, receiver;
    uint16_t dl;
    uint8_t *dataptr;
    canblocksbuff_t *dst;
    struct can_frame flowcontrol;

    status = (frame->data[1] & 0xF0) >> 4;
    sender = frame->data[0];
    receiver = (frame->can_id & CAN_SFF_MASK);

    flowcontrol.can_id = sender;
    flowcontrol.can_dlc = 4;
    flowcontrol.data[0] = receiver;
    flowcontrol.data[1] = ((CANBLOCKS_STATUS_FC << 4)|CANBLOCKS_FLOWSTAT_CLEAR);
    flowcontrol.data[2] = CANBLOCKS_BLOCKSIZE;
    flowcontrol.data[3] = CANBLOCKS_MIN_SEP_TIME;

    if(!((receiver == rec_filter) || (receiver == CANBLOCKS_BROADCAST))) {
        return CANBLOCKS_COMPRET_ERROR;
    }

    switch(status) {
        case CANBLOCKS_STATUS_SF:
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
                return CANBLOCKS_COMPRET_COMPLETE;
            }
            else {
                /* no free buffer */
                return CANBLOCKS_COMPRET_ERROR;
            }
        case CANBLOCKS_STATUS_FF:
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
                    return CANBLOCKS_COMPRET_ERROR;
                }
                return CANBLOCKS_COMPRET_TRANS;
            }
            else {
                /* no free buffer */
                return CANBLOCKS_COMPRET_ERROR;
            }
        case CANBLOCKS_STATUS_CF:
            /* if consecutive frame */
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
                    return CANBLOCKS_COMPRET_COMPLETE;
                }
                if(dst->block_counter >= CANBLOCKS_BLOCKSIZE) {
                    /* maximum blocks saved */
                    /* copy canframes to frame->data */
                    _buff_transfer_canframes(dst);
                    _buff_reset_canframes(dst);
                    dst->block_counter = 0;
                    /* send flowcontrol */
                    if(write(*socket, &flowcontrol, sizeof(struct can_frame)) < 1) {
                        return CANBLOCKS_COMPRET_ERROR;
                    }
                }
                return CANBLOCKS_COMPRET_TRANS;
            }
            else {
                /* no buffer found */
                return CANBLOCKS_COMPRET_ERROR;
            }
    }
    /* if the code comes till here, something is very broken */
    return CANBLOCKS_COMPRET_ERROR;
}

int canblocks_get_frame(struct canblocks_frame *dst) {
    canblocksbuff_t *fin = NULL;
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

int canblocks_send_frame(int *socket, struct canblocks_frame *frame) {

    struct can_frame sframe, recvfc;
    unsigned int i, j, r, lencnt, fc_blocksize, fc_minseptime;
    uint8_t *datainc;
    fd_set rfds;
    struct timeval tv;
    struct timespec wait;

    /* zero rfds and 10000 usec wait */
    FD_ZERO(&rfds);
    FD_SET(*socket, &rfds);
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    sframe.can_id = frame->rec;
    datainc = frame->data;
    /* single frame */
    if(frame->dl <= 6) {
        sframe.can_dlc = frame->dl + 2;
        sframe.data[0] = frame->sender;
        sframe.data[1] = (CANBLOCKS_STATUS_SF << 4) | frame->dl;
        for(i = 0; i < frame->dl; i++) {
            sframe.data[i + 2] = *datainc++;
        }
        r = write(*socket, &sframe, sizeof(struct can_frame));
        if(r > 0)
            return 1;
        else
            return 0;
    }

    lencnt = frame->dl;
    /* build first frame */
    sframe.can_dlc = 8;
    sframe.data[0] = frame->sender;
    sframe.data[1] = (CANBLOCKS_STATUS_FF << 4) | ((frame->dl & 0x0F00) >> 8);
    sframe.data[2] = frame->dl & 0x00FF;
    for(i = 3; i < 8; i++) {
        sframe.data[i] = *datainc++;
        lencnt--;
    }

    /* set sock option for timeout */
    setsockopt(*socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

    r = write(*socket, &sframe, sizeof(struct can_frame));
    /* wait for FC with timeout */
    if(recv(*socket, &recvfc, sizeof(struct can_frame), 0) != -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("missing, flowcontrol... exiting\n");
            return 0;
        } else {
            if((recvfc.can_id == sframe.data[0]) && (recvfc.data[0] == sframe.can_id) &&
               ((recvfc.data[1] >> 4) == CANBLOCKS_STATUS_FC)) {
            fc_blocksize = recvfc.data[2];
            fc_minseptime = recvfc.data[3];
            wait.tv_nsec = fc_minseptime * 1000000; /* msec to nsec */
            }
        }
    }

    int block_count = 1;

    /* while still bytes to send */
    while(lencnt > 0) {
        /* while not last packet */
        if(lencnt > 6) {
            /* build consecutive frame */
            sframe.can_id = frame->rec;
            sframe.can_dlc = 8;
            sframe.data[0] = frame->sender;
            sframe.data[1] = (CANBLOCKS_STATUS_CF << 4) | block_count;
            for(i = 2; i < 8; i++) {
                sframe.data[i] = *datainc++;
                lencnt--;
            } 
            /* send consecutive frame */
            write(*socket, &sframe, sizeof(struct can_frame));
            if(block_count == fc_blocksize - 1) {
                if(recv(*socket, &recvfc, sizeof(struct can_frame), 0) != -1) {
                    if(errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("missing, flowcontrol... exiting\n");
                        return 0;
                    } else {
                        if((recvfc.can_id == sframe.data[0]) && (recvfc.data[0] == sframe.can_id) &&
                           ((recvfc.data[1] >> 4) == CANBLOCKS_STATUS_FC)) {
                        fc_blocksize = recvfc.data[2];
                        fc_minseptime = recvfc.data[3];
                        wait.tv_nsec = fc_minseptime * 1000000; /* msec to nsec */
                        }
                    }
                }
            }
            block_count = (block_count + 1) % fc_blocksize;
            nanosleep(&wait, NULL);
        } else {
            /* compute frame length */
            j = lencnt + 2;
            sframe.can_id = frame->rec;
            sframe.can_dlc = j;
            sframe.data[0] = frame->sender;
            sframe.data[1] = (CANBLOCKS_STATUS_CF << 4) | block_count;
            for(i = 2; i < j; i++) {
                sframe.data[i] = *datainc++;
                lencnt--;
            } 
            write(*socket, &sframe, sizeof(struct can_frame));
        }
    }



    setsockopt(*socket, SOL_SOCKET, SO_RCVTIMEO, NULL,sizeof(struct timeval));
    /* something went wrong */
    return 0;
}

int _buff_get_next_free(canblocksbuff_t **dst) {
    int i;
    for(i = 0; i < CANBLOCKS_BUFFER_SIZE; i++) {
        if(canblocks_buffer[i].free) {
            *dst = &canblocks_buffer[i];
            return 1;
        }
    }
    return 0;
}

int _buff_get_finished(canblocksbuff_t **dst) {
    int i;
    for(i = 0; i < CANBLOCKS_BUFFER_SIZE; i++) {
        if(canblocks_buffer[i].finished) {
            *dst = &canblocks_buffer[i];
            return 1;
        }
    }
    return 0;
}

int _buff_get_pending(canblocksbuff_t **dst, uint8_t senderID) {
    int i;
    for(i = 0; i < CANBLOCKS_BUFFER_SIZE; i++) {
        if(!canblocks_buffer[i].finished && !canblocks_buffer[i].free
                && canblocks_buffer[i].frame.sender == senderID) {
            *dst = &canblocks_buffer[i];
            return 1;
        }
    }
    return 0;
}

void _buff_transfer_canframes(canblocksbuff_t *dst) {
    int i = 0, k = 0;
    for(i = 0; i < CANBLOCKS_BLOCKSIZE; i++) {
        if(dst->canframes[i].can_dlc > 0) {
            if((dst->canframes[i].data[1] >> 4) == CANBLOCKS_STATUS_FF)
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

void _buff_reset_field(canblocksbuff_t *dst) {
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

void _buff_reset_canframes(canblocksbuff_t *dst) {
    int i,k;
    for(i = 0; i < CANBLOCKS_BLOCKSIZE; i++) {
        dst->canframes[i].can_id = 0;
        dst->canframes[i].can_dlc = 0;
        for(k = 0; k < 8; k++) {
            dst->canframes[i].data[k] = 0;
        }
    }
}

int canblocks_fr2str(char *dst, struct canblocks_frame *src) {
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

int canblocks_str2fr(char *src, struct canblocks_frame *dst) {
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

void canblocks_reset_frame(struct canblocks_frame *dst) {
    free(dst->data);
    dst->sender = 0;
    dst->rec = 0;
    dst->dl = 0;
}
