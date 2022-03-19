#include "real_address_tests.h"

#include <arpa/inet.h>
#include <stdlib.h>

#include "../src/log.h"
#include "../src/real_address.h"

void print_ip_v6(struct sockaddr_in6* addr)
{
    char buf[64];
    if (!inet_ntop(AF_INET6, (void*)&addr->sin6_addr, buf, sizeof(buf))) {
        ERROR("%s: inet_ntop failed!", addr->sin6_addr.s6_addr);
    } else {
        SUCCESS("IP address: %s", buf);
    }
}

int run_real_address_tests()
{
    struct sockaddr_in6* rval = (struct sockaddr_in6*)malloc(sizeof(struct sockaddr_in6));

    if (real_address("::", rval))
        return EXIT_FAILURE;
    print_ip_v6(rval);

    if (real_address("www.google.com", rval))
        return EXIT_FAILURE;
    print_ip_v6(rval);

    if (real_address("uclouvain.be", rval))
        return EXIT_FAILURE;
    print_ip_v6(rval);

    if (real_address("2a00:1450:400e:80c::2004", rval))
        return EXIT_FAILURE;
    print_ip_v6(rval);

    return EXIT_SUCCESS;
}
