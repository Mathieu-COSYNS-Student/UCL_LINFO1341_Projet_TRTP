#include "fec_tests.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../src/log.h"
#include "../src/packet.h"

bool test_fec_1()
{
    pkt_t* pkt1 = pkt_new();
    pkt_set_type(pkt1, PTYPE_DATA);
    pkt_t* pkt2 = pkt_new();
    pkt_set_type(pkt2, PTYPE_DATA);
    pkt_t* pkt3 = pkt_new();
    pkt_set_type(pkt3, PTYPE_DATA);
    pkt_t* pkt4 = pkt_new();
    pkt_set_type(pkt4, PTYPE_DATA);

    const pkt_t* pkts[4] = { pkt1, pkt2, pkt3, pkt4 };

    pkt_t* pkt_fec = pkt_new_fec(pkts);

    const pkt_t* pkts_2[4] = { pkt1, pkt3, pkt4, pkt_fec };

    pkt_t* pkt2_bis = pkt_from_fec(pkts_2);

    return !memcmp(pkt2, pkt2_bis, sizeof(pkt_t));
}

bool test_fec_2()
{
    pkt_t* pkt1 = pkt_new();
    pkt_set_type(pkt1, PTYPE_DATA);
    pkt_set_length(pkt1, 1);
    pkt_t* pkt2 = pkt_new();
    pkt_set_type(pkt2, PTYPE_DATA);
    pkt_set_length(pkt2, 2);
    pkt_t* pkt3 = pkt_new();
    pkt_set_type(pkt3, PTYPE_DATA);
    pkt_set_length(pkt3, 3);
    pkt_t* pkt4 = pkt_new();
    pkt_set_type(pkt4, PTYPE_DATA);
    pkt_set_length(pkt4, 4);

    const pkt_t* pkts[4] = { pkt1, pkt2, pkt3, pkt4 };

    pkt_t* pkt_fec = pkt_new_fec(pkts);

    const pkt_t* pkts_2[4] = { pkt1, pkt3, pkt4, pkt_fec };

    pkt_t* pkt2_bis = pkt_from_fec(pkts_2);

    return !memcmp(pkt2, pkt2_bis, sizeof(pkt_t));
}

bool test_fec_3()
{
    pkt_t* pkt1 = pkt_new();
    pkt_set_type(pkt1, PTYPE_DATA);
    const char payload1[2] = { '1' };
    pkt_set_payload(pkt1, payload1, 2);

    pkt_t* pkt2 = pkt_new();
    pkt_set_type(pkt2, PTYPE_DATA);
    const char payload2[2] = { '2' };
    pkt_set_payload(pkt2, payload2, 2);

    pkt_t* pkt3 = pkt_new();
    pkt_set_type(pkt3, PTYPE_DATA);
    const char payload3[2] = { '3' };
    pkt_set_payload(pkt3, payload3, 2);

    pkt_t* pkt4 = pkt_new();
    pkt_set_type(pkt4, PTYPE_DATA);
    const char payload4[2] = { '4' };
    pkt_set_payload(pkt4, payload4, 2);

    const pkt_t* pkts[4] = { pkt1, pkt2, pkt3, pkt4 };

    pkt_t* pkt_fec = pkt_new_fec(pkts);

    const pkt_t* pkts_2[4] = { pkt1, pkt3, pkt4, pkt_fec };

    pkt_t* pkt2_bis = pkt_from_fec(pkts_2);

    return !memcmp(pkt2, pkt2_bis, sizeof(pkt_t));
}

bool test_fec_4()
{
    pkt_t* pkt1 = pkt_new();
    pkt_set_type(pkt1, PTYPE_DATA);
    const char payload1[12] = { 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd' };
    pkt_set_payload(pkt1, payload1, 12);

    pkt_t* pkt2 = pkt_new();
    pkt_set_type(pkt2, PTYPE_DATA);
    const char payload2[19] = { 'I', ' ', 'l', 'o', 'v', 'e', ' ', 't', 'h', 'i', 's', ' ', 'c', 'o', 'u', 'r', 's', 'e' };
    pkt_set_payload(pkt2, payload2, 19);

    pkt_t* pkt3 = pkt_new();
    pkt_set_type(pkt3, PTYPE_DATA);
    const char payload3[8] = { 'G', 'o', 'o', 'd', 'b', 'y', 'e' };
    pkt_set_payload(pkt3, payload3, 8);

    pkt_t* pkt4 = pkt_new();
    pkt_set_type(pkt4, PTYPE_DATA);
    const char payload4[21] = { 'Y', 'o', 'u', ' ', 'h', 'a', 't', 'e', ' ', 't', 'h', 'e', ' ', 'p', 'r', 'o', 'j', 'e', 'c', 't' };
    pkt_set_payload(pkt4, payload4, 21);

    const pkt_t* pkts[4] = { pkt1, pkt2, pkt3, pkt4 };

    pkt_t* pkt_fec = pkt_new_fec(pkts);

    const pkt_t* pkts_2[4] = { pkt1, pkt3, pkt4, pkt_fec };

    pkt_t* pkt2_bis = pkt_from_fec(pkts_2);

    return !memcmp(pkt2, pkt2_bis, sizeof(pkt_t));
}

typedef bool (*FecTestCallback)();
FecTestCallback fec_tests_lookup_table[] = {
    &test_fec_1,
    &test_fec_2,
    &test_fec_3,
    &test_fec_4,
    NULL
};

int run_fec_tests()
{
    size_t number_of_tests_failed = 0;
    size_t i = 0;

    while (fec_tests_lookup_table[i]) {
        if (!(fec_tests_lookup_table[i]()))
            number_of_tests_failed++;
        i++;
    }

    char* test_name = "Fec tests";

    if (number_of_tests_failed) {
        TEST_FAILED(test_name, i, number_of_tests_failed);
        return EXIT_FAILURE;
    }

    TEST_SUCCESS(test_name, i);

    return EXIT_SUCCESS;
}