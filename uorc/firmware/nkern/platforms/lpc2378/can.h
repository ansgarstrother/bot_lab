#ifndef _CAN_H
#define _CAN_H

#include <stdint.h>

/*
CAN bus calculator:

http://www.port.de/engl/canprod/sv_req_form.html
http://www.kvaser.com/can/index.htm

*/

typedef struct
{
    uint64_t   timestamp; // valid on receipt only
    uint32_t   id;        // message id
    uint32_t   len;       // length of can message
    uint32_t   data[2];  
} can_message_t;

void can_init(uint32_t btr);

int can_write(int port, can_message_t *m);
int can_read(int port, can_message_t *m);
int can_overflow_count(int can_port);

#endif
