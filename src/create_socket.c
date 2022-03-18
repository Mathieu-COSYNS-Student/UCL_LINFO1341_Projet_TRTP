#include "create_socket.h"
#include "log.h"

int create_socket(struct sockaddr_in6* source_addr,
    int src_port,
    struct sockaddr_in6* dest_addr,
    int dst_port)
{
    // Check args
    if (!source_addr && !dest_addr) {
        ERROR("source_addr and dest_addr can not be both NULL.");
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

    // Create a IPv6 socket supporting datagrams
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock == -1) {
        ERROR("Could not create the IPv6 SOCK_DGRAM socket.");
        perror("Error");
        return -1;
    }

    if (source_addr) {
        if (src_port >= 0) {
            // Bind it to the source
            int err = bind(sock, (struct sockaddr*)source_addr, sizeof(source_addr));
            if (err == -1) {
                ERROR("Could not bind on the socket.");
                perror("Error");
                return -1;
            }
        } else {
            ERROR("src_port should be greater than 0 when source_addr is not NULL");
            return -1;
        }
    }

    if (dest_addr) {
        if (dst_port >= 0) {
            // Connect it to the destination
            int err = connect(sock, (struct sockaddr*)dest_addr, sizeof(dest_addr));
            if (err == -1) {
                ERROR("Cound not connect the socket.");
                perror("Error");
                return -1;
            }
        } else {
            ERROR("dst_port should be greater than 0 when dest_addr is not NULL");
            return -1;
        }
    }

    return 0;
}