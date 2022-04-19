#include "packet_tests.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../src/log.h"
#include "../src/packet.h"
#include "../src/xxd.h"

#define DUMP_PKT_ENCODED_AND_DECODED(pkt_assertion, msg, ...)                              \
    ERROR("Error in assert_encode_decode with pkt_assertion->id = %d", pkt_assertion->id); \
    ERROR(msg, ##__VA_ARGS__);                                                             \
    DEBUG("Original packet:");                                                             \
    DEBUG_DUMP(pkt_assertion->pkt, sizeof(pkt_t));                                         \
    DEBUG("Encoded packet:");                                                              \
    DEBUG_DUMP(pkt_assertion->buffer, pkt_assertion->buffer_len);                          \
    if (new_pkt) {                                                                         \
        DEBUG("Decoded packet:");                                                          \
        DEBUG_DUMP(pkt_assertion->new_pkt, sizeof(pkt_t));                                 \
    }

typedef void (*alter_buffer_fn)(char*, size_t);
typedef struct {
    int id;
    pkt_t* pkt;
    char* buffer;
    size_t buffer_len;
    pkt_t* new_pkt;
    size_t expected_encoded_len;
    pkt_status_code expected_encode_status;
    pkt_status_code expected_decode_status;
    alter_buffer_fn alter_buffer_fn;
} pkt_assertion_t;

pkt_assertion_t* pkt_assertion_new()
{
    size_t buffer_len = PKT_MAX_HEADERLEN + MAX_PAYLOAD_SIZE + PKT_FOOTERLEN;

    pkt_assertion_t* pkt_assertion = (pkt_assertion_t*)malloc(sizeof(pkt_assertion_t));
    char* buffer = (char*)malloc(buffer_len * sizeof(char));
    pkt_t* pkt = pkt_new();
    pkt_t* new_pkt = pkt_new();

    pkt_assertion->pkt = pkt;
    pkt_assertion->buffer = buffer;
    pkt_assertion->buffer_len = buffer_len;
    pkt_assertion->new_pkt = new_pkt;
    pkt_assertion->expected_encode_status = PKT_OK;
    pkt_assertion->expected_decode_status = PKT_OK;
    pkt_assertion->alter_buffer_fn = NULL;

    return pkt_assertion;
}

void pkt_assertion_resize_buffer(pkt_assertion_t* pkt_assertion, size_t buffer_len)
{
    pkt_assertion->buffer = realloc(pkt_assertion->buffer, buffer_len);
    pkt_assertion->buffer_len = buffer_len;
}

void pkt_assertion_del(pkt_assertion_t* pkt_assertion)
{
    pkt_del(pkt_assertion->pkt);
    free(pkt_assertion->buffer);
    pkt_del(pkt_assertion->new_pkt);
    free(pkt_assertion);
}

void set_pkt_assertion_1(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_tr(pkt_assertion->pkt, 0);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);

    char payload[11] = { 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd' };
    pkt_set_payload(pkt_assertion->pkt, payload, 11);

    pkt_assertion->expected_encoded_len = PKT_MAX_HEADERLEN + 11 + PKT_FOOTERLEN;
}

void set_pkt_assertion_2(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_tr(pkt_assertion->pkt, 1);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x0);
    pkt_set_length(pkt_assertion->pkt, 0);

    pkt_assertion->expected_encoded_len = PKT_MAX_HEADERLEN;
}

void set_pkt_assertion_3(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_tr(pkt_assertion->pkt, 1);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);

    pkt_assertion->expected_encoded_len = PKT_MAX_HEADERLEN;
}

void set_pkt_assertion_4(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_ACK);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);

    pkt_assertion->expected_encoded_len = PKT_MIN_HEADERLEN;
}

void set_pkt_assertion_5(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_ACK);
    pkt_set_tr(pkt_assertion->pkt, 1);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);

    pkt_assertion->expected_encoded_len = PKT_MIN_HEADERLEN;
    pkt_assertion->expected_decode_status = E_TR;
}

void set_pkt_assertion_6(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_NACK);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);

    pkt_assertion->expected_encoded_len = PKT_MIN_HEADERLEN;
}

void set_pkt_assertion_7(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_NACK);
    pkt_set_tr(pkt_assertion->pkt, 1);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);

    pkt_assertion->expected_encoded_len = PKT_MIN_HEADERLEN;
    pkt_assertion->expected_decode_status = E_TR;
}

void set_pkt_assertion_8(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);

    char payload[11] = { 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd' };
    pkt_set_payload(pkt_assertion->pkt, payload, 11);

    pkt_assertion_resize_buffer(pkt_assertion, PKT_MAX_HEADERLEN + 10);
    pkt_assertion->expected_encode_status = E_NOMEM;
}

void set_pkt_assertion_9(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);
    pkt_set_length(pkt_assertion->pkt, 525);

    pkt_assertion->expected_encode_status = E_LENGTH;
}

void set_pkt_assertion_10(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_FEC);
    pkt_set_tr(pkt_assertion->pkt, 1);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);
    pkt_set_length(pkt_assertion->pkt, MAX_PAYLOAD_SIZE);

    pkt_assertion->expected_encoded_len = PKT_MAX_HEADERLEN;
    pkt_assertion->expected_decode_status = E_TR;
}

void change_len_in_buffer(char* buffer, size_t buffer_len)
{
    if (buffer_len < sizeof(char) + sizeof(uint16_t))
        return;
    uint16_t new_length = 525;
    memcpy(buffer + sizeof(char), &new_length, sizeof(uint16_t));
}

void set_pkt_assertion_11(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);

    pkt_assertion->expected_encoded_len = PKT_MAX_HEADERLEN;
    pkt_assertion->expected_decode_status = E_LENGTH;
    pkt_assertion->alter_buffer_fn = change_len_in_buffer;
}

void set_pkt_assertion_12(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_FEC);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);
    char payload[11] = { 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd' };
    pkt_set_payload(pkt_assertion->pkt, payload, 11);
    pkt_set_length(pkt_assertion->pkt, MAX_PAYLOAD_SIZE);

    pkt_assertion->expected_encoded_len = PKT_MAX_LEN;
}

void set_pkt_assertion_13(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_FEC);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);
    pkt_set_length(pkt_assertion->pkt, 256);

    pkt_assertion->expected_encoded_len = PKT_MAX_HEADERLEN + MAX_PAYLOAD_SIZE + PKT_FOOTERLEN;
}

void change_crc1_in_buffer_ptype_data(char* buffer, size_t buffer_len)
{
    if (buffer_len < PKT_MAX_HEADERLEN)
        return;
    uint32_t new_crc = 0x11111111;
    memcpy(buffer + PKT_MAX_HEADERLEN - sizeof(uint32_t), &new_crc, sizeof(uint32_t));
}

void set_pkt_assertion_14(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);

    pkt_assertion->expected_encoded_len = PKT_MAX_HEADERLEN;
    pkt_assertion->expected_decode_status = E_CRC;
    pkt_assertion->alter_buffer_fn = change_crc1_in_buffer_ptype_data;
}

void change_crc1_in_buffer_ptype_ack(char* buffer, size_t buffer_len)
{
    if (buffer_len < PKT_MIN_HEADERLEN)
        return;
    uint32_t new_crc = 0x11111111;
    memcpy(buffer + PKT_MIN_HEADERLEN - sizeof(uint32_t), &new_crc, sizeof(uint32_t));
}

void set_pkt_assertion_15(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_ACK);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);

    pkt_assertion->expected_encoded_len = PKT_MIN_HEADERLEN;
    pkt_assertion->expected_decode_status = E_CRC;
    pkt_assertion->alter_buffer_fn = change_crc1_in_buffer_ptype_ack;
}

void set_pkt_assertion_16(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);
    pkt_set_length(pkt_assertion->pkt, 510);

    pkt_assertion->expected_encoded_len = PKT_MAX_HEADERLEN + 510 + PKT_FOOTERLEN;
}

void change_crc2_in_buffer_ptype_data(char* buffer, size_t buffer_len)
{
    uint32_t new_crc = 0x11111111;
    memcpy(buffer + buffer_len - sizeof(uint32_t), &new_crc, sizeof(uint32_t));
}

void set_pkt_assertion_17(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);
    pkt_set_length(pkt_assertion->pkt, 256);

    pkt_assertion->expected_encoded_len = PKT_MAX_HEADERLEN + 256 + PKT_FOOTERLEN;
    pkt_assertion->expected_decode_status = E_CRC;
    pkt_assertion->alter_buffer_fn = change_crc2_in_buffer_ptype_data;
}

void set_pkt_assertion_18(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);
    pkt_set_length(pkt_assertion->pkt, 510);

    pkt_assertion->expected_encoded_len = PKT_MAX_HEADERLEN + 510 + PKT_FOOTERLEN;
    pkt_assertion->expected_decode_status = E_CRC;
    pkt_assertion->alter_buffer_fn = change_crc2_in_buffer_ptype_data;
}

void set_pkt_assertion_19(pkt_assertion_t* pkt_assertion)
{
    pkt_set_type(pkt_assertion->pkt, PTYPE_DATA);
    pkt_set_window(pkt_assertion->pkt, 0x1C);
    pkt_set_seqnum(pkt_assertion->pkt, 0x7b);
    pkt_set_timestamp(pkt_assertion->pkt, 0x17);
    pkt_set_length(pkt_assertion->pkt, MAX_PAYLOAD_SIZE);

    pkt_assertion->expected_encoded_len = PKT_MAX_LEN;
    pkt_assertion->expected_decode_status = E_CRC;
    pkt_assertion->alter_buffer_fn = change_crc2_in_buffer_ptype_data;
}

typedef void (*SetAssertionCallback)(pkt_assertion_t*);
SetAssertionCallback set_assertion_lookup_table[] = {
    &set_pkt_assertion_1,
    &set_pkt_assertion_2,
    &set_pkt_assertion_3,
    &set_pkt_assertion_4,
    &set_pkt_assertion_5,
    &set_pkt_assertion_6,
    &set_pkt_assertion_7,
    &set_pkt_assertion_8,
    &set_pkt_assertion_9,
    &set_pkt_assertion_10,
    &set_pkt_assertion_11,
    &set_pkt_assertion_12,
    &set_pkt_assertion_13,
    &set_pkt_assertion_14,
    &set_pkt_assertion_15,
    &set_pkt_assertion_16,
    &set_pkt_assertion_17,
    &set_pkt_assertion_18,
    &set_pkt_assertion_19,
    NULL
};

bool assert_encode_decode(pkt_assertion_t* pkt_assertion)
{
    pkt_t* pkt = pkt_assertion->pkt;
    char* buffer = pkt_assertion->buffer;
    pkt_t* new_pkt = pkt_assertion->new_pkt;
    pkt_status_code expected_encode_status = pkt_assertion->expected_encode_status;
    pkt_status_code expected_decode_status = pkt_assertion->expected_decode_status;

    pkt_status_code encode_status = pkt_encode(pkt, buffer, &pkt_assertion->buffer_len);

    if (encode_status != expected_encode_status) {
        DUMP_PKT_ENCODED_AND_DECODED(pkt_assertion,
            "Packet encoding failed with status %d. Expected status %d.",
            encode_status, expected_encode_status);
        return false;
    }

    if (pkt_assertion->expected_encode_status != PKT_OK)
        return true;

    if (pkt_assertion->expected_encoded_len != pkt_assertion->buffer_len) {
        DUMP_PKT_ENCODED_AND_DECODED(pkt_assertion,
            "Packet encoding failed. Packet length=%ld, %ld expected.",
            pkt_assertion->buffer_len, pkt_assertion->expected_encoded_len);
        return false;
    }

    if (pkt_assertion->alter_buffer_fn)
        pkt_assertion->alter_buffer_fn(buffer, pkt_assertion->buffer_len);

    pkt_status_code decode_status = pkt_decode(buffer, pkt_assertion->buffer_len, new_pkt);

    if (decode_status != pkt_assertion->expected_decode_status) {
        DUMP_PKT_ENCODED_AND_DECODED(pkt_assertion,
            "Packet decoding failed with status %d. Expected status %d.",
            decode_status, expected_decode_status);
        return false;
    }

    if (pkt_assertion->expected_decode_status != PKT_OK)
        return true;

    pkt_set_crc1(new_pkt, pkt_get_crc1(pkt));
    pkt_set_crc2(new_pkt, pkt_get_crc2(pkt));

    if (memcmp(pkt, new_pkt, sizeof(*pkt))) {
        DUMP_PKT_ENCODED_AND_DECODED(pkt_assertion,
            "Encoded and decoded packet differ from the original.");
        return false;
    }

    return true;
}

int run_packet_tests()
{
    size_t number_of_tests_failed = 0;
    size_t i = 0;

    while (set_assertion_lookup_table[i]) {
        pkt_assertion_t* pkt_assertion = pkt_assertion_new();
        pkt_assertion->id = i + 1;
        set_assertion_lookup_table[i](pkt_assertion);

        if (!assert_encode_decode(pkt_assertion))
            number_of_tests_failed++;

        pkt_assertion_del(pkt_assertion);
        i++;
    }

    char* test_name = "Packet tests";

    if (number_of_tests_failed) {
        TEST_FAILED(test_name, i, number_of_tests_failed);
        return EXIT_FAILURE;
    }

    TEST_SUCCESS(test_name, i);

    return EXIT_SUCCESS;
}