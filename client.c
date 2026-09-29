#include "msg.h"

#include <arpa/inet.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* usage: ./client [count] [interval_ms]
 * Sends a stream of synthetic sensor readings so the dashboard has
 * something to plot. */
int main(int argc, char **argv)
{
    int count       = argc > 1 ? atoi(argv[1]) : 100;
    int interval_ms = argc > 2 ? atoi(argv[2]) : 200;

    static const char *labels[] = { "temperature", "pressure", "humidity" };
    const int nlabels = sizeof labels / sizeof labels[0];

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEMO_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) { perror("connect"); return 1; }
    printf("[client] connected to 127.0.0.1:%d\n", DEMO_PORT);

    for (int i = 0; i < count; i++) {
        message_t out, reply;
        int k = i % nlabels;

        out.value = 10.0f * (k + 1) + 5.0f * sinf(0.15f * i + k);
        snprintf(out.label, sizeof out.label, "%s", labels[k]);

        if (send_msg(fd, &out) < 0) { perror("send_msg"); break; }
        printf("[client] sent:     value=%.3f label=\"%s\"\n", out.value, out.label);

        if (recv_msg(fd, &reply) < 0) { fprintf(stderr, "[client] server closed\n"); break; }
        printf("[client] received: value=%.3f label=\"%s\"\n", reply.value, reply.label);

        usleep((useconds_t)interval_ms * 1000);
    }

    close(fd);
    return 0;
}
