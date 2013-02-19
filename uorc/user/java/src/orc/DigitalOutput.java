package orc;

/** Represents a digital input on the simple digital IO pins (SEE
 * WARNING!).  Warning: setting a pin to an output when an external
 * voltage is being applied can permanently damage the hardware.
 **/
public class DigitalOutput
{
    Orc orc;
    int port;
    boolean invert;

    /** Initialize an output-only pin.
     *
     * @param port Digital I/O port number, slow [0,7] or fast [8,15].
     **/
    public DigitalOutput(Orc orc, int port)
    {
        this(orc, port, false);
    }

    /** Initialize an output-only pin.
     *
     * @param port Simple digital I/O port number, slow [0,7] or fast [8,15]
     **/
    public DigitalOutput(Orc orc, int port, boolean invert)
    {
        this.orc = orc;
        this.port = port;
        this.invert = invert;

        if (port < 8)
            orc.doCommand(0x6000, new byte[] { (byte) port, 0, 0 });
        else
            orc.doCommand(0x7000, new byte[] { (byte) (port - 8), Orc.FAST_DIGIO_MODE_OUT, 0, 0, 0, 0});
    }

    public void setValue(boolean v)
    {
        if (port < 8)
            orc.doCommand(0x6001, new byte[] { (byte) port, (byte) ((v^invert) ? 1 : 0)});
        else
            orc.doCommand(0x7000, new byte[] { (byte) (port - 8), Orc.FAST_DIGIO_MODE_OUT, 0, 0, 0, (byte) ((v^invert) ? 1 : 0) });
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
