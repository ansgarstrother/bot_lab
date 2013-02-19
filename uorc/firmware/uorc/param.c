#include "param.h"
#include "flash.h"

void serial0_putc(char c);

void param_init()
{
    if (flash_params->start_signature != FLASH_PARAM_SIGNATURE ||
        flash_params->end_signature   != FLASH_PARAM_SIGNATURE ||
        flash_params->version         != FLASH_PARAM_VERSION ||
        flash_params->length          != FLASH_PARAM_LENGTH) {

//        pprintf(serial0_putc, "*** FLASH parameter block invalid, reverting to defaults ***\n");

        // restore default values.
        struct flash_params default_params;
        default_params.start_signature = FLASH_PARAM_SIGNATURE;
        default_params.version = FLASH_PARAM_VERSION;
        default_params.length  = FLASH_PARAM_LENGTH;
        default_params.ipaddr  = inet_addr("192.168.237.7");
        default_params.ipmask  = inet_addr("255.255.255.0");
        default_params.ipgw    = inet_addr("192.168.237.1");
        default_params.macaddr = 0x02000000ededULL;
        default_params.end_signature = FLASH_PARAM_SIGNATURE;
        default_params.dhcpd_enable = 1;

        flash_erase_and_write((uint32_t) flash_params, &default_params, sizeof(struct flash_params));
    }
}
