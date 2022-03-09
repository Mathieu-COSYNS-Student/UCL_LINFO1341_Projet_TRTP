#include <string.h>

#include "../src/packet.h"
#include "../src/xxd.h"
#include "../src/log.h"

int main()
{
    char buf[528];
    size_t len = 528;
    pkt_t* pkt = pkt_new();

    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, 0x1C);
    pkt_set_seqnum(pkt, 0x7b);
    pkt_set_timestamp(pkt, 0x17);

    char payload[11] = { 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd' };
    pkt_set_payload(pkt, payload, 11);

    pkt_encode(pkt, buf, &len);

    // hexDump(0, buf, len);

    pkt_t* new_pkt = pkt_new();
    pkt_decode(buf, len, new_pkt);

    ASSERT(memcmp(pkt, new_pkt, sizeof(*pkt)));

    pkt_del(pkt);
    return 0;
}
