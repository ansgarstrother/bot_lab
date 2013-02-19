#include <nkern.h>
#include "lm3s8962.h"
#include "config.h"

static int qei0_dir; // either +1 or -1.
static int qei1_dir;

// The QEISPEED register is unsigned, so we track the speed
// ourselves. This introduces some race conditions, but it's the best
// we can do. Note, we used to use QEISTAT & (1<<1) as our direction
// indicator, but this is vulnerable to noise: a single errant
// transition will result in the wrong sign.
static void qei_task(void *arg)
{
    int32_t qei0last = 0, qei1last = 0;

    while (1) {

        int32_t qei0 = QEI0_POS_R;
        int32_t qei1 = QEI1_POS_R;

        int32_t qei0diff = qei0 - qei0last;
        int32_t qei1diff = qei1 - qei1last;

        qei0last = qei0;
        qei1last = qei1;

        if (qei0diff != 0)
            qei0_dir = (qei0 - qei0last) >= 0 ? 1 : -1;

        if (qei1diff != 0)
            qei1_dir = (qei1 - qei1last) >= 0 ? 1 : -1;

        nkern_usleep(CPU_HZ / QEI_VELOCITY_SAMPLE_HZ / 2);
    }
}

void qei_init()
{
    nkern_task_create("qei",
                      qei_task, NULL,
                      NKERN_PRIORITY_NORMAL, 512);
}

uint32_t qei_get_position(int port)
{
    if (port == 0) {
        return QEI0_POS_R;
    } else {
        return QEI1_POS_R;
    }

    return 0;
}

// NOTE: QEI speed isn't signed. Isn't that dumb? We fix it here, but race condition?
int32_t qei_get_velocity(int port)
{
    if (port == 0) {
        uint32_t stat = QEI0_STAT_R;
        uint32_t speed = QEI0_SPEED_R;

        return (qei0_dir > 0) ? speed : -speed;

    } else {
        uint32_t stat = QEI1_STAT_R;
        uint32_t speed = QEI1_SPEED_R;

        return (qei1_dir > 0) ? speed : -speed;
    }

    return 0;
}
