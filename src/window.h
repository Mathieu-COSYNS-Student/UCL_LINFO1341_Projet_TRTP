#ifndef __WINDOW_H_
#define __WINDOW_H_

#include "packet.h"

typedef enum {
    SEND_WINDOW = 0,
    RECV_WINDOW
} window_type;

typedef enum {
    EMPTY_SLOT = 0,
    PKT_TO_ACK,
    PKT_TO_NACK,
    PKT_NEED_ACK,
    PKT_ACK_OK,
} pkt_window_status;

typedef struct {
    window_type type;
    pkt_t* pkts;
    pkt_window_status* pkts_status;
    uint8_t size;
    uint8_t peer_size;
    uint8_t start_pos;
    uint8_t end_pos;
    bool read_finished;
    bool write_finished;
} window_t;

window_t* window_new(window_type type);

void window_del(window_t* window);

bool window_set_size(window_t* window, const uint8_t size);

bool window_closed(window_t* window);

int window_seqnum(window_t* window);

uint8_t window_fill_size(window_t* window);

bool window_is_full(window_t* window);

bool window_has_pkt_to_send(window_t* window);

bool window_add_data_pkt(window_t* window, char* buffer, size_t buffer_len);

bool window_add_fec_pkt_if_needed(window_t* window);

pkt_t* window_remove_first_pkt(window_t* window);

pkt_t* next_pkt(window_t* window);

uint8_t window_next_seqnum(window_t* window, uint8_t seqnum);

#endif /* __WINDOW_H_ */