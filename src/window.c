#include "window.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "statistics.h"

window_t* window_new(window_type type)
{
    window_t* window = (window_t*)calloc(sizeof(window_t), 1);
    if (window) {
        window->type = type;
    }
    return window;
}

void window_del(window_t* window)
{
    if (!window)
        return;

    if (window->size > 0) {
        free(window->elements);
    }
    free(window);
}

bool window_resize_if_needed(window_t* window, const uint8_t size)
{
    if (!window)
        return false;
    if (size == window->size)
        return true;

    if (window->elements == NULL) {
        window->elements = calloc(sizeof(pkt_window_element), size);
        if (window->elements == NULL)
            return false;
    } else {
        pkt_window_element* elements = reallocarray(window->elements, sizeof(pkt_window_element), size);
        if (window->elements == NULL)
            return false;
        window->elements = elements;
        if (size > window->size)
            memset(window->elements + window->size, 0, (window->size - size) * sizeof(pkt_window_element));
    }
    window->size = size;
    return true;
}

bool window_is_full(window_t* window)
{
    return !window || window->logical_size >= window->size;
}

bool window_cmp_pkt(window_t* window, pkt_t* pkt, ssize_t index)
{
    return memcmp(&(window->elements + index)->pkt, pkt, sizeof(pkt_t));
}

bool window_add_pkt(window_t* window, pkt_t* pkt, pkt_window_status status, ssize_t index)
{
    if (!window)
        return false;

    if (index == -1) {
        index = window->logical_size;
    }

    if (index >= window->size) {
        return false;
    }

    pkt_window_element* element = window->elements + index;

    if (element->status != EMPTY_SLOT)
        return false;

    memcpy(&element->pkt, pkt, sizeof(pkt_t));
    element->status = status;

    window->logical_size = index + 1;

    return true;
}

pkt_t* window_peek_first_pkt(window_t* window)
{
    if (!window)
        return NULL;
    if (window->logical_size == 0)
        return NULL;
    if (window->elements->status == EMPTY_SLOT)
        return NULL;
    pkt_t* pkt = pkt_new();
    memcpy(pkt, &window->elements->pkt, sizeof(pkt_t));
    return pkt;
}

pkt_t* window_remove_first_pkt(window_t* window, uint8_t shift)
{
    pkt_t* pkt = window_peek_first_pkt(window);
    if (!pkt)
        return NULL;
    for (uint8_t i = 0; i < window->logical_size; i++) {
        memcpy(window->elements + i, window->elements + i + shift, sizeof(pkt_window_element));
    }
    for (uint8_t i = window->logical_size - shift; i < window->logical_size; i++) {
        (window->elements + i)->status = EMPTY_SLOT;
    }

    window->logical_size -= shift;
    return pkt;
}

bool window_is_valid(window_t* window)
{
    return window && (window->type == SEND_WINDOW || window->type == RECV_WINDOW);
}

uint8_t next_seqnum(uint8_t seqnum)
{
    return (seqnum + 1) % (1 << 8);
}

bool window_closed(window_t* window)
{
    return !window_is_valid(window) || (window->read_finished && window->write_finished);
}

bool window_add_data_pkt(window_t* window, char* buffer, size_t buffer_len)
{
    if (!window_is_valid(window))
        return false;
    if (window->type != SEND_WINDOW)
        return false;
    if (window_is_full(window)) {
        return false;
    }

    pkt_t* pkt = pkt_new();
    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_seqnum(pkt, window->seqnum);
    pkt_set_payload(pkt, buffer, buffer_len);

    window->seqnum = next_seqnum(window->seqnum);

    bool packet_added = window_add_pkt(window, pkt, PKT_PREPARED, -1);

    pkt_del(pkt);

    return packet_added;
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

pkt_t* window_slide_if_possible(window_t* window, uint8_t shift)
{
    pkt_t* pkt = window_peek_first_pkt(window);
    if (!pkt)
        return NULL;

    pkt_window_status status = window->elements->status;

    if (status != PKT_ACK_OK)
        return NULL;

    window_remove_first_pkt(window, shift);

    return pkt;
}

pkt_t* build_next_pkt_send_window(pkt_window_element* element)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &time);
    uint32_t timestamp_now = time.tv_sec + (time.tv_nsec / 1000000);

    pkt_set_timestamp(&element->pkt, timestamp_now);

    pkt_t* pkt = pkt_new();
    memcpy(pkt, &element->pkt, sizeof(pkt_t));

    return pkt;
}

pkt_t* next_pkt_send_window(window_t* window, statistics_t* statistics)
{
    for (size_t i = 0; i < fminl(window->peer_size, window->logical_size); i++) {
        pkt_window_element* element = window->elements + i;
        if (element->status == PKT_PREPARED) {
            element->status = PKT_NEED_ACK;
            return build_next_pkt_send_window(element);
        } else if (element->status == PKT_NEED_ACK) {
            struct timespec time;
            clock_gettime(CLOCK_MONOTONIC_COARSE, &time);
            long timestamp_now = time.tv_sec + (time.tv_nsec / 1000000);

            if (((long)pkt_get_timestamp(&element->pkt)) + WINDOW_RESTRANMISSION_TIMEOUT <= timestamp_now) {

                statistics->packet_retransmitted++;

                return build_next_pkt_send_window(element);
            }
        } else {
            ERROR("There is a unexpected packet in the sending window");
            return NULL;
        }
    }

    return NULL;
}

pkt_t* build_next_pkt_recv_window(window_t* window, ptypes_t pkt_type)
{
    pkt_t* pkt = pkt_new();
    pkt_set_type(pkt, pkt_type);
    pkt_set_window(pkt, window->size);

    return pkt;
}

pkt_t* next_pkt_recv_window(window_t* window)
{
    if (window->logical_size == 0)
        return NULL;

    pkt_t* pkt = NULL;
    ssize_t to_ack = -1;

    for (size_t i = 0; i < window->logical_size; i++) {
        pkt_window_element* element = window->elements + i;
        if (element->status == PKT_TO_ACK) {
            element->status = PKT_ACK_OK;
            to_ack = pkt_get_seqnum(&element->pkt);
        } else {
            if (to_ack != -1) {
                break;
            }

            if (element->status == PKT_TO_NACK) {
                pkt = build_next_pkt_recv_window(window, PTYPE_NACK);
                pkt_set_seqnum(pkt, element->pkt.seqnum);
                break;
            }
        }
    }

    if (to_ack != -1 && !pkt) {
        pkt = build_next_pkt_recv_window(window, PTYPE_ACK);
        pkt_set_seqnum(pkt, next_seqnum(to_ack));
    }

    if (window->write_finished) {
        window->read_finished = true;
    }

    return pkt;
}

pkt_t* next_pkt(window_t* window, statistics_t* statistics)
{
    if (!window_is_valid(window))
        return NULL;

    if (window->type == SEND_WINDOW)
        return next_pkt_send_window(window, statistics);

    return next_pkt_recv_window(window);
}

void window_update_from_received_pkt(window_t* window, pkt_t* pkt, statistics_t* statistics)
{
    if (!window_is_valid(window))
        return;

    ptypes_t type = pkt_get_type(pkt);

    if (window->type == SEND_WINDOW) {
        uint8_t peer_size = pkt_get_window(pkt);
        if (window_resize_if_needed(window, peer_size)) {
            window->peer_size = peer_size;
        }

        if (type == PTYPE_ACK) {
            // TODO seqnum
            window->elements[0].status = PKT_ACK_OK;
            window_slide_if_possible(window, 1);

            if (window->write_finished && window->logical_size == 0) {
                window->read_finished = true;
            }

            update_stats_rtt(0, statistics);
            return;
        } else if (type == PTYPE_NACK) {
            // TODO
            update_stats_rtt(0, statistics);
            return;
        }
    } else if (window->type == RECV_WINDOW) {
        if (type == PTYPE_DATA) {
            bool added = false;
            // TODO seqnum
            ssize_t window_index = -1;
            if (pkt_get_tr(pkt)) {
                added = window_add_pkt(window, pkt, PKT_TO_NACK, window_index);
            } else {
                added = window_add_pkt(window, pkt, PKT_TO_ACK, window_index);
                if (added && pkt_get_length(pkt) == 0) {
                    window->read_finished = true;
                }
            }

            if (!added && !window_cmp_pkt(window, pkt, window_index)) {
                statistics->packet_duplicated++;
            }
            return;
        } else if (type == PTYPE_FEC) {
            // TODO
            if (0) {
                statistics->packet_duplicated++;
            }
            if (0) {
                statistics->packet_recovered++;
            }
            return;
        }
    }

    statistics->packet_ignored++;
}