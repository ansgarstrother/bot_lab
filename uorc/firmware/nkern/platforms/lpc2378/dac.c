#include <lpc23xx.h>

void dac_init()
{
    // no PCONP bit?
//    PCONP |= (1<<xxx);

    PINSEL1 =( PINSEL1&(~(3<<20))) | (2<<20);
}

// 16 bit value 
void dac_write(int val)
{
    int bias = 0;
    DACR = (bias << 16) | (val & 0xffc0);
}
