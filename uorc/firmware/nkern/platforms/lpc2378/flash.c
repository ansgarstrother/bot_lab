#include <nkern.h>
#include "flash.h"

typedef void (*IAP)(uint32_t[], uint32_t[]);
const IAP iap_entry = (IAP) (0x7ffffff1);

static int flash_prepare(int flash_sector)
{
    uint32_t prepare_req[] = { 50, flash_sector, flash_sector };
    uint32_t prepare_ret[] = { 0 };
    iap_entry(prepare_req, prepare_ret);
    if (prepare_ret[0] != 0)
        return -1;
    return 0;
}

// DISABLE INTERRUPTS!
int flash_erase(int cpu_khz, int flash_sector)
{
    if (flash_prepare(flash_sector))
        return -1;

    uint32_t erase_req[] = { 52, flash_sector, flash_sector, cpu_khz };
    uint32_t erase_ret[] = { 0 };
    iap_entry(erase_req, erase_ret);
    if (erase_ret[0] != 0)
        return -2;

    return 0;
}

// datalen is always rounded up to the next 256 byte boundary 
// DISABLE INTERRUPTS!
int flash_write(int cpu_khz, int flash_sector, void *_flash_addr, void *_ram_addr, uint32_t datalen)
{
    uint8_t *flash_addr = (uint8_t*) _flash_addr;
    uint8_t *ram_addr = (uint8_t*) _ram_addr;
    uint32_t write_size = 256;

    for (uint32_t offset = 0; offset < datalen; offset+=write_size) {

        if (flash_prepare(flash_sector))
            return -1;

        uint32_t write_req[] = { 51, 
                                 (uint32_t) &flash_addr[offset], 
                                 (uint32_t) &ram_addr[offset],
                                 write_size, cpu_khz };
        uint32_t write_ret[] = { 0 };
        iap_entry(write_req, write_ret);
        if (write_ret[0] != 0)
            return -5;
    }

    return 0;
}
