package orc;

/** Represents a quadrature phase decoder input on the uOrc Board. **/
public class QuadratureEncoder
{
    Orc orc;
    int port;
    boolean invert;

    // This should match the firmware...
    static final int QEI_VELOCITY_SAMPLE_HZ = 40;

    public QuadratureEncoder(Orc orc, int port, boolean invert)
    {
        assert(port>=0 && port<=1);

        this.orc = orc;
        this.port = port;
        this.invert = invert;
    }

    public int getPosition()
    {
        return getPosition(orc.getStatus());
    }

    public int getPosition(OrcStatus status)
    {
        assert(status.orc == this.orc);

        return status.qeiPosition[port] * (invert ? -1 : 1);
    }

    /** In ticks per second (XXX: Might return unsigned speed due to hardware errata!). **/
    public double getVelocity()
    {
        return getVelocity(orc.getStatus());
    }

    public double getVelocity(OrcStatus status)
    {
        assert(status.orc == this.orc);

        return status.qeiVelocity[port] * (invert ? -1 : 1) * QEI_VELOCITY_SAMPLE_HZ;
    }
}
