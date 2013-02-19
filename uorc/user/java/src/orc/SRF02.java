package orc;

/** Devantech SRF02 ultrasound range finder **/
public class SRF02
{
    Orc orc;
    int i2caddr;
    static final int DEFAULT_I2C_ADDR = 0x70;

    static double SOUND_METERS_PER_SEC = 343;

    public SRF02(Orc orc, int i2caddr)
    {
        this.orc = orc;
        this.i2caddr = i2caddr;
    }

    public SRF02(Orc orc)
    {
        this(orc, DEFAULT_I2C_ADDR);
    }

    /** Initiate a ping. Call readTime() 70ms later to get the value. **/
    public void ping()
    {
        byte resp[] = orc.i2cTransaction(i2caddr, new byte[] { 0x00, 0x52 }, 0);
    }

    /** Should be called at least 70ms after ping() **/
    public double readTime()
    {
        byte resp[] = orc.i2cTransaction(i2caddr, new byte[] { 0x02 }, 4);

        int usecs = ((resp[0]&0xff)<<8) + (resp[1]&0xff);

        int minusecs = ((resp[2]&0xff)<<8) + (resp[3]&0xff);

        return usecs/1000000.0;
    }

    public double readRange()
    {
        return readTime() * SOUND_METERS_PER_SEC / 2.0;
    }

    /** ping(), wait, getRange() **/
    public double measure()
    {
        ping();
        try {
            Thread.sleep(70);
        } catch (InterruptedException ex) {
        }
        return readRange();
    }

    public static void main(String args[])
    {
        Orc orc = Orc.makeOrc();

        SRF02 srf = new SRF02(orc);

        while (true) {
            System.out.printf("%15f m\n", srf.measure());
        }
    }

}
