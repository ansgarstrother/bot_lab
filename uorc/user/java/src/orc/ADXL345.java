package orc;

import orc.*;
import orc.util.*;
import java.io.*;

/** Class for communicating with an ADXL345 3-axis accelerometer via
 * the I2C port (use the 3.3V connection).
 **/
public class ADXL345
{
    Orc orc;

    static final int I2C_ADDRESS = 0x1D;

    public ADXL345(Orc orc)
    {
        this.orc = orc;

        int deviceId = getDeviceId();
        assert(deviceId == 0xe5);

        orc.i2cTransaction(I2C_ADDRESS, new byte[] { 0x2d, 0x08 }, 0); // power device on.
    }

    public int getDeviceId()
    {
        byte res[] = orc.i2cTransaction(I2C_ADDRESS, new byte[] { 0x00 }, 1);

        return res[0]&0xff;
    }

    public int[] readAxes()
    {
        byte resp[] = orc.i2cTransaction(I2C_ADDRESS, new byte[] { 0x32 }, 6);

        int v[] = new int[3];
        v[0] = (resp[0]&0xff) + ((resp[1])<<8);
        v[1] = (resp[2]&0xff) + ((resp[3])<<8);
        v[2] = (resp[4]&0xff) + ((resp[5])<<8);
        return v;
    }

    public static void main(String args[])
    {
        Orc orc = Orc.makeOrc();

        ADXL345 accel = new ADXL345(orc);

        while (true) {
            int axes[] = accel.readAxes();

            System.out.printf("%10d %10d %10d\n", axes[0], axes[1], axes[2]);

            try {
                Thread.sleep(30);
            } catch (InterruptedException ex) {
            }
        }
    }
}
