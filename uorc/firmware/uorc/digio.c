#include <nkern.h>
#include "lm3s8962.h"

#include "mcp23s17.h"

uint32_t mcp23s17_gpio_data;  // (GPIOA<<8) | (GPIOB.  (GPIOB are the numbered ports).
uint32_t mcp23s17_gpio_config_b; // (GPIOB_DIR) | (GPIOB_GPPU<<8)

static int estop_state;               // if 1, we're in ESTOP.

void digio_task(void *arg)
{
    // XXX Make this interrupt driven?
    while (1) {
        uint32_t gpioa = mcp23s17_read(MCP23S17_GPIOA);
        uint32_t gpiob = mcp23s17_read(MCP23S17_GPIOB);

        mcp23s17_gpio_data = ((gpioa&0xff)<<8) | ((gpiob&0xff));

        // XXX assumes latching (not momentary) ESTOP.

        if (0) {
            // normally open
            estop_state = (gpioa&(1<<6)) ? 0 : 1;
        } else {
            // normally closed
            estop_state = (gpioa&(1<<6)) ? 1 : 0;
        }

        uint32_t button0 = (gpioa&(1<<7)) ? 0 : 1;
        uint32_t motfault0 = (gpioa&(1<<0)) ? 0 : 1;
        uint32_t motfault1 = (gpioa&(1<<2)) ? 0 : 1;
        uint32_t motfault2 = (gpioa&(1<<4)) ? 0 : 1;

        nkern_usleep(25000);
    }
}

int digio_get()
{
    return mcp23s17_gpio_data;
}

int digio_get_estop()
{
    return estop_state;
}

uint32_t digio_get_config()
{
    return mcp23s17_gpio_config_b;
}

void digio_configure(int port, int iodir, int pullup)
{
    uint8_t old_iodirb = mcp23s17_gpio_config_b & 0xff;
    uint8_t old_gppub = (mcp23s17_gpio_config_b>>8)&0xff;

    uint8_t new_iodirb = (old_iodirb & (~(1<<port))) | (iodir<<port);
    uint8_t new_gppub = (old_gppub & (~(1<<port))) | (pullup<<port);

    if (new_iodirb != old_gppub) {
        mcp23s17_write(MCP23S17_IODIRB, new_iodirb);
    }

    if (new_gppub != old_gppub) {
        mcp23s17_write(MCP23S17_GPPUB, new_gppub);
    }

    mcp23s17_gpio_config_b = new_iodirb | (new_gppub << 8);
}

void digio_set(int port, int value)
{
    uint8_t old_datab = mcp23s17_gpio_data&0xff;
    uint8_t new_datab = (old_datab & (~(1<<port))) | (value << port);

    if (new_datab != old_datab)
        mcp23s17_write(MCP23S17_GPIOB, new_datab);
}

void digio_init()
{
    nkern_usleep(20000); // wait for POR

    // The POR is not terribly reliable. Thus, let's reinitialize
    // every register with its POR default.  (DO NO CUSTOMIZATION
    // HERE; this is reusable code!)
    mcp23s17_write_until_verified(MCP23S17_IODIRA, 0xff);
    mcp23s17_write_until_verified(MCP23S17_IODIRB, 0xff);
    mcp23s17_write_until_verified(MCP23S17_IPOLA,  0x00);
    mcp23s17_write_until_verified(MCP23S17_IPOLB,  0x00);
    mcp23s17_write_until_verified(MCP23S17_GPINTENA, 0x00);
    mcp23s17_write_until_verified(MCP23S17_GPINTENB, 0x00);
    mcp23s17_write_until_verified(MCP23S17_GPPUA, 0x00);
    mcp23s17_write_until_verified(MCP23S17_GPPUB, 0x00);
    mcp23s17_write(MCP23S17_GPIOA, 0x00);
    mcp23s17_write(MCP23S17_GPIOB, 0x00);
    mcp23s17_write_until_verified(MCP23S17_OLATA, 0x00);
    mcp23s17_write_until_verified(MCP23S17_OLATB, 0x00);

    // Configure GPIO expander chip
    // set direction (1=input)
    mcp23s17_write_until_verified(MCP23S17_IODIRA, (1<<0) | (1<<2) | (1<<4) | (1<<6) | (1<<7));
    mcp23s17_write_until_verified(MCP23S17_GPPUA, (1<<0) | (1<<2) | (1<<4)); // enable FAULT pullups

    // set all 'dumb' digital IO pins to inputs
    mcp23s17_write_until_verified(MCP23S17_IODIRB, 0xff);
    mcp23s17_write_until_verified(MCP23S17_GPPUA, 0xff);
    mcp23s17_gpio_config_b = 0xffff;

    nkern_task_create("digio",
                      digio_task, NULL,
                      NKERN_PRIORITY_NORMAL, 512);

}
