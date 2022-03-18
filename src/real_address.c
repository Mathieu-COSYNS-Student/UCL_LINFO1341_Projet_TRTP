#include "real_address.h"
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

const char* real_address(const char* address, struct sockaddr_in6* rval)
{
    struct addrinfo* res;

    if (getaddrinfo(address, NULL, NULL, &res)) {
        return "getaddrinfo error";
    }

    *rval = *((struct sockaddr_in6*)res->ai_addr);

    return NULL;
}
