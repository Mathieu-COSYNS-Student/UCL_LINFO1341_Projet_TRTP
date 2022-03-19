#include <stdio.h>

#include "log.h"
#include "statistics.h"

FILE* write_common_stats(const char* pathname, statistics_t* statistics)
{
    FILE* file = stderr;

    if (pathname) {
        file = fopen(pathname, "w");
        DEBUG("Writing stats on %s", pathname);
    } else {
        DEBUG("Writing stats on stderr");
    }

    if (!file)
        return NULL;

    if (fprintf(file, "data_sent:%ld\n", statistics->data_sent) <= 0)
        return NULL;
    if (fprintf(file, "data_received:%ld\n", statistics->data_received) <= 0)
        return NULL;
    if (fprintf(file, "data_truncated_received:%ld\n", statistics->data_truncated_received) <= 0)
        return NULL;
    if (fprintf(file, "fec_sent:%ld\n", statistics->fec_sent) <= 0)
        return NULL;
    if (fprintf(file, "fec_received:%ld\n", statistics->fec_received) <= 0)
        return NULL;
    if (fprintf(file, "ack_sent:%ld\n", statistics->ack_sent) <= 0)
        return NULL;
    if (fprintf(file, "ack_received:%ld\n", statistics->ack_received) <= 0)
        return NULL;
    if (fprintf(file, "nack_sent:%ld\n", statistics->nack_sent) <= 0)
        return NULL;
    if (fprintf(file, "nack_received:%ld\n", statistics->nack_received) <= 0)
        return NULL;
    if (fprintf(file, "packet_ignored:%ld\n", statistics->packet_ignored) <= 0)
        return NULL;

    return file;
}

int write_sender_stats(const char* pathname, statistics_t* statistics)
{
    FILE* file = write_common_stats(pathname, statistics);

    if (!file)
        return 0;

    if (fprintf(file, "min_rtt:%ld\n", statistics->min_rtt) <= 0)
        return 0;
    if (fprintf(file, "max_rtt:%ld\n", statistics->max_rtt) <= 0)
        return 0;
    if (fprintf(file, "packet_retransmitted:%ld\n", statistics->packet_retransmitted) <= 0)
        return 0;

    if (fclose(file) != 0)
        return 0;

    return 1;
}

int write_receiver_stats(const char* pathname, statistics_t* statistics)
{
    FILE* file = write_common_stats(pathname, statistics);

    if (!file)
        return 0;

    if (fprintf(file, "packet_duplicated:%ld\n", statistics->packet_duplicated) <= 0)
        return 0;
    if (fprintf(file, "packet_recovered:%ld\n", statistics->packet_recovered) <= 0)
        return 0;

    if (fclose(file) != 0)
        return 0;

    return 1;
}