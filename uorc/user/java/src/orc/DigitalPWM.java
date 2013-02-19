package orc;

/** Allows low-speed PWM (on the FAST digital I/O pins), meaning rates
 * up to 1khz. Timing will not be terribly accurate, since this PWM is
 * generated in software but should be good to a few percent.
 **/
public class DigitalPWM
{
    Orc orc;
    int port;

    /** Initialize PWM output.
     *
     * @param port Digital I/O port number on the FAST inputs [0,7]
     **/
    public DigitalPWM(Orc orc, int port)
    {
        this.orc = orc;
        this.port = port;
    }

    /** Note that small values may have a significant effect on the
        CPU utilization of the uorc. Recommended to keep frequency
        under 100Hz (period over 10000 usec).

        @param period_usec [1000, 1000000]
        @param dutyCycle [0, 1]

    **/
    public void setPWM(int period_usec, double dutyCycle)
    {
        assert (dutyCycle >= 0 && dutyCycle <= 1.0);
        assert (period_usec >= 1000 && period_usec <= 1000000);

        int iduty = (int) (dutyCycle * 0xfff);
        int command = (iduty << 20) + (period_usec);

        orc.doCommand(0x7000, new byte[] { (byte) (port - 8), Orc.FAST_DIGIO_MODE_OUT,
                                           (byte) ((command>>24)&0xff),
                                           (byte) ((command>>16)&0xff),
                                           (byte) ((command>>8)&0xff),
                                           (byte) ((command>>0)&0xff) });

    }

    public void setValue(boolean v)
    {

    }

    public static void main(String args[])
    {
        Orc orc = Orc.makeOrc();
        DigitalOutput dout = new DigitalOutput(orc, 0);

        while (true) {
            try {
                dout.setValue(true);
                System.out.println("true");
                Thread.sleep(1000);
                dout.setValue(false);
                System.out.println("false");
                Thread.sleep(1000);
            } catch (InterruptedException ex) {
            }
        }

    }
}
