#include "packet.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

int pkt_is_ack_nack(const pkt_t* pkt)
{
    uint8_t type = pkt_get_type(pkt);
    return type == PTYPE_ACK || type == PTYPE_NACK;
}

uint32_t calc_header_crc(const char* buf, ssize_t len)
{
    char bufcpy[PKT_MAX_HEADERLEN - sizeof(uint32_t)];
    memcpy(bufcpy, buf, len);

    bufcpy[0] = buf[0] & 0xdf;
    uLong crc = crc32(0L, (Bytef*)bufcpy, len);

    return crc;
}

uint32_t calc_payload_crc(const char* buf, ssize_t len)
{
    return crc32(0L, (Bytef*)buf, len);
}

void calc_and_write_header_crc(char* buf, ssize_t len)
{
    uint32_t crc = htonl(calc_header_crc(buf, len));
    memcpy(buf + len, &crc, sizeof(uint32_t));
}

void calc_and_write_payload_crc(char* buf, ssize_t len)
{
    uint32_t crc = htonl(calc_payload_crc(buf, len));
    memcpy(buf + len, &crc, sizeof(uint32_t));
}

pkt_t* pkt_new()
{
    void* pkt = calloc(sizeof(pkt_t), 1);
    return (pkt_t*)pkt;
}

void pkt_del(pkt_t* pkt)
{
    if (!pkt)
        return;
    free(pkt);
}

pkt_t* pkt_copy(pkt_t* pkt)
{
    pkt_t* new_pkt = pkt_new();
    if (!new_pkt)
        return NULL;

    memcpy(new_pkt, pkt, sizeof(pkt_t));

    return new_pkt;
}

ssize_t predict_header_length(const pkt_t* pkt)
{
    if (pkt_is_ack_nack(pkt))
        return PKT_MIN_HEADERLEN;
    return PKT_MAX_HEADERLEN;
}

pkt_status_code pkt_decode(const char* data, const size_t len, pkt_t* pkt)
{
    if (len < PKT_MIN_HEADERLEN)
        return E_NOHEADER;
    if (len > PKT_MAX_LEN)
        return E_LENGTH;

    memcpy(pkt, data, len);

    if (pkt_get_tr(pkt) && pkt_get_type(pkt) != PTYPE_DATA)
        return E_TR;

    if (pkt_is_ack_nack(pkt)) {
        if (len != PKT_MIN_HEADERLEN)
            return E_LENGTH;
        memcpy(((char*)pkt) + 3, data + 1, len - 1);
        pkt_set_length(pkt, 0);
    }

    uint16_t payload_len = pkt_get_length(pkt);
    if ((pkt_has_payload(pkt)
            && (len > PKT_MAX_HEADERLEN + payload_len + (payload_len ? PKT_FOOTERLEN : 0)
                || payload_len > MAX_PAYLOAD_SIZE))
        || (pkt_get_type(pkt) == PTYPE_FEC && payload_len != MAX_PAYLOAD_SIZE))
        return E_LENGTH;

    ssize_t header_len = predict_header_length(pkt);
    uint32_t crc1 = calc_header_crc(data, header_len - sizeof(uint32_t));

    if (crc1 != pkt_get_crc1(pkt))
        return E_CRC;

    if (pkt_has_payload(pkt)) {
        uint32_t crc2 = 0;
        memcpy(&crc2, data + PKT_MAX_HEADERLEN + payload_len, sizeof(uint32_t));
        memset((char*)&pkt->payload + payload_len, 0, sizeof(uint32_t));
        memcpy(&pkt->crc2, &crc2, sizeof(uint32_t));
        crc2 = calc_payload_crc(data + header_len, payload_len);

        if (crc2 != pkt_get_crc2(pkt))
            return E_CRC;
    }

    return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char* buf, size_t* len)
{
    uint32_t payload_len = pkt_get_length(pkt);

    if (payload_len > MAX_PAYLOAD_SIZE) {
        return E_LENGTH;
    }

    ssize_t header_len = predict_header_length(pkt);
    size_t len_to_write = header_len;

    if (pkt_has_payload(pkt))
        len_to_write += payload_len + PKT_FOOTERLEN;

    if (*len < len_to_write)
        return E_NOMEM;

    memcpy(buf, pkt, len_to_write);

    if (pkt_is_ack_nack(pkt))
        memcpy(buf + 1, ((char*)pkt) + 3, len_to_write - 1);

    calc_and_write_header_crc(buf, header_len - 4);

    if (pkt_has_payload(pkt))
        calc_and_write_payload_crc(buf + header_len, payload_len);

    *len = len_to_write;
    return PKT_OK;
}

bool pkt_has_payload(const pkt_t* pkt)
{
    return !pkt_get_tr(pkt) && !pkt_is_ack_nack(pkt) && pkt_get_payload(pkt);
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
    return ntohl(pkt->crc1);
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
    return ntohl(pkt->crc2);
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
    pkt->crc1 = htonl(crc1);
    return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t* pkt, const uint32_t crc2)
{
    pkt->crc2 = htonl(crc2);
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t* pkt,
    const char* data,
    const uint16_t length)
{
    if (!data && length) {
        return E_LENGTH;
    }

    pkt_status_code result = pkt_set_length(pkt, length);
    if (result != PKT_OK) {
        return result;
    }

    memcpy(pkt->payload, data, length);

    return PKT_OK;
}

pkt_t* pkt_new_fec(const pkt_t* pkts[4])
{
    pkt_t* pkt_fec = pkt_new();
    if (!pkt_fec)
        return NULL;

    char fec_payload[MAX_PAYLOAD_SIZE] = { 0 };
    uint16_t fec_length = 0;

    for (size_t i = 0; i < 4; i++) {
        uint16_t current_pkt_length = pkt_get_length(pkts[i]);
        char* current_pkt_payload = pkt_get_payload(pkts[i]);

        fec_length ^= current_pkt_length;
        for (size_t i = 0; i < MAX_PAYLOAD_SIZE; i++) {
            if (i < current_pkt_length)
                fec_payload[i] ^= current_pkt_payload[i];
            else
                fec_payload[i] ^= 0;
        }
    }

    pkt_set_length(pkt_fec, fec_length);
    pkt_set_payload(pkt_fec, fec_payload, MAX_PAYLOAD_SIZE);

    return pkt_fec;
}

pkt_t* pkt_from_fec(const pkt_t* pkt_fec, const pkt_t* pkts[3])
{
}