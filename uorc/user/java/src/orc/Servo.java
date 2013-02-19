package orc;

/** Represents a Futaba-style servo connected to one of the "fast" digital I/O pins. **/
public class Servo
{
    Orc orc;
    int port;

    double pos0,  pos1;
    int    usec0, usec1;

    /** port [0,7]. **/
    public Servo(Orc orc, int port, double pos0, int usec0, double pos1, int usec1)
    {
        this.orc = orc;
        this.port = port;
        this.pos0 = pos0;
        this.usec0 = usec0;
        this.pos1 = pos1;
        this.usec1 = usec1;
    }

    public void setPulseWidth(int usecs)
    {
        orc.doCommand(0x7000, new byte[] {(byte) port,
                                          (byte) Orc.FAST_DIGIO_MODE_SERVO,
                                          (byte) ((usecs>>24)&0xff),
                                          (byte) ((usecs>>16)&0xff),
                                          (byte) ((usecs>>8)&0xff),
                                          (byte) ((usecs>>0)&0xff)});
    }

    public void idle()
    {
        int value = 0;
        orc.doCommand(0x7000, new byte[] {(byte) port,
                                          (byte) Orc.FAST_DIGIO_MODE_OUT,
                                          (byte) ((value>>24)&0xff),
                                          (byte) ((value>>16)&0xff),
                                          (byte) ((value>>8)&0xff),
                                          (byte) ((value>>0)&0xff)});
    }

    public void setPosition(double pos)
    {
        if (pos < Math.min(pos0, pos1))
            pos = Math.min(pos0, pos1);

        if (pos > Math.max(pos0, pos1))
            pos = Math.max(pos0, pos1);

        setPulseWidth((int) (usec0 + (usec1-usec0)*(pos - pos0)/(pos1-pos0)));
    }

    /** Create a servo based on a nominal MPI MX-400 servo with
     * positions ranging from [0, PI]
     **/
    public static Servo makeMPIMX400(Orc orc, int port)
    {
        return new Servo(orc, port, 0, 600, Math.PI, 2500);
    }
}
