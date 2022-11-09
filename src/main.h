#ifndef MAIN_H
#define MAIN_H

#define GL_OWN_CAN_ID 0x00
#define WEBSOCK_MAX_RECV 16000

extern uint8_t verbose; /* defines if program runs in verbose mode */
extern uint8_t run_daemon; /* defines if program runs in daemon mode */
extern uint8_t virtualcan; /* defines if virtual can vcan0 is used */
extern uint8_t rec_filter; /* fiter ID for receiving stuff */
extern const char* listenport; /* port where daemon listen for messages TCP->CAN */
extern const char* device; /* CAN device */

#endif
