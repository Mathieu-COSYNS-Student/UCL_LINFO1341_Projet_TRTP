#include <stdlib.h>

#include "packet_tests.h"
#include "testBrandon.h"

int main()
{
    if (run_packet_tests() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    if (run_test_brandon() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}