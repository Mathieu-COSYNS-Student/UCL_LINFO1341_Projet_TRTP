#include "utils.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

long get_time_in_milliseconds()
{
    struct timespec spec;

    clock_gettime(CLOCK_BOOTTIME, &spec);

    return spec.tv_sec * 1000 + spec.tv_nsec / 1.0e6;
}
