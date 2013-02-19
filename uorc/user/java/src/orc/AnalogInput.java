package orc;

/** Represents one of the 14bit external ADC ports. **/
public class AnalogInput
{
    Orc orc;
    int port;

    public AnalogInput(Orc orc, int port)
    {
        assert(port>=0 && port<=7);

        this.orc = orc;
        this.port = port;
    }

    public double getVoltage()
    {
        OrcStatus status = orc.getStatus();
        return status.analogInputFiltered[port]/65535.0 * 5; //4.096;
    }

    public double getVoltageUnfiltered()
    {
        OrcStatus status = orc.getStatus();
        return status.analogInput[port]/65535.0 * 5; //4.096;
    }

    /** Set the alpha coefficient for the low-pass filter. y[n] =
     * alpha*y[n-1] + (1-alpha)*x[n]. I.e., values near one represent
     * low-pass filters with greater effect. The nominal sample rate
     * is 500Hz.
     *
     * @param alpha [0,1].
     **/
    public void setLPF(double alpha)
    {
        assert(alpha>=0 && alpha<=1.0);
        int v = (int) (alpha*65536);
        orc.doCommand(0x3000, new byte[] {(byte) port,
                                          (byte) ((v>>8)&0xff),
                                          (byte) ((v>>0)&0xff)});
    }
}
