#ifndef __UORC_H
#define __UORC_H

#include <stdint.h>

typedef struct uorc_response uorc_response_t;
struct uorc_response
{
    volatile int      valid; // did we get a response?
    uint32_t transaction_id;
    int64_t utime_orc;
    int64_t utime_host;
    uint32_t response_id;

    uint8_t *buffer;
    uint32_t buffer_len;
    uint32_t buffer_pos;
};

typedef struct uorc uorc_t;

uorc_t* uorc_create();

void uorc_destroy(uorc_t *uorc);

/** use timeoutms < 0 for infinite retries. **/
uorc_response_t *uorc_command(uorc_t *uorc, uint32_t command,
                              const uint8_t *payload, uint32_t payload_len, int timeoutms);

void uorc_response_destroy(uorc_response_t *response);

/*******************************************************************
 * User functions below.
 *******************************************************************/

typedef struct uorc_motor uorc_motor_t;
struct uorc_motor
{
    uorc_t *uorc;
    int port;
    int invert;
};

typedef struct uorc_encoder uorc_encoder_t;
struct uorc_encoder
{
    uorc_t *uorc;
    int port;
    int invert;
};

typedef struct uorc_status uorc_status_t;
struct uorc_status
{
    int64_t utime_orc;
    int64_t utime_host;

    uint32_t status_flags;
    uint32_t debug_chars_waiting;

    uint32_t analog_input[13];
    uint32_t analog_input_filtered[13];
    uint32_t analog_input_filter_alpha[13];

    uint32_t simple_digital_values;
    uint32_t simple_digital_directions;

    uint32_t motor_enable[3];
    int32_t motor_pwm_actual[3];
    int32_t motor_pwm_goal[3];
    uint32_t motor_slew_raw[3]; // (PWM ticks per ms) * 256

    uint32_t qei_position[2];
    uint32_t qei_velocity[2];

    uint32_t fast_digital_mode[8];
    uint32_t fast_digital_config[8];

    uint64_t gyro_integrator[3];
    uint32_t gyro_integrator_count[3];
};

/** You must free the resulting string **/
char *uorc_get_version(uorc_t *uorc);

/** You allocate the status object, we write into it. **/
void uorc_get_status(uorc_t *uorc, uorc_status_t *status);

/** v is [-1, 1].
 * Remember: the uorc must receive motor commands periodically or it will automatically stop.
 **/
void uorc_motor_set_pwm(uorc_motor_t *motor, float v);
float uorc_motor_get_current(uorc_motor_t *motor, uorc_status_t *status);


uint32_t uorc_encoder_get_position(uorc_encoder_t *enc, uorc_status_t *status);
float uorc_encoder_get_velocity(uorc_encoder_t *enc, uorc_status_t *status);

#endif
