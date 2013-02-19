#include <math.h>
#include <nkern.h>
#include <extadc.h>
#include <motor.h>

enum wfstate_t { STATE_LOST, STATE_FOLLOW_LEFT, STATE_FOLLOW_RIGHT };

static double min(double a, double b)
{
    if (a<b)
        return a;
    return b;
}

static double short_ir_v2d(double v)
{
    double Xd = 0.0043*0.0254;
    double Xm = 4.536*0.0254;
    double Xb = -0.0372*0.0254;

    double d = Xm/(v-Xb) + Xd;
    if (d < 0)
        return 0;
    if (d > 100)
        return 100;
    return d;
}

static void motors(double left, double right)
{
    motor_set_goal_pwm(0, (int) (-255*left));
    motor_set_goal_pwm(1, (int) (255*right));
}

static double left_range()
{
    return short_ir_v2d(extadc_get_filtered(1)*5.0/65535.0);
}

static double right_range()
{
    return short_ir_v2d(extadc_get_filtered(2)*5.0/65535.0);
}

static double front_range()
{
    return short_ir_v2d(extadc_get_filtered(3)*5.0/65535.0);
}

static double clamp(double v)
{
    if (v < -1)
        return -1;
    if (v > 1)
        return 1;
    return v;
}

extern iop_t *serial0_iop;

void wallfollower_task(void *arg)
{
    enum wfstate_t state = STATE_LOST;
    double gain = 1;
    double lostRange = 0.3;
    double goalRange = 0.2;
    double panicRange = 0.15;
    int LOST_DELAY_THRESH = 24;

    int lostDelay = 0;

    motor_set_enable(0, 1);
    motor_set_enable(1, 1);

    while (1) {
        double left = left_range();
        double right = right_range();
        double front = front_range();
        
        switch (state) 
        {
        case STATE_LOST: {
            lostDelay = 0;
            if (min(left,right) < lostRange) {
                if (left < right)
                    state = STATE_FOLLOW_LEFT;
                else
                    state = STATE_FOLLOW_RIGHT;
            }

            if (front < panicRange) 
                motors(-0.5, 0.5);
            else
                motors(0.5, 0.5);
            break;
        }

        case STATE_FOLLOW_LEFT: {
            if (left > lostRange)
                lostDelay++;
            else
                lostDelay = 0;

            if (lostDelay > LOST_DELAY_THRESH)
                state = STATE_LOST;

            double error = left - goalRange;

            if (lostDelay > 0) 
                motors(.2, .5);
            else if (front > panicRange)
                motors(clamp(0.5 - gain*error), clamp(.5 + gain*error));
            else
                motors(.5, -.5);
            break;

        }

        case STATE_FOLLOW_RIGHT: {
            if (right > lostRange)
                lostDelay++;
            else
                lostDelay = 0;

            if (lostDelay > LOST_DELAY_THRESH)
                state = STATE_LOST;
            
            double error = right - goalRange;

            if (lostDelay > 0)
                motors(.5, .2);
            else if (front > panicRange)
                motors(clamp(0.5 + gain*error), clamp(0.5 - gain*error));
            else
                motors(-.5, .5);

            break;
        }
        }

        pprintf(serial0_iop, "%3d %10d %10d %10d %10d\n", state, (int) (left*100), (int) (right*100), (int) (front*100), lostDelay);

        nkern_usleep(30000);
    }
}
