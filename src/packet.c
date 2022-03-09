#include "packet.h"

/* Extra #includes */
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

struct __attribute__((__packed__)) pkt {
    uint8_t type_tr_window;
    uint16_t length;
    uint8_t seqnum;
    uint32_t timestamp;
    uint32_t crc1;
    char payload[MAX_PAYLOAD_SIZE];
    uint32_t crc2;
};

/* Extra code */
int pkt_is_ack_nack(const pkt_t* pkt)
{
    uint8_t tr = pkt_get_tr(pkt);
    return tr == PTYPE_ACK || tr == PTYPE_NACK;
}

uint32_t calc_header_crc(const char* buf, ssize_t len)
{
    char bufcpy[PKT_MAX_HEADERLEN - 4];
    memcpy(bufcpy, buf, len);

    bufcpy[0] = buf[0] & 0xdf;
    uLong crc = htonl(crc32(0L, (Bytef*)bufcpy, len));

    return crc;
}

uint32_t calc_payload_crc(const char* buf, ssize_t len)
{
    return htonl(crc32(0L, (Bytef*)buf, len));
}

void calc_and_write_header_crc(char* buf, ssize_t len)
{
    uint32_t crc = calc_header_crc(buf, len);
    memcpy(buf + len, &crc, sizeof(uint32_t));
}

void calc_and_write_payload_crc(char* buf, ssize_t len)
{
    uint32_t crc = calc_payload_crc(buf, len);
    memcpy(buf + len, &crc, sizeof(uint32_t));
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
    if (len < PKT_MIN_HEADERLEN)
        return E_NOHEADER;
    if (len > PKT_MAX_LEN)
        return E_LENGTH;

    memcpy(pkt, data, len);

    if (pkt_is_ack_nack(pkt)) {
        if (len > PKT_MIN_HEADERLEN)
            return E_LENGTH;
        memcpy(((char*)pkt) + 3, data + 1, len - 1);
        pkt_set_length(pkt, 0);
    }

    uint16_t payload_len = pkt_get_length(pkt);
    if (payload_len
        && (len > PKT_MAX_HEADERLEN + payload_len + (payload_len ? PKT_FOOTERLEN : 0)
            || payload_len > MAX_PAYLOAD_SIZE))
        return E_LENGTH;

    if (pkt_get_tr(pkt) && pkt_get_type(pkt) != PTYPE_DATA)
        return E_TR;

    ssize_t header_len = predict_header_length(pkt);
    uint32_t crc1 = calc_header_crc(data, header_len - 4);

    if (crc1 != pkt_get_crc1(pkt))
        return E_CRC;

    if (payload_len) {
        memcpy(&pkt->crc2, data + PKT_MAX_HEADERLEN + payload_len, 4);
        uint32_t crc2 = calc_payload_crc(data + header_len, payload_len);

        if (crc2 != pkt_get_crc2(pkt))
            return E_CRC;
    }

    return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char* buf, size_t* len)
{
    uint32_t payload_len = pkt_get_length(pkt);

    ssize_t header_len = predict_header_length(pkt);
    size_t len_to_write = header_len;

    if (!pkt_is_ack_nack(pkt) && payload_len)
        len_to_write += payload_len + PKT_FOOTERLEN;

    if (*len < len_to_write)
        return E_NOMEM;

    memcpy(buf, pkt, len_to_write);

    if (pkt_is_ack_nack(pkt))
        memcpy(buf + 1, ((char*)pkt) + 3, len_to_write - 1);

    calc_and_write_header_crc(buf, header_len - 4);

    if (payload_len && !pkt_is_ack_nack(pkt))
        calc_and_write_payload_crc(buf + header_len, payload_len);

    *len = len_to_write;
    return PKT_OK;
}

ptypes_t pkt_get_type(const pkt_t* pkt)
{
    return (pkt->type_tr_window & 0xC0) >> 6;
}

uint8_t pkt_get_tr(const pkt_t* pkt)
{
    return (pkt->type_tr_window & 0x20) >> 5;
}

uint8_t pkt_get_window(const pkt_t* pkt)
{
    return (pkt->type_tr_window & 0x1f);
}

uint8_t pkt_get_seqnum(const pkt_t* pkt)
{
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
    return ntohs(pkt->length);
}

uint32_t pkt_get_timestamp(const pkt_t* pkt)
{
    return pkt->timestamp;
}

uint32_t pkt_get_crc1(const pkt_t* pkt)
{
    return pkt->crc1;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
    if (pkt->length == 0) {
        return NULL;
    }
    return pkt->payload;
}

uint32_t pkt_get_crc2(const pkt_t* pkt)
{
    if (pkt->length == 0) {
        return 0;
    }
    return pkt->crc2;
}

pkt_status_code pkt_set_type(pkt_t* pkt, const ptypes_t type)
{
    pkt->type_tr_window = (pkt->type_tr_window & 0x3f) | (type << 6);
    return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t* pkt, const uint8_t tr)
{
    if (tr != 0 && tr != 1)
        return E_TR;
    pkt->type_tr_window = (pkt->type_tr_window & 0xdf) | (tr << 5);
    return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t* pkt, const uint8_t window)
{
    if (window > MAX_WINDOW_SIZE)
        return E_WINDOW;
    pkt->type_tr_window = (pkt->type_tr_window & 0xe0) | window;
    return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t* pkt, const uint8_t seqnum)
{
    pkt->seqnum = seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t* pkt, const uint16_t length)
{
    pkt->length = htons(length);
    return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t* pkt, const uint32_t timestamp)
{
    pkt->timestamp = timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t* pkt, const uint32_t crc1)
{
    pkt->crc1 = crc1;
    return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t* pkt, const uint32_t crc2)
{
    pkt->crc2 = crc2;
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t* pkt,
    const char* data,
    const uint16_t length)
{
    pkt_status_code result = pkt_set_length(pkt, length);
    if (result != PKT_OK) {
        return result;
    }

    memcpy(pkt->payload, data, length);

    return PKT_OK;
}

ssize_t predict_header_length(const pkt_t* pkt)
{
    if (pkt_is_ack_nack(pkt))
        return PKT_MIN_HEADERLEN;
    return PKT_MAX_HEADERLEN;
}
