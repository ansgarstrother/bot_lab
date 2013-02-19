package orc;

/** A TLC3548 analog-to-digital converter connected via SPI. **/
public class TLC3548
{
    Orc orc;
    int maxclk = 2500000;
    int spo = 0;
    int sph = 1;
    int nbits = 16;
    boolean shortSampling = false; // is datasheet backwards?

    boolean externalReference = true; // else 4v internal reference

    public TLC3548(Orc orc)
    {
        this.orc = orc;

        // init the device.
        orc.spiTransaction(maxclk, spo, sph, nbits, new int[] { 0xa000 });

        int flag = (0<<8); // internal OSC
        if (externalReference)
            flag |= (1<<11);
        if (shortSampling)
            flag |= (1<<9);

        orc.spiTransaction(maxclk, spo, sph, nbits, new int[] { 0xa000 | flag });
    }

    // begin a conversion on port, return the result of the last conversion
    public int beginConversion(int port)
    {
        int rx[] = orc.spiTransaction(maxclk, spo, sph, nbits, new int[] { port << 12, 0, 0, 0, 0, 0, 0, 0, 0, 0});

        return rx[0];
    }

    public static void main(String args[])
    {
        Orc orc = Orc.makeOrc();
        TLC3548 tlc = new TLC3548(orc);

        while (true) {
            for (int i = 0; i < 8; i++)
                System.out.printf("%04x  ", tlc.beginConversion(i));
            System.out.printf("\n");


            try {
                Thread.sleep(20);
            } catch (InterruptedException ex) {
            }

        }

    }
}
