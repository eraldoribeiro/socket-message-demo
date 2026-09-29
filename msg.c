#include "msg.h"

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* Wire format (all integers in network byte order):
 *   uint32  value   IEEE-754 bits of the float
 *   uint8   len     label length in bytes, excluding the NUL
 *   char[len] label
 */

static int write_all(int fd, const void *buf, size_t n)
{
    const char *p = buf;
    while (n > 0) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += w;
        n -= (size_t)w;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t n)
{
    char *p = buf;
    while (n > 0) {
        ssize_t r = read(fd, p, n);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return -1;          /* peer closed */
        p += r;
        n -= (size_t)r;
    }
    return 0;
}

int send_msg(int fd, const message_t *m)
{
    uint32_t bits;
    memcpy(&bits, &m->value, sizeof bits);
    bits = htonl(bits);

    size_t len = strnlen(m->label, MSG_LABEL_MAX);
    uint8_t len8 = (uint8_t)len;

    if (write_all(fd, &bits, sizeof bits) < 0) return -1;
    if (write_all(fd, &len8, sizeof len8) < 0) return -1;
    return write_all(fd, m->label, len);
}

int recv_msg(int fd, message_t *m)
{
    uint32_t bits;
    uint8_t len8;

    if (read_all(fd, &bits, sizeof bits) < 0) return -1;
    if (read_all(fd, &len8, sizeof len8) < 0) return -1;
    if (read_all(fd, m->label, len8) < 0) return -1;
    m->label[len8] = '\0';

    bits = ntohl(bits);
    memcpy(&m->value, &bits, sizeof bits);
    return 0;
}
