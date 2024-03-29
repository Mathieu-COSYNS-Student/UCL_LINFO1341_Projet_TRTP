#ifndef __READ_WRITE_LOOP_H_
#define __READ_WRITE_LOOP_H_

#include <stdio.h>

#include "statistics.h"

typedef struct {
    bool fec_enabled;
} trtp_options_t;

/* Loop reading a socket, decoding packets and writing to output,
 * while reading input and encoding packets and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as input signals EOF
 */
void exchange_trtp(const int sfd, FILE* input, FILE* output, const trtp_options_t* const options, statistics_t* statistics);

#endif
