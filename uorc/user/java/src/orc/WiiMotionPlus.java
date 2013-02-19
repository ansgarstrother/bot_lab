package orc;

import orc.*;
import orc.util.*;
import java.io.*;

/** Accesses Nintendo WiiMotion Plus via I2C (use the 3.3V port only!) **/
// PINOUT:    Green:    Data
//            White:    Ground
//            Red:      3.3V
//            Yellow:   Clock
public class WiiMotionPlus
{
    Orc orc;

    static final int I2C_INIT_ADDRESS = 0x53;
    static final int I2C_ADDRESS      = 0x52;

    public WiiMotionPlus(Orc orc)
    {
        this.orc = orc;

        // this will generate an i2c error when the device has already been switched
        // to the other I2C address.
        orc.i2cTransaction(I2C_INIT_ADDRESS, new byte[] { (byte) 0xfe, 0x04 }, 0);

        // The first write to 0x00 always seems to result in an error.
        orc.i2cTransaction(I2C_ADDRESS, new byte[] { 0x00 }, 0);
    }

    /** Returns a three-dimensional array giving the raw angular rate
     * data for each of the three axes.
     **/
    public int[] readAxes()
    {
        byte resp[] = orc.i2cTransaction(I2C_ADDRESS,
                                         new byte[] {0x00}, 6);

        int data[] = new int[3];
        data[0] = (resp[0]&0xff) + ((resp[3]&0xfc)<<6);
        data[1] = (resp[1]&0xff) + ((resp[4]&0xfc)<<6);
        data[2] = (resp[2]&0xff) + ((resp[5]&0xfc)<<6);

        return data;
    }

    public static void main(String args[])
    {
        Orc orc = Orc.makeOrc();

        WiiMotionPlus wmp = new WiiMotionPlus(orc);

        while (true) {
            int axes[] = wmp.readAxes();

            System.out.printf("%10d %10d %10d\n", axes[0], axes[1], axes[2]);
            try {
                Thread.sleep(30);
            } catch (InterruptedException ex) {
            }
        }
    }
}
