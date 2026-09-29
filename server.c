#include "msg.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) { perror("socket"); return 1; }

    int yes = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEMO_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   /* 127.0.0.1 only */

    if (bind(lfd, (struct sockaddr *)&addr, sizeof addr) < 0) { perror("bind"); return 1; }
    if (listen(lfd, 1) < 0) { perror("listen"); return 1; }

    printf("[server] listening on 127.0.0.1:%d\n", DEMO_PORT);

    int cfd = accept(lfd, NULL, NULL);
    if (cfd < 0) { perror("accept"); return 1; }
    printf("[server] client connected\n");

    message_t in, out;
    while (recv_msg(cfd, &in) == 0) {
        printf("[server] received: value=%.3f label=\"%s\"\n", in.value, in.label);

        out.value = in.value * 2.0f;
        snprintf(out.label, sizeof out.label, "doubled %s", in.label);

        if (send_msg(cfd, &out) < 0) { perror("send_msg"); break; }
        printf("[server] sent:     value=%.3f label=\"%s\"\n", out.value, out.label);
    }

    printf("[server] client disconnected\n");
    close(cfd);
    close(lfd);
    return 0;
}
