#include "real_address.h"

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

const char* real_address(const char* address, struct sockaddr* rval)
{
    struct addrinfo* res;

    int ret = getaddrinfo(address, NULL, NULL, &res);
    if (ret != 0) {
        return gai_strerror(ret);
    }

    memcpy(rval, res->ai_addr, sizeof(struct sockaddr));

    freeaddrinfo(res);

    return NULL;
}
