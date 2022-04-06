#include "address_tests.h"

#include <stdlib.h>
#include <string.h>

#include "../src/address.h"
#include "../src/log.h"

#define ADDRS_LEN 6

void get_ips(const char* addr, char* data, size_t size)
{
    char command[256];
    sprintf(command, "host %s | rev | cut -d' ' -f 1 | rev", addr);

    FILE* pf = popen(command, "r");
    if (pf == NULL)
        ERROR("Failed to open command stream");

    fgets(data, size, pf);

    if (pclose(pf) != 0)
        ERROR("Failed to close command stream");
}

int run_real_address_tests()
{
    const char* addrs[ADDRS_LEN] = {
        "0.0.0.0",
        "::1",
        "localhost",
        "www.google.com",
        "uclouvain.be",
        "intel12",
    };

    size_t i = 0;
    size_t number_of_tests_failed = 0;
    struct sockaddr_storage* rval = (struct sockaddr_storage*)malloc(sizeof(struct sockaddr_storage));

    for (; i < ADDRS_LEN; i++) {
        const char* err = real_address(addrs[i], rval);
        if (err) {
            ERROR("Real address failed for \"%s\": %s", addrs[i], err);
            number_of_tests_failed++;
        } else {
            char buf[64];
            if (human_readable_ip(buf, rval)) {
                if (strcmp(addrs[i], buf)) {

                    char data[256];
                    char command[256];
                    sprintf(command, "host %s | rev | cut -d' ' -f 1 | rev", addrs[i]);

                    FILE* pf = popen(command, "r");
                    if (pf == NULL)
                        ERROR("Failed to open command stream");

                    bool match = false;
                    while (!match && fgets(data, 256, pf)) {
                        data[strlen(data) - 1] = '\0';
                        if (!strcmp(buf, data))
                            match = true;
                    }

                    if (pclose(pf) != 0)
                        ERROR("Failed to close command stream");

                    if (match) {
                        print_ip(addrs[i], rval);
                    } else {
                        number_of_tests_failed++;
                        ERROR("\"%s\" IP address: %s incorrect.", addrs[i], buf);
                    }
                } else {
                    print_ip(addrs[i], rval);
                }
            } else {
                number_of_tests_failed++;
            }
        }
    }

    free(rval);

    char* test_name = "Real address tests";

    if (number_of_tests_failed) {
        TEST_FAILED(test_name, i, number_of_tests_failed);
        return EXIT_FAILURE;
    }

    TEST_SUCCESS(test_name, i);

    return EXIT_SUCCESS;
}
