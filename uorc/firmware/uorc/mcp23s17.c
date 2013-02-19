#include "mcp23s17.h"
#include "lm3s8962.h"

#define MCP_HZ 1000000

// how long a wait to enforce between transactions. Empirically,
// values near 10 are NOT long enough.
#define MCP_SPACE_US 50

uint32_t mcp23s17_read(uint32_t addr)
{
    ssi_lock();

    ssi_config(MCP_HZ, 0, 0, 8);

    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 5, 0); // assert gpio SS

    uint32_t tx[3] = {0x41, addr, 0};
    uint32_t rx[3];

    ssi_rxtx(tx, rx, 3);
    uint32_t value = rx[2];

    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 5, 1); // deassert gpio SS

    nkern_usleep(MCP_SPACE_US);
    ssi_unlock();
    return value;
}

void mcp23s17_write(uint32_t addr, uint32_t value)
{
    ssi_lock();
    ssi_config(MCP_HZ, 0, 0, 8);

    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 5, 0); // assert gpio SS

    uint32_t tx[3] = {0x40, addr, value};
    uint32_t rx[3];

    // request.
    ssi_rxtx(tx, rx, 3);

    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 5, 1); // deassert gpio SS
    nkern_usleep(MCP_SPACE_US);
    ssi_unlock();
}

void mcp23s17_write_until_verified(uint32_t addr, uint32_t value)
{
    while (1) {
        mcp23s17_write(addr, value);
        uint32_t readval = mcp23s17_read(addr);
        if (readval == value)
            return;

        nkern_usleep(2000);
    }
}

