#include <stdint.h>
#include "lm3s8962.h"

/** addr1k is a 1kb-aligned address. **/
void flash_erase(uint32_t addr1k)
{
    FLASH_FMA_R = addr1k;
    
    FLASH_FMC_R = 0xA4420002; // magic key + erase

    while (FLASH_FMC_R & FLASH_FMC_ERASE);
}

/** Assume addr is 4-byte aligned. **/
void flash_write(uint32_t addr, uint32_t value)
{
    if (*((uint32_t*)addr)==value)
        return;

    FLASH_FMD_R = value;
    FLASH_FMA_R = addr;

    FLASH_FMC_R = 0xA4420001; // magic key + write byte

    while (FLASH_FMC_R & FLASH_FMC_WRITE);
}

/** Assume addr is 4-byte aligned. **/
void flash_erase_and_write(void *_addr, void *_buf, uint32_t _len)
{
    if (_len == 0)
        return;

    uint32_t addr = (uint32_t) _addr;
    uint32_t *buf = (uint32_t*) _buf;
    uint32_t len  = (_len+3)/4;          // length in words

    int needwrite = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (((uint32_t*)addr)[i] != buf[i]) {
            needwrite = 1;
            break;
        }
    }

    if (!needwrite)
        return;

    // erase the appropriate 1KB pages
    for (uint32_t addr1k = addr & 0xfffffc00; addr1k < addr + len*4; addr1k += 1024) {
        flash_erase(addr1k);
    }
    
    // write the data.
    for (uint32_t pos = 0; pos < len; pos++) {
        flash_write(addr + pos*4, buf[pos]);
    }
}
