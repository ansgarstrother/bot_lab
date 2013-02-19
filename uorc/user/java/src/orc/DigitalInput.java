package orc;

/** Represents a digital input on the simple digital IO pins. **/
public class DigitalInput
{
    Orc orc;
    int port;
    boolean invert;

    /** Initialize an input-only pin.
     *
     * @param port Digital I/O port number, simple digital I/O are
     * [0,7], fast digital I/O are [8,15]. Note that on fast ports,
     * pull-ups are always enabled.
     **/
    public DigitalInput(Orc orc, int port, boolean pullup, boolean invert)
    {
        this.orc = orc;
        this.port = port;
        this.invert = invert;

        if (port < 8)
            orc.doCommand(0x6000, new byte[] { (byte) port, 1, (byte) (pullup ? 1 : 0)});
        else
            orc.doCommand(0x7000, new byte[] { (byte) (port - 8), Orc.FAST_DIGIO_MODE_IN, 0, 0, 0, 0 });
    }

    public boolean getValue()
    {
        OrcStatus os = orc.getStatus();

        boolean v;

        if (port < 8)
            v = ((os.simpleDigitalValues&(1<<port))!=0);
        else
            v = (os.fastDigitalConfig[port-8]!=0);

        return v ^ invert;
    }
}
