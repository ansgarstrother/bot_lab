package orc;

/** An analog rate gyro can be connected to one of the first 3
 * external ADC ports. The uOrc hardware includes special integrator
 * firmware; this provides an interface to that logic.
 *
 * @param port [0,2]
 **/
public class Gyro
{
    Orc orc;
    int port;

    double SAMPLE_HZ; // should match value on uOrc, but we calibrate this.

    // average value of integrator corresponding to no rotation (calibrated)
    double v0;

    // sensitivity rating of part. ADXRS150 = 0.0125, ADXRS300=0.005, ADXRS614=0.025
    double mvPerDegPerSec;

    // what is our rotational speed (in rad/sec) for every LSB deviation from v0?
    double radPerSecPerLSB = 0.0000532638;

    long integratorOffset;
    int  integratorCountOffset;

    boolean calibrated;

    double voltsPerLSB = 4.0/65536; // property of the ADCs

    double theta = 0; // gyro position in radians

    /** Create a gyro with 0.005 V/(deg/S) sensitivity **/
    public Gyro(Orc orc, int port)
    {
        this(orc, port, 0.005);
    }

    public Gyro(Orc orc, int port, double voltsPerDegPerSec)
    {
        assert(port>=0 && port <=2);
        this.orc = orc;
        this.port = port;

        double lsbPerDegPerSec = (1.0/voltsPerLSB) * voltsPerDegPerSec;
        double degPerSecPerLSB = 1.0 / lsbPerDegPerSec;
        this.radPerSecPerLSB = Math.toRadians(degPerSecPerLSB);

        reset();
    }

    /** Reset the gyro position to zero. **/
    public void reset()
    {
        OrcStatus status = orc.getStatus();
        integratorOffset = status.gyroIntegrator[port];
        integratorCountOffset = status.gyroIntegratorCount[port];
        theta = 0;
    }

    public synchronized double getTheta()
    {
        if (!calibrated) {
            System.out.println("orc.Gyro: Must calibrate before calling getTheta!()");
            calibrated = true; // silence further warnings.
            return theta;
        }

        OrcStatus s = orc.getStatus();

        double integrator = s.gyroIntegrator[port] - integratorOffset;
        double integratorCount = s.gyroIntegratorCount[port] - integratorCountOffset;

        if (integratorCount == 0) {
            // no time has passed between reset and getTheta().
            // (haven't calibrated?)
            return theta;
        }

        double dt = integratorCount / SAMPLE_HZ; // time that the integrator has been running
        double averageIntegrator = integrator/integratorCount - v0;

        //	System.out.printf("dt: %15f %15f\n", dt, averageIntegrator);

        theta += averageIntegrator * dt * radPerSecPerLSB;

        integratorOffset = s.gyroIntegrator[port];
        integratorCountOffset = s.gyroIntegratorCount[port];

        return theta;
    }

    /** Calibrate the gyro while the gyro remains motionless. This
     * calibration will only measure the offset error, it can not
     * measure scale error. Also, the gyro theta is not reset: you may
     * wish to call reset() after calibration. **/
    public void calibrate(double seconds)
    {
        OrcStatus s0 = orc.getStatus();

        try {
            Thread.sleep(((int) (seconds*1000)));
        } catch (InterruptedException ex) {
        }

        OrcStatus s1 = orc.getStatus();

        double dt = (s1.utimeOrc - s0.utimeOrc)/1000000.0;
        double dv = (s1.gyroIntegrator[port] - s0.gyroIntegrator[port]);
        double ds = (s1.gyroIntegratorCount[port] - s0.gyroIntegratorCount[port]);
        SAMPLE_HZ = ds/ dt;
        this.v0 = dv / ds;

        System.out.printf("Requested calib t: %15f seconds\n", seconds);
        System.out.printf("Actual calib t:    %15f seconds\n", dt);
        System.out.printf("Integrator change: %15.1f ADC LSBs\n", dv);
        System.out.printf("Integrator counts: %15.1f counts\n", ds);
        System.out.printf("Sample rate:       %15f Hz\n", SAMPLE_HZ);
        System.out.printf("calibrated at:     %15f ADC LSBs (about %f V)\n", v0, v0/65535*5.0);

        calibrated = true;

        // don't reset automatically.
    }

    public static void main(String args[])
    {
        int port = 0;
        Orc orc = Orc.makeOrc();
        Gyro gyro = new Gyro(orc, port);
        AnalogInput ain = new AnalogInput(orc, port);

        double calibrateTime = 3.0;
        System.out.println("Calibrating for "+calibrateTime+" seconds...");
        gyro.calibrate(calibrateTime);

        double starttime = System.currentTimeMillis()/1000.0;

        while (true) {
            double rad = gyro.getTheta();
            double dt = System.currentTimeMillis()/1000.0 - starttime;

            System.out.printf("\r t=%15f V=%15f theta=%15f rad (%15f deg)", dt, ain.getVoltage(),rad, Math.toDegrees(rad));
            try {
                Thread.sleep(100);
            } catch (InterruptedException ex) {
            }
        }
    }
}
