#include "window.h"

#include <stdlib.h>
#include <string.h>

window_t* window_new(window_type type)
{
    window_t* window = (window_t*)malloc(sizeof(window_t));
    memset(window, 0, sizeof(window_t));
    window->type = type;
    return window;
}

void window_del(window_t* window)
{
    if (window->size > 0) {
        free(window->pkts);
        free(window->pkts_status);
    }
    free(window);
}

bool window_set_size(window_t* window, const uint8_t size)
{
    if (size <= window->size)
        return true;

    if (window->pkts == NULL) {
        window->pkts = calloc(sizeof(pkt_t), size);
        if (window->pkts == NULL)
            return false;
        window->pkts_status = calloc(sizeof(pkt_window_status), size);
        if (window->pkts_status == NULL) {
            free(window->pkts);
            return false;
        }
    } else {
        window->pkts = reallocarray(window->pkts, sizeof(pkt_t), size);
        if (window->pkts == NULL)
            return false;
        window->pkts_status = reallocarray(window->pkts_status, sizeof(pkt_window_status), size);
        if (window->pkts_status == NULL) {
            free(window->pkts);
            return false;
        }
        for (uint8_t i = window->size; i < size; i++) {
            window->pkts_status[i] = EMPTY_SLOT;
        }
    }
    window->size = size;
    return true;
}

uint8_t window_next_seqnum(window_t* window, uint8_t seqnum)
{
    return (seqnum + 1) % window->size;
}

bool window_is_full(window_t* window)
{
    return window_fill_size(window) >= window->size;
}

bool window_add_pkt(window_t* window, pkt_t* pkt, pkt_window_status status)
{
    if (window_is_full(window)) {
        pkt_del(pkt);
        return false;
    }

    memcpy(window->pkts + window->end_pos, pkt, sizeof(pkt_t));
    pkt_del(pkt);
    window->pkts_status[window->end_pos] = status;

    window->end_pos = window_next_seqnum(window, window->end_pos);

    return true;
}

bool window_add_data_pkt(window_t* window, char* buffer, size_t buffer_len)
{
    if (window->type != SEND_WINDOW)
        return false;
    if (window_is_full(window)) {
        return false;
    }

    pkt_t* pkt = pkt_new();
    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_seqnum(pkt, window->end_pos);
    pkt_set_payload(pkt, buffer, buffer_len);

    return window_add_pkt(window, pkt, PKT_NEED_ACK);
}

bool window_add_fec_pkt_if_needed(window_t* window)
{
    if (window->type != SEND_WINDOW)
        return false;
    if (window_is_full(window)) {
        return false;
    }

    bool can_calc_fec = false;

    if (can_calc_fec) {
        // Create fec pkt

        // return window_add_pkt(window, pkt);
    }

    return false;
}

uint8_t window_fill_size(window_t* window)
{
    if (window->start_pos <= window->end_pos)
        return window->end_pos - window->start_pos;
    return window->size + window->end_pos - window->start_pos;
}

pkt_t* window_peek_first_pkt(window_t* window)
{
    pkt_t* pkt = pkt_new();
    memcpy(pkt, window->pkts + window->start_pos, sizeof(pkt_t));
    return pkt;
}

pkt_t* window_remove_first_pkt(window_t* window)
{
    pkt_window_status status = window->pkts_status[window->start_pos];
    if (status != PKT_ACK_OK && status != PKT_ACK_OK)
        return NULL;

    pkt_t* pkt = window_peek_first_pkt(window);

    window->pkts_status[window->start_pos] = EMPTY_SLOT;
    window->start_pos = window_next_seqnum(window, window->start_pos);

    return pkt;
}

int window_seqnum(window_t* window)
{
    int ack_seqnum = -1;
    for (uint8_t i = 0; i < window->size; i++) {
        if (window->pkts_status[i] != EMPTY_SLOT) {
            ack_seqnum = i;
        } else if (ack_seqnum != -1) {
            return ack_seqnum;
        }
    }

    return ack_seqnum;
}

bool window_closed(window_t* window)
{
    return window->read_finished && window->write_finished;
}

bool window_has_pkt_to_send(window_t* window)
{
    return window_fill_size(window) > 0;
}

pkt_t* next_pkt_send_window(window_t* window)
{
    if (!window_has_pkt_to_send(window))
        return NULL;

    while (window_remove_first_pkt(window)) { }

    return window_peek_first_pkt(window);
}

pkt_t* next_pkt_recv_window(window_t* window)
{
    if (!window_has_pkt_to_send(window))
        return NULL;
    return NULL;
}

pkt_t* next_pkt(window_t* window)
{
    if (window->type == SEND_WINDOW)
        return next_pkt_send_window(window);

    return next_pkt_recv_window(window);
}