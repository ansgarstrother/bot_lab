#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "uorc.h"

static int64_t timestamp_now()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

void benchmark(uorc_t *uorc)
{
    // benchmark
    int64_t time0 = timestamp_now();

    for (int i = 0; i < 1000; i++) {
        char *version = uorc_get_version(uorc);
        free(version);
    }

    int64_t time1 = timestamp_now();

    double dt = (time1 - time0) / 1000000.0;
    printf("%.3f\n", 1000 / dt);
}

int main(int argc, char *argv[])
{
    uorc_t *uorc = uorc_create();

    uorc_status_t status;

    struct uorc_motor leftMotor = { .uorc = uorc, .port = 0, .invert = 0 },
        rightMotor = { .uorc = uorc, .port = 1, .invert = 1 };

    uorc_motor_set_pwm(&leftMotor, .75);

    while (1) {
        uorc_get_status(uorc, &status);
        printf("%15d %15f\n", status.utime_orc, uorc_motor_get_current(&leftMotor, &status));
    }

    uorc_destroy(uorc);
}
