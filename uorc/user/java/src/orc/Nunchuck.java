package orc;

import orc.*;
import orc.util.*;
import java.io.*;

/** Accesses Nintendo Wii Nunchuck via I2C; use the 3.3V port **/
// PINOUT:    Green:    Data
//            White:    Ground
//            Red:      3.3V
//            Yellow:   Clock
//
//NOTE: The Acceleromoter returns an 10-bit value, the Joystick returns a 8-bit value.
//
//X-Axis joystick return values,
//    Full Left: 0x1E    Middle: 0x7E    Full Right: 0xE1
//Y-Axis joystick return values,
//    Full Down: 0x1D    Middle: 0x7B    Full Up: 0xDF
//
//X-Axis Accelerometer (at 1G)
//    Minimum:           Medium:         Maximum:
//Y-Axis Accelerometer (at 1G)
//    Minimum:           Medium:         Maximum:
//Z-Axis Accelerometer (at 1G)
//    Minimum:           Medium:         Maximum:
//
//Button return values are,
//    Bit 0: Z-button (0 = pressed, 1 = released)
//    Bit 1: C-button (0 = pressed, 1 = released)
public class Nunchuck
{
    Orc orc;

    //Used to receive data from Nunchuck
    byte writeme2[] = new byte[1];
    byte readme2[] = new byte[20];
    byte addrss = (byte) 0x52;
    byte writeleng;
    byte readleng;

    static final int I2C_ADDRESS = -92;

    //Create Nunchuck
    public Nunchuck(Orc orc)
    {
        this.orc = orc;
        writeme2[0] = 0x00;

        orc.i2cTransaction(I2C_ADDRESS, new byte[] {0x40, 0x00}, 0);
    }

    /** Read the whole nuncheck state vector at once; a vector is
     * returned containing joyx, joyy, accelx, accely, accelz,
     * buttons **/
    public int[] readState()
    {
        orc.i2cTransaction(I2C_ADDRESS, new byte[] {0}, 0);
        byte resp[] = orc.i2cTransaction(I2C_ADDRESS, null, 6);

        // 0: joystick x
        // 1: joystick y
        // 2: accel x
        // 3: accel y
        // 4: accel z
        // 5: button

        int state[] = new int[6];
        state[0] = ((resp[0]&0xff)^0x17) + 0x17;
        state[1] = ((resp[1]&0xff)^0x17) + 0x17;

        state[2] = ((resp[2]&0xff)^0x17) + 0x17;
        state[3] = ((resp[3]&0xff)^0x17) + 0x17;
        state[4] = ((resp[4]&0xff)^0x17) + 0x17;

        state[5] = (((resp[5]&0xff)^0x17) + 0x17) & 0x03;

        return state;
    }

    public int[] readJoystick()
    {
        int v[] = readState();
        return new int[] { v[0], v[1]};
    }

    public int[] readAccelerometers()
    {
        int v[] = readState();
        return new int[] { v[2], v[3], v[4]};
    }

    public int readButtons()
    {
        int v[] = readState();
        return v[5];
    }

}
