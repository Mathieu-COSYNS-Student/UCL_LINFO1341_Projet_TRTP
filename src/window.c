#include "window.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "statistics.h"
#include "utils.h"

window_t* window_new(window_type type, uint8_t initial_capacity)
{
    window_t* window = (window_t*)calloc(sizeof(window_t), 1);
    if (window) {
        window->type = type;
        window->shutdown_time = -1;
        window->queue = queue_new(sizeof(pkt_window_element), initial_capacity);
        if (!window->queue) {
            free(window);
            return NULL;
        }
    }
    return window;
}

void window_del(window_t* window)
{
    if (!window)
        return;

    queue_del(window->queue);
    free(window);
}

bool window_is_full(window_t* window)
{
    return !window || queue_is_full(window->queue);
}

bool window_are_pkt_same(window_t* window, size_t index, pkt_t* pkt)
{
    pkt_window_element* element = queue_get_item(window->queue, index);
    pkt_t* new_pkt_1 = pkt_copy(&element->pkt);
    pkt_t* new_pkt_2 = pkt_copy(pkt);

    pkt_set_timestamp(new_pkt_1, 0);
    pkt_set_timestamp(new_pkt_2, 0);

    bool result = !memcmp(new_pkt_1, new_pkt_2, sizeof(pkt_t));

    pkt_del(new_pkt_1);
    pkt_del(new_pkt_2);

    return result;
}

bool window_is_valid(window_t* window)
{
    return window && (window->type == SEND_WINDOW || window->type == RECV_WINDOW);
}

int window_get_index_for_seqnum(window_t* window, uint8_t seqnum)
{
    uint8_t first_seqnum = window->seqnum;
    uint8_t last_seqnum = window->seqnum + queue_get_capacity(window->queue);
    // if (window->type == SEND_WINDOW)
    //     last_seqnum--;
    bool overflow = first_seqnum > last_seqnum;
    if (!overflow) {
        if (seqnum >= first_seqnum && seqnum < last_seqnum) {
            return seqnum - first_seqnum;
        }
        return -1;
    }
    if (seqnum >= first_seqnum || seqnum <= last_seqnum) {
        uint8_t ret = seqnum - first_seqnum;
        return ret;
    }
    return -1;
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
    pkt_set_seqnum(pkt, window->seqnum + queue_get_size(window->queue));
    pkt_set_payload(pkt, buffer, buffer_len);

    pkt_window_element element;
    element.pkt = *pkt;
    element.status = PKT_PREPARED;

    bool packet_added = queue_enqueue(window->queue, &element);

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
    if (!window)
        return NULL;

    pkt_window_element* element = queue_peek(window->queue);

    if (!element)
        return NULL;

    pkt_window_status status = element->status;

    if (window->type == RECV_WINDOW && status == PKT_ACK_OK && pkt_get_length(&element->pkt) != 0) {
        element = queue_get_item(window->queue, 1);
        status = element->status;
        // DEBUG("index=1 seqnum=%d, status=%d", element->pkt.seqnum, status);
    }

    if (status != PKT_ACK_OK)
        return NULL;

    element = queue_dequeue_from_start_to_index(window->queue, shift - 1);

    if (pkt_get_length(&element->pkt) != 0)
        window->seqnum = window->seqnum + shift;

    pkt_t* pkt = pkt_new();

    if (!pkt)
        return NULL;

    memcpy(pkt, &element->pkt, sizeof(pkt_t));
    free(element);

    return pkt;
}

pkt_t* build_next_pkt_send_window(pkt_window_element* element)
{
    long time = get_time_in_milliseconds();
    pkt_set_timestamp(&element->pkt, time);

    pkt_t* pkt = pkt_new();
    memcpy(pkt, &element->pkt, sizeof(pkt_t));

    return pkt;
}

pkt_t* next_pkt_send_window(window_t* window, statistics_t* statistics)
{
    bool waiting_for_ack;
    for (size_t i = 0; i < fminl(window->peer_size, queue_get_size(window->queue)); i++) {
        pkt_window_element* element = queue_get_item(window->queue, i);
        if (element->status == PKT_PREPARED || element->status == PKT_TRUNCATED) {
            if (element->status == PKT_TRUNCATED) {
                statistics->packet_retransmitted++;
            }

            element->status = PKT_NEED_ACK;
            return build_next_pkt_send_window(element);
        } else if (element->status == PKT_NEED_ACK) {
            waiting_for_ack = true;
            long time = get_time_in_milliseconds();

            if (((long)pkt_get_timestamp(&element->pkt)) + WINDOW_RESTRANMISSION_TIMEOUT <= time) {

                DEBUG("Retransmission timer expired for packet #%d", pkt_get_seqnum(&element->pkt));
                statistics->packet_retransmitted++;

                return build_next_pkt_send_window(element);
            }
        } else {
            if (waiting_for_ack && element->status == PKT_ACK_OK) {
                return NULL;
            }
            ERROR("There is a unexpected packet in the sending window at index %ld. status=%d", i, element->status);
            DEBUG_DUMP(queue_get_item(window->queue, 0), queue_get_size(window->queue) * sizeof(pkt_window_element));
            exit(1);
            return NULL;
        }
    }

    return NULL;
}

pkt_t* build_next_pkt_recv_window(window_t* window, ptypes_t pkt_type)
{
    pkt_t* pkt = pkt_new();
    pkt_set_type(pkt, pkt_type);
    pkt_set_window(pkt, queue_get_capacity(window->queue) - 1);
    pkt_set_timestamp(pkt, window->last_used_pkt_timestamp);

    return pkt;
}

void print_window(window_t* window)
{
    fprintf(stderr, "seqnum_in_window: ");
    for (size_t i = 0; i < queue_get_size(window->queue); i++) {
        pkt_window_element* element = queue_get_item(window->queue, i);
        fprintf(stderr, "[%ld]: seqnum=%d, status=%d, ", i, pkt_get_seqnum(&element->pkt), element->status);
    }
    fprintf(stderr, "\n");
}

pkt_t* next_pkt_recv_window(window_t* window)
{
    if (queue_is_empty(window->queue))
        return NULL;

    pkt_t* pkt = NULL;
    bool has_ack_to_send = false;
    uint8_t to_ack = -1;

    for (size_t i = 0; i < queue_get_size(window->queue); i++) {
        pkt_window_element* element = queue_get_item(window->queue, i);
        if (element->status == EMPTY_SLOT)
            break;
        // DEBUG("next_pkt_recv_window: index=%ld, status=%d, seqnum=%d, has_ack_to_send=%d", i, element->status, pkt_get_seqnum(&element->pkt), has_ack_to_send);
        if (element->status == PKT_TO_ACK) {
            element->status = PKT_ACK_OK;
            to_ack = pkt_get_seqnum(&element->pkt);
            has_ack_to_send = true;
        } else {
            if (has_ack_to_send) {
                break;
            }

            if (element->status == PKT_TO_NACK) {
                element->status = EMPTY_SLOT;
                pkt = build_next_pkt_recv_window(window, PTYPE_NACK);
                pkt_set_seqnum(pkt, element->pkt.seqnum);
                break;
            }
        }
    }

    if (has_ack_to_send && !pkt) {
        pkt = build_next_pkt_recv_window(window, PTYPE_ACK);
        pkt_set_seqnum(pkt, to_ack + ((uint8_t)1));
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
    uint8_t seqnum = pkt_get_seqnum(pkt);
    if (window->type == SEND_WINDOW && type == PTYPE_ACK)
        seqnum--;
    int window_index = window_get_index_for_seqnum(window, seqnum);
    DEBUG("Packet #%d received with type=%d, tr=%d, window_seqnum=%d, index=%d",
        pkt_get_seqnum(pkt), type, pkt_get_tr(pkt), window->seqnum, window_index);

    if (window_index != -1) {
        if (window->type == SEND_WINDOW) {
            if (type == PTYPE_ACK || type == PTYPE_NACK) {
                uint8_t peer_size = pkt_get_window(pkt);
                queue_resize(window->queue, peer_size);
                window->peer_size = peer_size;

                if (type == PTYPE_ACK) {
                    for (int i = 0; i <= window_index; i++) {
                        ((pkt_window_element*)queue_get_item(window->queue, i))->status = PKT_ACK_OK;
                    }
                    free(window_slide_if_possible(window, window_index + 1));

                    if (window->write_finished && queue_is_empty(window->queue)) {
                        window->read_finished = true;
                    }
                } else if (type == PTYPE_NACK) {
                    pkt_window_element* element = queue_get_item(window->queue, window_index);
                    if (element->status != PKT_ACK_OK)
                        element->status = PKT_TRUNCATED;
                }
                update_stats_rtt(0, statistics);
                return;
            }
        } else if (window->type == RECV_WINDOW) {
            if (type == PTYPE_DATA || type == PTYPE_FEC) {

                if (window_are_pkt_same(window, window_index, pkt)) {
                    statistics->packet_duplicated++;
                    return;
                }

                pkt_window_element* previous = queue_get_item(window->queue, window_index);

                if (type == PTYPE_DATA) {
                    pkt_window_element element = { 0 };
                    element.pkt = *pkt;
                    if (pkt_get_tr(pkt)) {
                        if (previous->status == EMPTY_SLOT)
                            element.status = PKT_TO_NACK;
                    } else {
                        element.status = PKT_TO_ACK;
                        if (pkt_get_length(pkt) == 0) {
                            window->shutdown_time = get_time_in_milliseconds() + WINDOW_SHUTDOWN_TIMEOUT;
                        }
                    }

                    if (element.status != EMPTY_SLOT && queue_enqueue_at_index(window->queue, &element, window_index)) {
                        window->last_used_pkt_timestamp = pkt_get_timestamp(pkt);
                    }
                } else if (type == PTYPE_FEC) {
                    // TODO
                    if (0) {
                        window->last_used_pkt_timestamp = pkt_get_timestamp(pkt);
                        statistics->packet_recovered++;
                    }
                }
                return;
            }
        }
    }

    statistics->packet_ignored++;
}