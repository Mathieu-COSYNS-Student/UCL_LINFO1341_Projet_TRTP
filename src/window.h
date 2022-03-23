#ifndef __WINDOW_H_
#define __WINDOW_H_

#include "packet.h"
#include "statistics.h"

#define WINDOW_RESTRANMISSION_TIMEOUT 1800
#define WINDOW_SHUTDOWN_TIMEOUT 4000

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
    PKT_PREPARED,
} pkt_window_status;

typedef struct {
    pkt_t pkt;
    pkt_window_status status;
} pkt_window_element;

typedef struct {
    // data structure
    pkt_window_element* elements;
    uint8_t size;
    uint8_t logical_size;
    // window props
    window_type type;
    uint8_t peer_size;
    uint8_t seqnum;
    int32_t last_used_pkt_timestamp;
    long shutdown_time;
    bool read_finished;
    bool write_finished;
} window_t;

window_t* window_new(window_type type);

void window_del(window_t* window);

bool window_resize_if_needed(window_t* window, const uint8_t size);

bool window_is_full(window_t* window);

bool window_closed(window_t* window);

pkt_t* next_pkt(window_t* window, statistics_t* statistics);

bool window_add_data_pkt(window_t* window, char* buffer, size_t buffer_len);

bool window_add_fec_pkt_if_needed(window_t* window);

pkt_t* window_slide_if_possible(window_t* window, uint8_t shift);

void window_update_from_received_pkt(window_t* window, pkt_t* pkt, statistics_t* statistics);

#endif /* __WINDOW_H_ */