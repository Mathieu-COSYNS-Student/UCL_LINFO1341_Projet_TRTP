#include <stdbool.h>
#include <string.h>

#include "../src/log.h"
#include "../src/packet.h"
#include "../src/xxd.h"

#ifdef _DEBUG
#define TEST_FAILED(number_of_tests_failed) \
    ERROR("%d test(s) failed.", number_of_tests_failed);
#else
#define TEST_FAILED(number_of_tests_failed) \
    ERROR("%d test(s) failed. Try -D_DEBUG flag for more info", number_of_tests_failed);
#endif

#define TOTAL_PKT_TESTED 3

pkt_t* create_test_pkt_1()
{
    pkt_t* pkt = pkt_new();

    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, 0x1C);
    pkt_set_seqnum(pkt, 0x7b);
    pkt_set_timestamp(pkt, 0x17);

    char payload[11] = { 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd' };
    pkt_set_payload(pkt, payload, 11);

    return pkt;
}

pkt_t* create_test_pkt_2()
{
    pkt_t* pkt = pkt_new();

    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 1);
    pkt_set_window(pkt, 0x1C);
    pkt_set_seqnum(pkt, 0x7b);
    pkt_set_timestamp(pkt, 0x17);
    pkt_set_length(pkt, 0);

    return pkt;
}

pkt_t* create_test_pkt_3()
{
    pkt_t* pkt = pkt_new();

    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 1);
    pkt_set_window(pkt, 0x1C);
    pkt_set_seqnum(pkt, 0x7b);
    pkt_set_timestamp(pkt, 0x17);

    return pkt;
}

pkt_t* create_test_pkt(int pkt_number)
{
    switch (pkt_number) {
    case 1:
        return create_test_pkt_1();
    case 2:
        return create_test_pkt_2();
    case 3:
        return create_test_pkt_3();
    default:
        return NULL;
    }
}

bool assert_encode_decode_consistency(int pkt_number)
{
    bool success = true;
    char buf[528];
    size_t buf_len = 528;

    pkt_t* pkt = create_test_pkt(pkt_number);
    pkt_encode(pkt, buf, &buf_len);

    pkt_t* new_pkt = pkt_new();
    pkt_decode(buf, buf_len, new_pkt);
    pkt_set_crc1(new_pkt, pkt_get_crc1(pkt));
    pkt_set_crc2(new_pkt, pkt_get_crc2(pkt));

    if (memcmp(pkt, new_pkt, sizeof(*pkt))) {
        ERROR("Encoded and decoded packet differ from the original");
        DEBUG("Original packet (pkt_number=%d)", pkt_number);
        DEBUG_DUMP(pkt, sizeof(*pkt));
        DEBUG("Encoded packet");
        DEBUG_DUMP(buf, buf_len);
        DEBUG("Decoded packet");
        DEBUG_DUMP(new_pkt, sizeof(*new_pkt));
        success = false;
    }

    pkt_del(pkt);
    pkt_del(new_pkt);

    return success;
}
int main()
{
    int tests_failed = 0;
    for (size_t i = 0; i < TOTAL_PKT_TESTED; i++) {
        if (!assert_encode_decode_consistency(i + 1))
            tests_failed++;
    }

    if (tests_failed) {
        TEST_FAILED(tests_failed);
    }

    SUCCESS("%d test(s) out of %d were successful.", TOTAL_PKT_TESTED - tests_failed, TOTAL_PKT_TESTED);

    return 0;
}