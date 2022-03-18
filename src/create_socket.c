#include "create_socket.h"
#include <stdio.h>

int create_socket(struct sockaddr_in6* src_addr,
    int src_port,
    struct sockaddr_in6* dst_addr,
    int dst_port)
{
    // Check args
    if (!src_addr && !dst_addr) {
        fprintf(stderr, "src_addr and dst_addr can not be both NULL.");
        return -1;
    }

    if (src_port <= 0 && dst_port <= 0) {
        fprintf(stderr, "src_port and dst_port can not be both less than or equals to 0");
        return -1;
    }

    if (src_port > 65535) {
        fprintf(stderr, "src_port should be number less than or equals to 65535 and not %d.", src_port);
        return -1;
    }

    if (dst_port > 65535) {
        fprintf(stderr, "dst_port should be number less than or equals to 65535 and not %d.", dst_port);
        return -1;
    }

    // Create a IPv6 socket supporting datagrams
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock == -1) {
        fprintf(stderr, "Could not create the IPv6 SOCK_DGRAM socket.");
        perror("Error");
        return -1;
    }

    if (src_addr) {
        if (src_port >= 0) {
            // Bind it to the source
            src_addr->sin6_port = htons(src_port);
            int err = bind(sock, (struct sockaddr*)src_addr, sizeof(*src_addr));
            if (err == -1) {
                fprintf(stderr, "Could not bind on the socket.");
                perror("Error");
                return -1;
            }
        } else {
            fprintf(stderr, "src_port should be greater than 0 when src_addr is not NULL");
            return -1;
        }
    }

    if (dst_addr) {
        if (dst_port >= 0) {
            // Connect it to the destination
            dst_addr->sin6_port = htons(dst_port);
            int err = connect(sock, (struct sockaddr*)dst_addr, sizeof(*dst_addr));
            if (err == -1) {
                fprintf(stderr, "Could not connect the socket.");
                perror("Error");
                return -1;
            }
        } else {
            fprintf(stderr, "dst_port should be greater than 0 when dst_addr is not NULL");
            return -1;
        }
    }

    return sock;
}