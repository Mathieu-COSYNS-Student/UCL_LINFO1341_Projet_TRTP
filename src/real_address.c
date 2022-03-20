#include "real_address.h"

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

const char* real_address(const char* address, struct sockaddr* rval)
{
    struct addrinfo* res;

    int ret = getaddrinfo(address, NULL, NULL, &res);
    if (ret != 0) {
        return gai_strerror(ret);
    }

    *rval = *res->ai_addr;

    return NULL;
}
