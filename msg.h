#ifndef MSG_H
#define MSG_H

#include <stdint.h>

#define MSG_LABEL_MAX 255
#define DEMO_PORT 5555

typedef struct {
    float value;
    char  label[MSG_LABEL_MAX + 1];   /* NUL-terminated */
} message_t;

/* Return 0 on success, -1 on error or closed connection. */
int send_msg(int fd, const message_t *m);
int recv_msg(int fd, message_t *m);

#endif
