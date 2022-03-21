#include "real_address_tests.h"

#include <arpa/inet.h>
#include <stdlib.h>

#include "../src/log.h"
#include "../src/real_address.h"

#define ADDRS_LEN 5

void print_ip(const char* addr_name, const struct sockaddr* addr)
{
    char buf[64];
    if (addr->sa_family == AF_INET
        && !inet_ntop(AF_INET, (void*)&(((struct sockaddr_in*)addr)->sin_addr), buf, sizeof(buf))) {
        ERROR("%d: inet_ntop failed!", ((struct sockaddr_in*)addr)->sin_addr.s_addr);
    } else if (addr->sa_family == AF_INET6
        && !inet_ntop(AF_INET6, (void*)&(((struct sockaddr_in6*)addr)->sin6_addr), buf, sizeof(buf))) {
        ERROR("%s: inet_ntop failed!", ((struct sockaddr_in6*)addr)->sin6_addr.s6_addr);
    } else {
        SUCCESS("\"%s\" IP address: %s", addr_name, buf);
    }
}

int run_real_address_tests()
{
    const char* addrs[ADDRS_LEN] = {
        "0.0.0.0",
        "::1",
        "localhost",
        "www.google.com",
        "uclouvain.be",
    };

    size_t i = 0;
    int number_of_tests_failed = 0;
    struct sockaddr* rval = (struct sockaddr*)malloc(sizeof(struct sockaddr));

    for (; i < ADDRS_LEN; i++) {
        const char* err = real_address(addrs[i], rval);
        if (err) {
            number_of_tests_failed++;
            ERROR("Real address failed for \"%s\": %s", addrs[i], err);
        } else {
            print_ip(addrs[i], rval);
        }
    }

    free(rval);

    char* test_name = "Real address tests";

    if (number_of_tests_failed) {
        TEST_FAILED(test_name, number_of_tests_failed);
        return EXIT_FAILURE;
    }

    TEST_SUCCESS(test_name, i);

    return EXIT_SUCCESS;
}
