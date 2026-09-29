#include "msg.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEMO_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) { perror("connect"); return 1; }
    printf("[client] connected to 127.0.0.1:%d\n", DEMO_PORT);

    const message_t outgoing[] = {
        { 3.14f,   "pi" },
        { 9.81f,   "gravity" },
        { -40.0f,  "freezing point" },
    };

    for (size_t i = 0; i < sizeof outgoing / sizeof outgoing[0]; i++) {
        message_t reply;

        if (send_msg(fd, &outgoing[i]) < 0) { perror("send_msg"); break; }
        printf("[client] sent:     value=%.3f label=\"%s\"\n", outgoing[i].value, outgoing[i].label);

        if (recv_msg(fd, &reply) < 0) { fprintf(stderr, "[client] server closed\n"); break; }
        printf("[client] received: value=%.3f label=\"%s\"\n", reply.value, reply.label);
    }

    close(fd);
    return 0;
}
