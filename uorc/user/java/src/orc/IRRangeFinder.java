package orc;

/**
 * Orc wrapper for Infra-red range sensors.
 *
 * The manufacturer (Sharp) suggests that a line can be fit to a
 * function of the form 1/(d + Xd), where d is the distance and Xd is
 * some calibration value. We use parameters Xm and Xb as the
 * parameters of the line.
 *
 * V = 1/(d + Xd) * Xm + Xb
 *
 * Solving for d:
 *
 * d = (Xm/(V-Xb)) - Xd
 **/
public class IRRangeFinder
{
    AnalogInput ain;

    double Xd, Xm, Xb;
    double arcAngle = 0.08;
    double voltageStdDev = 0.025;

    /** Create an IRRangeFinder without any parameters-- useful only
        when you provide your own parameters. Otherwise, use (e.g.)
        makeGP2D12.
    **/
    public IRRangeFinder(Orc orc, int port)
    {
        this.ain = new AnalogInput(orc, port);
    }

    /** Get the range of the sensor in meters, returning 0 if the
     * sensor appears to be outside its working range.
     **/
    public double[] getRangeAndUncertainty()
    {
        double v = ain.getVoltage();

        double range = Xm/(v-Xb) + Xd;
        if (range < 0)
            range = 0;
        if (range > 100)
            range = 100;

        return new double[] { range, getRangeUncertainty(v) };
    }

    public double getRange()
    {
        return getRangeAndUncertainty()[0];
    }

    /** Estimate the standard deviation of the range measurement. This
     * assumes a perfect distance versus voltage model, and that noise
     * only occurs on the voltage reading.
     * @param v The voltage
     **/
    double getRangeUncertainty(double v)
    {
        /* We assume some constant uncertainty in voltage, and map this
           to uncertainty in distance by considering the derivative of
           our function fit.

           The derivative is:

           dd/dV = -Xm / ((V-Xb)^2)

           This gives us a line around V, a local approximation of the
           curve:

           deltaV = dd/dV|V * deltaD

           We solve for deltaD, which is:

           deltaD = deltaV / (dd/dV|V)

           (This is right: big derivatives mean small error.)

           And the variance goes as the square of any multiplicative
           factors...
        */

        double dddV = Math.abs(-Xm/(v-Xb)/(v-Xb));

        return voltageStdDev*dddV;
    }

    /** Configure distance and other parameters. **/
    public void setParameters(double Xd, double Xm, double Xb,
                              double voltageStdDev)
    {
        this.Xd = Xd;
        this.Xm = Xm;
        this.Xb = Xb;
        this.voltageStdDev = voltageStdDev;
    }

    /** Create and return an IRRangeFinder configured with approximate
     * parameters for a Sharp 2Y0A02. **/
    public static IRRangeFinder make2Y0A02(Orc orc, int port)
    {
        IRRangeFinder s = new IRRangeFinder(orc, port);
        s.Xd = -0.0618;
        s.Xm = 0.7389;
        s.Xb = -.1141;

        return s;
    }

    /** Create and return an IRRangeFinder configured with approximate
     * parameters for a Sharp GP2D12.
     * @port [0,7]
     **/
    public static IRRangeFinder makeGP2D12(Orc orc, int port)
    {
        IRRangeFinder s = new IRRangeFinder(orc, port);
        s.Xd = 0.0828;
        s.Xm = 0.1384;
        s.Xb = 0.2448;

        return s;
    }

}
