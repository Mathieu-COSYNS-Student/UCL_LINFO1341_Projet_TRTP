#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "address_tests.h"
#include "packet_tests.h"
#include "queue_tests.h"

typedef int (*TestsCallback)();
TestsCallback tests[] = {
    &run_packet_tests,
    &run_real_address_tests,
    &run_queue_tests,
    NULL
};

int main()
{
    bool success = true;
    for (size_t i = 0; tests[i]; i++) {
        int test_result = tests[i]();
        success = success && test_result == EXIT_SUCCESS;
    }

    if (!success)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}