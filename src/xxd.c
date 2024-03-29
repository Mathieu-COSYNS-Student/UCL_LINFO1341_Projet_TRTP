#include "xxd.h"

#include <stdio.h>
#include <string.h>

void hexDump(size_t offset, void* addr, int len)
{
    int i;
    unsigned char bufferLine[17];
    unsigned char* pc = (unsigned char*)addr;

    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0)
                fprintf(stderr, " %s\n", bufferLine);
            // Bogus test for zero bytes!
            // if (pc[i] == 0x00)
            //    exit(0);
            fprintf(stderr, "%08zx: ", offset);
            offset += (i % 16 == 0) ? 16 : i % 16;
        }

        fprintf(stderr, "%02x", pc[i]);
        if ((i % 2) == 1)
            fprintf(stderr, " ");

        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
            bufferLine[i % 16] = '.';
        } else {
            bufferLine[i % 16] = pc[i];
        }

        bufferLine[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0) {
        fprintf(stderr, "  ");
        if (i % 2 == 1)
            putchar(' ');
        i++;
    }
    fprintf(stderr, " %s\n", bufferLine);
}