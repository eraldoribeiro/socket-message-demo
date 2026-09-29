#include "msg.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* The dashboard is optional: the server connects to it lazily before each
 * publish and drops the connection on any error, so the dashboard can be
 * started, stopped, or restarted at any time without affecting the client. */
static int gui_fd = -1;

static void gui_publish(const message_t *m)
{
    if (gui_fd < 0) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return;

        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(GUI_PORT);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
            close(fd);
            return;
        }
        gui_fd = fd;
        printf("[server] dashboard connected\n");
    }

    if (send_msg(gui_fd, m) < 0) {
        close(gui_fd);
        gui_fd = -1;
        printf("[server] dashboard disconnected\n");
    }
}

int main(void)
{
    /* Writing to a closed dashboard socket must return an error, not kill us. */
    signal(SIGPIPE, SIG_IGN);

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

        gui_publish(&out);
    }

    printf("[server] client disconnected\n");
    if (gui_fd >= 0) close(gui_fd);
    close(cfd);
    close(lfd);
    return 0;
}
