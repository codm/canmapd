#ifndef MAIN_H
#define MAIN_H

#define GL_OWN_CAN_ID 0x00
#define WEBSOCK_MAX_RECV 1024



uint8_t verbose; /* defines if program runs in verbose mode */
uint8_t run_daemon; /* defines if program runs in daemon mode */
uint8_t virtualcan; /* defines if virtual can vcan0 is used */
uint8_t rec_filter; /* fiter ID for receiving stuff */
const char* listenport; /* port where daemon listen for messages TCP->CAN */
const char* device; /* CAN device */

#endif

