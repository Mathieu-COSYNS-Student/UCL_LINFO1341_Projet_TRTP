#include "create_socket.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "log.h"

int create_socket(struct sockaddr* src_addr,
    int src_port,
    struct sockaddr* dst_addr,
    int dst_port)
{
    // Check args
    if (!src_addr && !dst_addr) {
        ERROR("src_addr and dst_addr can not be both NULL.");
        return -1;
    }

    if (src_port <= 0 && dst_port <= 0) {
        ERROR("src_port and dst_port can not be both less than or equals to 0");
        return -1;
    }

    if (src_port > 65535) {
        ERROR("src_port should be number less than or equals to 65535 and not %d.", src_port);
        return -1;
    }

    if (dst_port > 65535) {
        ERROR("dst_port should be number less than or equals to 65535 and not %d.", dst_port);
        return -1;
    }

    if (src_addr && dst_addr && src_addr->sa_family != dst_addr->sa_family) {
        ERROR("src_addr and dst_addr should have the same kind of ip address");
    }

    sa_family_t sa_family;

    if (src_addr) {
        sa_family = src_addr->sa_family;
    } else {
        sa_family = dst_addr->sa_family;
    }

    int sock = socket(sa_family, SOCK_DGRAM, 0);
    if (sock == -1) {
        ERROR("Could not create the SOCK_DGRAM socket: %s", strerror(errno));
        return -1;
    }

    if (src_addr) {
        if (src_port >= 0) {
            // Bind it to the source
            if (src_addr->sa_family == AF_INET)
                ((struct sockaddr_in*)src_addr)->sin_port = htons(src_port);
            else if (src_addr->sa_family == AF_INET6)
                ((struct sockaddr_in6*)src_addr)->sin6_port = htons(src_port);

            int err = bind(sock, src_addr, sizeof(*src_addr));
            if (err == -1) {
                ERROR("Could not bind on the socket.: %s", strerror(errno));
                return -1;
            }
        } else {
            ERROR("src_port should be greater than 0 when src_addr is not NULL");
            return -1;
        }
    }

    if (dst_addr) {
        if (dst_port >= 0) {
            // Connect it to the destination
            if (dst_addr->sa_family == AF_INET)
                ((struct sockaddr_in*)dst_addr)->sin_port = htons(dst_port);
            else if (dst_addr->sa_family == AF_INET6)
                ((struct sockaddr_in6*)dst_addr)->sin6_port = htons(dst_port);

            int err = connect(sock, dst_addr, sizeof(*dst_addr));
            if (err == -1) {
                ERROR("Could not connect the socket.: %s", strerror(errno));
                return -1;
            }
        } else {
            ERROR("dst_port should be greater than 0 when dst_addr is not NULL");
            return -1;
        }
    }

    return sock;
}