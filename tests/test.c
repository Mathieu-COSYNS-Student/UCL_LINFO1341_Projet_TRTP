#include <stdio.h>
#include <stdlib.h>

#include "packet_tests.h"
#include "real_address_tests.h"

int main()
{
    int packet_tests = run_packet_tests();
    int real_address_tests = run_real_address_tests();

    if (packet_tests != EXIT_SUCCESS
        || real_address_tests != EXIT_SUCCESS)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}