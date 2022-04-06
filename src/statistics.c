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

void update_stats_from_valid_pkt_sent(pkt_t* pkt, statistics_t* statistics)
{
    if (pkt_get_type(pkt) == PTYPE_DATA) {
        statistics->data_sent++;
    }
    if (pkt_get_type(pkt) == PTYPE_ACK) {
        statistics->ack_sent++;
    }
    if (pkt_get_type(pkt) == PTYPE_NACK) {
        statistics->nack_sent++;
    }
    if (pkt_get_type(pkt) == PTYPE_FEC) {
        statistics->fec_sent++;
    }
}

void update_stats_from_valid_pkt_received(pkt_t* pkt, statistics_t* statistics)
{
    if (pkt_get_type(pkt) == PTYPE_DATA) {
        statistics->data_received++;
        if (pkt_get_tr(pkt)) {
            statistics->data_truncated_received++;
        }
    }
    if (pkt_get_type(pkt) == PTYPE_ACK) {
        statistics->ack_received++;
    }
    if (pkt_get_type(pkt) == PTYPE_NACK) {
        statistics->nack_received++;
    }
    if (pkt_get_type(pkt) == PTYPE_FEC) {
        statistics->fec_received++;
    }
}

void update_stats_rtt(size_t rtt, statistics_t* statistics)
{
    if (rtt < statistics->min_rtt)
        statistics->min_rtt = rtt;
    if (rtt > statistics->max_rtt)
        statistics->max_rtt = rtt;
}