#include "address.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "log.h"

bool human_readable_ip(char* buffer, const struct sockaddr_storage* addr)
{
    if (((struct sockaddr*)addr)->sa_family == AF_INET
        && !inet_ntop(AF_INET, (void*)&(((struct sockaddr_in*)addr)->sin_addr), buffer, 64)) {
        ERROR("%d: inet_ntop failed!", ((struct sockaddr_in*)addr)->sin_addr.s_addr);
        return false;
    } else if (((struct sockaddr*)addr)->sa_family == AF_INET6
        && !inet_ntop(AF_INET6, (void*)&(((struct sockaddr_in6*)addr)->sin6_addr), buffer, 64)) {
        ERROR("%s: inet_ntop failed!", ((struct sockaddr_in6*)addr)->sin6_addr.s6_addr);
        return false;
    }
    return true;
}

void print_ip(const char* addr_name, const struct sockaddr_storage* addr)
{
    char buf[64];
    if (human_readable_ip(buf, addr))
        INFO("\"%s\" IP address: %s", addr_name, buf);
}

const char* real_address(const char* address, struct sockaddr_storage* rval)
{
    struct addrinfo* res;

    int ret = getaddrinfo(address, NULL, NULL, &res);
    if (ret != 0) {
        return gai_strerror(ret);
    }

    if (res->ai_family == AF_INET) {
        memcpy(rval, res->ai_addr, sizeof(struct sockaddr_in));
    } else if (res->ai_family == AF_INET6) {
        memcpy(rval, res->ai_addr, sizeof(struct sockaddr_in6));
    }

    freeaddrinfo(res);

    return NULL;
}
