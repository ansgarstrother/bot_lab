#include <stdint.h>

/** reference version, god-awful slow. It should work even on
 * platforms with memory-alignment issues (ARM7TDMI). **/
uint16_t ip_checksum(void *_p, uint32_t len, uint32_t acc)
{
    uint8_t *p = (uint8_t*) _p;

    uint32_t lo = 0, hi = 0;
    while (len >= 2) {
        hi += *p++;
        lo += *p++;
        len -= 2;
    }

    if (len)
        hi += *p++;

    acc = acc + (hi<<8) + lo;
    while (acc >> 16)
        acc = (acc & 0xffff) + (acc >> 16);

    return acc & 0xffff;
}

// broken? (8/18/2009), but only 1% faster than ip_checksum_8 in net
// ping performance with 1024 byte packets (cortex_m3 @ 50mhz)
//
// This implementation relies on unaligned loads working correctly.
// This is true for ARM cortex M3, but untrue for ARM7TDMI!
uint16_t ip_checksum_16(void *_p, uint32_t len, uint32_t acc)
{
    uint8_t *p = (uint8_t*) _p;

#if 0
    while (len >= 16) {
        acc += *((uint16_t*) p);          p+=2;
        acc += *((uint16_t*) p);          p+=2;
        acc += *((uint16_t*) p);          p+=2;
        acc += *((uint16_t*) p);          p+=2;
        acc += *((uint16_t*) p);          p+=2;
        acc += *((uint16_t*) p);          p+=2;
        acc += *((uint16_t*) p);          p+=2;
        acc += *((uint16_t*) p);          p+=2;
        len -= 16;
    }
#endif

    while (len >= 2) {
        acc += *((uint16_t*) p);
        len -= 2;
        p += 2;
    }

    if (len)
        acc += ((*p) << 8);

    while (acc >> 16)
        acc = (acc & 0xffff) + (acc >> 16);

    return acc & 0xffff;
//    return ((acc&0xff)<<8) | (acc>>8);
}

// SEEMS BROKEN -ebolson
//
// This implementation relies on unaligned loads working correctly.
// This is true for ARM cortex M3, but untrue for ARM7TDMI!
uint16_t ip_checksum_32(void *_p, uint32_t len, uint32_t acc)
{
    uint8_t *p = (uint8_t*) _p;

    while (len >= 16) {
        uint32_t tmp = *((uint32_t*) p);
        acc += (tmp&0xffff) + (tmp>>16);
        p+=4;
        tmp = *((uint32_t*) p);
        acc += (tmp&0xffff) + (tmp>>16);
        p+=4;
        tmp = *((uint32_t*) p);
        acc += (tmp&0xffff) + (tmp>>16);
        p+=4;
        tmp = *((uint32_t*) p);
        acc += (tmp&0xffff) + (tmp>>16);
        p+=4;
        len -= 16;
    }

    while (len >= 4) {
        uint32_t tmp = *((uint32_t*) p);
        acc += (tmp&0xffff) + (tmp>>16);

        len -=4;
        p +=4;
    }

    while (len >= 2) {
        acc += *((uint16_t*) p);
        len -= 2;
        p += 2;
    }

    if (len)
        acc += *p;

    while (acc >> 16)
        acc = (acc & 0xffff) + (acc >> 16);

    return ((acc&0xff)<<8) | (acc>>8);
}

#if 0

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

int64_t timestamp_now()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char *argv[])
{
    int MAX_LENGTH = 2048;
    uint8_t buffer[MAX_LENGTH];

    srand(0);

    int64_t c0usecs = 0, c1usecs = 0;
    for (int iters = 0; iters < 100000; iters++) {
        for (int i = 0; i < MAX_LENGTH; i++)
            buffer[i] = rand();

        int offset = rand()%16;
        int len = rand()%(MAX_LENGTH - offset);

        int64_t  t0 = timestamp_now();
        uint16_t c0 = ip_checksum(&buffer[offset], len, 0);
        int64_t  t1 = timestamp_now();
        uint16_t c1 = ip_checksum4(&buffer[offset], len, 0);
        int64_t  t2 = timestamp_now();

        c0usecs += (t1-t0);
        c1usecs += (t2-t1);

        if (c0 != c1)
            printf("fail: %04x %04x\n", c0, c1);
    }

    printf("time: c0: 1.0  c1: %f\n", ((double) c1usecs) / c0usecs);
    
    return 1;
}

#endif
