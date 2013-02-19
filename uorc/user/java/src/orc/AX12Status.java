package orc;

public class AX12Status
{
    public static final int ERROR_INSTRUCTION = (1<<6);
    public static final int ERROR_OVERLOAD    = (1<<5);
    public static final int ERROR_CHECKSUM    = (1<<4);
    public static final int ERROR_RANGE       = (1<<3);
    public static final int ERROR_OVERHEAT    = (1<<2);
    public static final int ERROR_ANGLE_LIMIT = (1<<1);
    public static final int ERROR_VOLTAGE     = (1<<0);

    /** uorc utime **/
    public long utimeOrc;
    public long utimeHost;

    /** Degrees **/
    public double positionDegrees;

    /** Speed [-1,1] **/
    public double speed;

    /** Current load [-1, 1]. Mostly meaningless unless the servo has
     * been commanded to track a particular angle.  **/
    public double load;

    /** Voltage (volts) **/
    public double voltage;

    /** Temperature (Deg celsius) **/
    public double temperature;

    /** Error flags--- see ERROR constants. **/
    public int error_flags;

    public void print()
    {
        System.out.printf("position:    %f\n", positionDegrees);
        System.out.printf("speed:       %f\n", speed);
        System.out.printf("load:         %f\n", load);
        System.out.printf("voltage:     %f\n", voltage);
        System.out.printf("temperature: %f\n", temperature);
        System.out.printf("error flags:  %d\n", error_flags);
        System.out.printf("utime (orc):  %d\n", utimeOrc);
        System.out.printf("utime (host): %d\n", utimeHost);
    }
}
