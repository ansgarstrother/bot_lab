#include "param.h"
#include "flash.h"

void param_init()
{
    if (flash_params->start_signature != FLASH_PARAM_SIGNATURE ||
        flash_params->end_signature   != FLASH_PARAM_SIGNATURE ||
        flash_params->version         != FLASH_PARAM_VERSION ||
        flash_params->length          != FLASH_PARAM_LENGTH) {

        // restore default values.
        struct flash_params default_params;
        default_params.start_signature = FLASH_PARAM_SIGNATURE;
        default_params.version = FLASH_PARAM_VERSION;
        default_params.length  = FLASH_PARAM_LENGTH;

        for (int i = 0; i < 8; i++) {
            default_params.gyro_offset[i] = 0;
        }

        default_params.gyro_gain[0] = 1.0E6 / 66602.0 * 3.0;
        default_params.gyro_gain[3] = 1.0E6 / 66602.0 * 3.0;
        default_params.gyro_gain[4] = 1.0E6 / 66602.0 * 3.0;
        default_params.gyro_gain[7] = 1.0E6 / 66602.0 * 3.0;
        default_params.gyro_gain[1] = 1.0E6 / 266410.0 * 3.0;
        default_params.gyro_gain[2] = 1.0E6 / 266410.0 * 3.0;
        default_params.gyro_gain[5] = 1.0E6 / 266410.0 * 3.0;
        default_params.gyro_gain[6] = 1.0E6 / 266410.0 * 3.0;

        default_params.end_signature = FLASH_PARAM_SIGNATURE;

        kprintf("resetting to default parameters\n");

        flash_erase_and_write((uint32_t) flash_params, &default_params, sizeof(struct flash_params));
    }
}
