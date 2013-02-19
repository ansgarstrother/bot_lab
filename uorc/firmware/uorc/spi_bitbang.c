void ssi_sck(uint32_t v)
{
    gpio_set_bit(GPIO_PORTA_DATA_BITS_R, 2, v);
}

void ssi_mosi(uint32_t v)
{
    gpio_set_bit(GPIO_PORTA_DATA_BITS_R, 5, v);
}

uint32_t ssi_miso()
{
    return (GPIO_PORTA_DATA_R >> 4)&1;
}

void sdelay()
{
    for (int i = 0; i < 100; i++)
        nop();
}

uint32_t ssi_cmd(uint32_t v, uint32_t nbits)
{
    // assume caller has dropped \CS
    // assume sck is low. (that's how we leave it)

    uint32_t response = 0;

    // ssi_sck(0);
    for (int i = nbits-1; i>=0; i--) {
        ssi_sck(1);
        ssi_mosi((v >> i) & 1);
        sdelay();
        ssi_sck(0);
        sdelay();
        response |= (ssi_miso()<<i);
    }

    // assume caller will raise \CS
    return response;
}
