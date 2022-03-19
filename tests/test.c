#include <stdlib.h>

#include "packet_tests.h"
#include "real_address_tests.h"

int main()
{
    if (run_packet_tests() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    if (run_real_address_tests() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}