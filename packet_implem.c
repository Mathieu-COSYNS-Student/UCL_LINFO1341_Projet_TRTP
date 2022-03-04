#include "packet_interface.h"

/* Extra #includes */
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

struct __attribute__((__packed__)) pkt {
    ptypes_t type : 2;
    unsigned tr : 1;
    unsigned window : 5;
    uint16_t length;
    uint8_t seqnum;
    uint32_t timestamp;
    uint32_t crc1;
    char payload[MAX_PAYLOAD_SIZE];
    uint32_t crc2;
};

struct __attribute__((__packed__)) ack_nack_pkt {
    ptypes_t type : 2;
    unsigned tr : 1;
    unsigned window : 5;
    uint8_t seqnum;
    uint32_t timestamp;
    uint32_t crc1;
};
typedef struct ack_nack_pkt ack_nack_pkt_t;

/* Extra code */
int is_ack_nack_pkt(const pkt_t* pkt)
{
    uint8_t tr = pkt_get_tr(pkt);
    return tr == PTYPE_ACK || tr == PTYPE_NACK;
}

pkt_t* pkt_new()
{
    return (pkt_t*)malloc(sizeof(pkt_t));
}

void pkt_del(pkt_t* pkt)
{
    free(pkt);
}

pkt_status_code pkt_decode(const char* data, const size_t len, pkt_t* pkt)
{
    /* Your code will be inserted here */
    if (pkt == NULL)
        fprintf(stderr, "%c%ld", data[0], len);
    return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char* buf, size_t* len)
{
    /* Your code will be inserted here */
    if (pkt == NULL)
        fprintf(stderr, "%c%ld", buf[0], *len);
    return PKT_OK;
}

ptypes_t pkt_get_type(const pkt_t* pkt)
{
    return pkt->type;
}

uint8_t pkt_get_tr(const pkt_t* pkt)
{
    return pkt->tr;
}

uint8_t pkt_get_window(const pkt_t* pkt)
{
    return pkt->window;
}

uint8_t pkt_get_seqnum(const pkt_t* pkt)
{
    if (is_ack_nack_pkt(pkt)) {
        ack_nack_pkt_t* ack_nack_pkt = (ack_nack_pkt_t*)pkt;
        return ack_nack_pkt->seqnum;
    }
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
    if (is_ack_nack_pkt(pkt)) {
        return 0;
    }
    return ntohl(pkt->length);
}

uint32_t pkt_get_timestamp(const pkt_t* pkt)
{
    if (is_ack_nack_pkt(pkt)) {
        ack_nack_pkt_t* ack_nack_pkt = (ack_nack_pkt_t*)pkt;
        return ack_nack_pkt->timestamp;
    }
    return pkt->timestamp;
}

uint32_t pkt_get_crc1(const pkt_t* pkt)
{
    if (is_ack_nack_pkt(pkt)) {
        ack_nack_pkt_t* ack_nack_pkt = (ack_nack_pkt_t*)pkt;
        return ack_nack_pkt->crc1;
    }
    return pkt->crc1;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
    if (is_ack_nack_pkt(pkt)) {
        return NULL;
    }
    return pkt->payload;
}

uint32_t pkt_get_crc2(const pkt_t* pkt)
{
    if (is_ack_nack_pkt(pkt)) {
        return 0;
    }

    uint32_t crc2;
    memcpy(&crc2, pkt + predict_header_length(pkt) + pkt_get_length(pkt), sizeof(uint32_t));

    return crc2;
}

pkt_status_code pkt_set_type(pkt_t* pkt, const ptypes_t type)
{
    pkt->type = type;
    return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t* pkt, const uint8_t tr)
{
    if (tr != 0 && tr != 1)
        return E_TR;
    pkt->tr = tr;
    return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t* pkt, const uint8_t window)
{
    if (window > MAX_WINDOW_SIZE)
        return E_WINDOW;
    pkt->window = window;
    return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t* pkt, const uint8_t seqnum)
{
    if (is_ack_nack_pkt(pkt)) {
        ack_nack_pkt_t* ack_nack_pkt = (ack_nack_pkt_t*)pkt;
        ack_nack_pkt->seqnum = seqnum;
        return PKT_OK;
    }
    pkt->seqnum = seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t* pkt, const uint16_t length)
{
    if (is_ack_nack_pkt(pkt)) {
        return E_LENGTH;
    }
    pkt->length = length;
    return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t* pkt, const uint32_t timestamp)
{
    if (is_ack_nack_pkt(pkt)) {
        ack_nack_pkt_t* ack_nack_pkt = (ack_nack_pkt_t*)pkt;
        ack_nack_pkt->timestamp = timestamp;
        return PKT_OK;
    }
    pkt->timestamp = timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t* pkt, const uint32_t crc1)
{
    if (is_ack_nack_pkt(pkt)) {
        ack_nack_pkt_t* ack_nack_pkt = (ack_nack_pkt_t*)pkt;
        ack_nack_pkt->crc1 = crc1;
        return PKT_OK;
    }
    pkt->crc1 = crc1;
    return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t* pkt, const uint32_t crc2)
{
    if (is_ack_nack_pkt(pkt)) {
        return E_CRC;
    }
    memcpy(pkt + predict_header_length(pkt) + pkt_get_length(pkt), &crc2, sizeof(uint32_t));
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t* pkt,
    const char* data,
    const uint16_t length)
{
    pkt_set_length(pkt, length);

    memcpy(pkt + predict_header_length(pkt), data, length);

    return PKT_OK;
}

ssize_t predict_header_length(const pkt_t* pkt)
{
    if (is_ack_nack_pkt(pkt))
        return PKT_MIN_HEADERLEN;
    return PKT_MAX_HEADERLEN;
}
