package orc;

import java.io.*;
/** Basic class for controlling a motor. **/
public class Motor
{
    Orc orc;
    int port;
    boolean invert;

    /** Create a new Motor.
     * @param port [0,2]
     * @param invert If true, the direction of the will be inverted.
     **/
    public Motor(Orc orc, int port, boolean invert)
    {
        this.orc = orc;
        this.port = port;
        this.invert = invert;
    }

    /** Disconnects the motor output. The next call to setPWM will
     * reenable the motor. This is distinct from setPWM(0), which
     * actively brakes the motor.
     **/
    public void idle()
    {
        orc.doCommand(0x1000, new byte[] {(byte) port,
                                          (byte) 0,
                                          (byte) 0,
                                          (byte) 0});
    }

    /** Enable the motor and set an output PWM.
     * @param v [-1, 1]
     **/
    public void setPWM(double v)
    {
        int pwm = mapPWM(v);

        orc.doCommand(0x1000, new byte[] {(byte) port,
                                          (byte) 1,
                                          (byte) ((pwm>>8)&0xff),
                                          (byte) (pwm&0xff)});

    }

    /** Get the instantaneous PWM output of the board, accounting for
     * slew rate limits. Returns [-1, 1] **/
    public double getPWM(OrcStatus status)
    {
        double v = status.motorPWMactual[port] / 255.0;
        if (invert)
            v *= -1;
        return v;
    }

    public double getPWM()
    {
        return getPWM(orc.getStatus());
    }

    protected int mapPWM(double v)
    {
        assert(v>=-1 && v<=1);
        int pwm = (int) (v * 255);
        if (invert)
            pwm *= -1;
        return pwm;
    }

    /** Returns an instantaneous estimate of the current magnitude (in
     * Amps). The direction of the current flow is not reflected in
     * the value.
     **/
    public double getCurrent(OrcStatus status)
    {
        double voltage = status.analogInput[port + 8] / 65535.0 * 3.0;
        double current = voltage * 375.0/200.0;

        return current;
    }

    public double getCurrent()
    {
        return getCurrent(orc.getStatus());
    }

    /** Returns an instantaneous estimate of the current magnitude (in
     * Amps). The direction of the current flow is not reflected in
     * the value.
     **/
    public double getCurrentFiltered(OrcStatus status)
    {
        double voltage = status.analogInputFiltered[port + 8] / 65535.0 * 3.0;
        double current = voltage * 375.0/200.0;

        return current;
    }

    public double getCurrentFiltered()
    {
        return getCurrentFiltered(orc.getStatus());
    }

    /** Set the amount of time required for the motor to transition
     * from -1 to +1, in seconds.  Internally, slew rate is
     * implemented by adjusting the actual PWM value closer to the
     * goal value set by setPWM(). The maximum adjustment size is
     * limited by the slew rate. The Orc hardware updates the PWM
     * value at 1000Hz, with a maximum change given by a 16 bit number
     * that represents (number of PWM values)*128.
     *
     * @param seconds [0.001, 120]. Values less than 0.001 are
     * accepted, but increased to 0.001
     **/
    public void setSlewSeconds(double seconds)
    {
        assert(seconds>=0 && seconds < 120);

        seconds = Math.max(seconds, 0.001);

        // 510   : PWM values between -1 and +1.
        // 1000  : update rate (Hz)
        // 256   : iv is in fixed point with 8 fractional bits.
        double dv = (510.0 / 1000.0 / seconds)*128.0;
        int iv = (int) dv;

        iv = Math.max(iv, 1);
        iv = Math.min(iv, 65535);

        orc.doCommand(0x1001, new byte[] {(byte) port,
                                          (byte) ((iv>>8)&0xff),
                                          (byte) (iv&0xff)});
    }

    /** Motors must belong to same Orc object. **/
    public static void setMultiplePWM(Motor ms[], double vs[])
    {
        ByteArrayOutputStream outs = new ByteArrayOutputStream();

        for (int i = 0; i < ms.length; i++) {
            assert(ms[i].orc == ms[0].orc);

            Motor m = ms[i];
            outs.write((byte) m.port);
            outs.write((byte) 1);

            int pwm = m.mapPWM(vs[i]);
            outs.write((byte) ((pwm>>8)&0xff));
            outs.write((byte) (pwm&0xff));
        }

        ms[0].orc.doCommand(0x1000, outs.toByteArray());
    }

    /** Is the motor in a fault state? **/
    public boolean isFault()
    {
        return isFault(orc.getStatus());
    }

    /** Is the motor in a fault state? **/
    public boolean isFault(OrcStatus status)
    {
        assert(status.orc == this.orc);

        return (status.simpleDigitalValues & (1<<(8+port*2)))==0;
    }

    public void clearFault()
    {
        idle();
    }
}
