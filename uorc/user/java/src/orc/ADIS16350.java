package orc;

/** ADIS16350 6-DOF gyro connected via SPI **/
public class ADIS16350
{
    Orc orc;
    int maxclk = 100000; // device max = 2mhz
    int spo = 1;
    int sph = 1;
    int nbits = 16;

    public ADIS16350(Orc orc)
    {
        this.orc = orc;
    }

    int readRegister(int addr)
    {
        int v[] = orc.spiTransaction(maxclk, spo, sph, nbits, new int[] { (addr<<8) });
        v = orc.spiTransaction(maxclk, spo, sph, nbits, new int[] { (addr<<8) });

        return v[v.length-1];
    }

    static int convert14(int v)
    {
        v &= 0x3fff;
        if ((v&0x2000)>0) // sign extension
            v |= 0xffffc000;
        return v;
    }

    static int convert12(int v)
    {
        v &= 0x0fff;
        if ((v&0x0800)>0) // sign extension
            v |= 0xfffff000;
        return v;
    }

    void writeRegister(int addr)
    {
        int v[] = orc.spiTransaction(maxclk, spo, sph, nbits, new int[] { (addr | 0x80)<<8, 0 });
    }

    public double[] readState()
    {
        // this could probably be done better as a big batch transaction rather than 11 different transactions
        double x[] = new double[11];
        x[0] = convert12(readRegister(0x02)) * 0.0018315; // power supply measurement (V)
        x[1] = Math.toRadians(convert14(readRegister(0x04)) * 0.07326); // x-axis gyro, rad/sec (uncalib)
        x[2] = Math.toRadians(convert14(readRegister(0x06)) * 0.07326); // y-axis gyro, rad/sec (uncalib)
        x[3] = Math.toRadians(convert14(readRegister(0x08)) * 0.07326); // z-axis gyro, rad/sec (uncalib)
        x[4] = convert14(readRegister(0x0a)) * 0.002522; // x-axis accel, gs
        x[5] = convert14(readRegister(0x0c)) * 0.002522; // y-axis accel, gs
        x[6] = convert14(readRegister(0x0e)) * 0.002522; // z-axis accel, gs
        x[7] = convert12(readRegister(0x10)) * 0.1453 + 25.0; // x-axis gyro temperature (deg C)
        x[8] = convert12(readRegister(0x12)) * 0.1453 + 25.0; // y-axis gyro temperature (deg C)
        x[9] = convert12(readRegister(0x14)) * 0.1453 + 25.0; // z-axis gyro temperature (deg C)
        x[10] = convert12(readRegister(0x16)) * 0.0006105; // auxillary ADC (V)

        return x;
    }

    public static void main(String args[])
    {
        Orc orc = Orc.makeOrc();
        ADIS16350 adis = new ADIS16350(orc);

        double zpos = 0;

        long lastTime = System.currentTimeMillis();
        while (true) {

            System.out.printf("%20.5f ", System.currentTimeMillis()/1000.0);

            if (true) {
                double state[] = adis.readState();
                //System.out.printf("%15f %15f %15f\n", state[4], state[5], state[6]);
                for (int i = 0; i < state.length; i++)
                    System.out.printf("%15f", state[i]);
            } else {
                System.out.printf("%15f", convert14(adis.readRegister(0x0e))*0.002522);
            }
            System.out.printf("\n");

            try {
                Thread.sleep(5);
            } catch (InterruptedException ex) {
            }
        }
    }
}
