package orc;

import java.io.*;
import java.net.*;
import java.util.*;

import orc.util.*;

/** Represents a connection to a uOrc board. **/
public class Orc
{
    DatagramSocket sock;
    ReaderThread   reader;

    InetAddress orcAddr;

    int         nextTransactionId;

    static final int ORC_BASE_PORT = 2378;

    static final double MIN_TIMEOUT = 0.002;
    static final double MAX_TIMEOUT = 0.010;

    public static final int FAST_DIGIO_MODE_IN = 1;
    public static final int FAST_DIGIO_MODE_OUT = 2;
    public static final int FAST_DIGIO_MODE_SERVO = 3;
    public static final int FAST_DIGIO_MODE_SLOW_PWM = 4;

    double meanRTT = MIN_TIMEOUT;

    public boolean verbose = false;

    HashMap<Integer, OrcResponse> transactionResponses = new HashMap<Integer, OrcResponse>();

    ArrayList<OrcListener> listeners = new ArrayList<OrcListener>();

    // uorc counts in useconds, no wrapping, 0.1% accuracy. Reset if
    // error ever greater than 0.5 second (i.e., if board gets rebooted.)
    TimeSync ts = new TimeSync(1E6, 0, 0.001, 0.5);

    // XXX we could do a RTT-based synchronization algorithm ...

    public static void main(String args[])
    {
        Orc orc;

        if (args.length == 0)
            orc = makeOrc();
        else
            orc = makeOrc(args[0]);

        System.out.println("Version: "+orc.getVersion());

        System.out.println("Benchmarking...");

        long startmtime = System.currentTimeMillis();
        int niters = 1000;
        for (int i = 0; i < niters; i++)
            orc.getVersion();

        long endmtime = System.currentTimeMillis();
        double iterspersec = niters / ((endmtime - startmtime)/1000.0);
        System.out.printf("Iterations per second: %.1f\n", iterspersec);

    }

    public InetAddress getAddress()
    {
        return orcAddr;
    }

    public static Orc makeOrc()
    {
        return makeOrc("192.168.237.7");
    }

    public long toHostUtime(long uorcUtime)
    {
        return ts.getHostUtime(uorcUtime);
    }

    public String getVersion()
    {
        OrcResponse resp = doCommand(0x0002, null);
        StringBuffer sb = new StringBuffer();

        try {
            while (resp.ins.available() > 0) {
                byte b = (byte) resp.ins.read();
                if (b==0)
                    break;
                sb.append((char) b);
            }
        } catch (IOException ex) {
            System.out.println("ex: "+ex);
        }

        return sb.toString();
    }

    public static Orc makeOrc(String hostname)
    {
        while (true ) {
            try {
                return new Orc(Inet4Address.getByName(hostname));
            } catch (IOException ex) {
                System.out.println("Exception creating Orc: "+ex);
            }

            try {
                Thread.sleep(500);
            } catch (InterruptedException ex) {
            }
        }
    }

    public Orc(InetAddress inetaddr) throws IOException
    {
        this.orcAddr = inetaddr;
        this.sock = new DatagramSocket();
        this.reader = new ReaderThread();
        this.reader.setDaemon(true);
        this.reader.start();

        String version = getVersion();
        System.out.println("Connected to uorc with firmware "+version);

        if (!version.startsWith("v")) {
            System.out.println("Unrecognized firmware signature.");
            //System.exit(-1);
        }

        int vers = 0;
        int idx = 1;
        // the string is of the format vXXX-YYY-ZZZZ. Get the XXX
        // part. (YYY is # of commits past XXX, ZZZZ is the git hash.)
        while (idx < version.length() && Character.isDigit(version.charAt(idx))) {
            char c = version.charAt(idx);
            if (c=='-')
                break;
            vers = vers*10 + Character.digit(c, 10);
            idx++;
        }

        if (vers < 1) {
            System.out.println("Your firmware is too old.");
            //System.exit(-1);
        }
    }

    public void addListener(OrcListener ol)
    {
        listeners.add(ol);
    }

    public OrcResponse doCommand(int commandId, byte payload[])
    {
        while(true) {
            try {
                return doCommandEx(commandId, payload);
            } catch (IOException ex) {
                System.out.println("ERR: Orc ex: "+ex);
            }
        }
    }

    /** try a transaction once **/
    public OrcResponse doCommandEx(int commandId, byte payload[]) throws IOException
    {
        ByteArrayOutputStream bouts = new ByteArrayOutputStream();
        DataOutputStream outs = new DataOutputStream(bouts);
        OrcResponse response = new OrcResponse();

        // packet header
        outs.writeInt(0x0ced0002);
        int transactionId;
        synchronized(this) {
            transactionId = nextTransactionId++;
            transactionResponses.put(transactionId, response);
        }
        outs.writeInt(transactionId);
        outs.writeLong(System.nanoTime()/1000);

        // write command
        outs.writeInt(commandId);
        if (payload != null)
            outs.write(payload);

        // retransmit until we get a response
        boolean okay;
        byte p[] = bouts.toByteArray();
        DatagramPacket packet = new DatagramPacket(p, p.length, orcAddr, ORC_BASE_PORT + ((commandId>>24)&0xff));

        try {
            do {
                long starttime = System.nanoTime();

                sock.send(packet);
                okay = response.waitForResponse(50 + (int) (10.0 * meanRTT));
                if (!okay) {
                    if (verbose)
                        System.out.printf("Transaction timeout: xid=%8d, command=%08x, timeout=%.4f\n",
                                          transactionId, commandId, meanRTT);
                }

                long endtime = System.nanoTime();

                double rtt = (endtime - starttime) / 1000000000.0;

                double alpha = 0.995;
                meanRTT = alpha * meanRTT + (1-alpha)*rtt;
                meanRTT = Math.min(Math.max(meanRTT, MIN_TIMEOUT), MAX_TIMEOUT);

            } while (!okay);

            //	System.out.printf("RTT: %15f.3 ms, Mean: %15f\n", rtt*1000.0, meanRTT*1000.0);
            return response;

        } catch (IOException ex) {
            //
            throw ex;
        } finally {
            transactionResponses.remove(transactionId);
        }
    }

    class ReaderThread extends Thread
    {
        public void run()
        {
            while(true) {
                byte packetBuffer[] = new byte[1600];
                DatagramPacket packet = new DatagramPacket(packetBuffer, packetBuffer.length);
                try {
                    sock.receive(packet);

                    DataInputStream ins = new DataInputStream(new ByteArrayInputStream(packetBuffer, 0, packet.getLength()));
                    int signature = ins.readInt();
                    if (signature != 0x0ced0001) {
                        System.out.println("bad signature");
                        continue;
                    }

                    int transId = ins.readInt();
                    long utimeOrc = ins.readLong(); // time on orc
                    int  responseId = ins.readInt();

                    ts.update(System.currentTimeMillis()*1000, utimeOrc);
                    long utimeHost = toHostUtime(utimeOrc);

                    // wake any processes that might be waiting for this response.
                    OrcResponse sig;
                    synchronized(Orc.this) {
                        sig = transactionResponses.remove(transId);
                    }

                    if (sig != null) {
                        sig.ins = ins;
                        sig.responseBuffer = packetBuffer;
                        sig.responseBufferOffset = 20;
                        sig.responseBufferLength = packet.getLength();
                        sig.utimeOrc = utimeOrc;
                        sig.utimeHost = utimeHost;
                        sig.responseId = responseId;
                        sig.gotResponse();
                    } else {
                        if (verbose) {
                            System.out.println("Unexpected reply for transId: " +
                                               transId+" (last issued: " +
                                               (nextTransactionId-1)+")");
                        }
                    }

                } catch (IOException ex) {
                    System.out.println("Orc.ReaderThread Ex: "+ex);
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException ex2) {
                    }
                }
            }
        }
    }

    public OrcStatus getStatus()
    {
        while (true) {
            try {
                return new OrcStatus(this, doCommand(0x0001, null));
            } catch (IOException ex) {
            }
        }
    }

    /** Perform an I2C transaction. The transaction consists of up to
     * two phases, a write followed by a read.
     * @param addr The I2C address, [0,127]
     * @param writebuf A buffer containing data to be written. If
     * null, then the write phase will be skipped.
     * @param readlen The number of bytes to read after the write
     * phase. If zero, the read phase will be skipped.
     * @return The data from the I2C read phase, or null if readlen
     * was zero.
     **/
    public byte[] i2cTransaction(int addr, Object ...os)
    {
       	ByteArrayOutputStream bouts = new ByteArrayOutputStream();

        bouts.write((byte) addr);
        bouts.write((byte) 1); // 1 = 400khz mode, 0 = 100 khz

        // must have even number of parameters.
        assert((os.length & 1) == 0);

        // must have at least one set of parameters
        assert(os.length >= 2);
        int ntransactions = os.length / 2;

        for (int transaction = 0; transaction < ntransactions; transaction++) {
            byte writebuf[] = (byte[]) os[2*transaction + 0];
            int writebuflen = (writebuf == null) ? 0 : writebuf.length;
            int  readlen = (Integer) os[2*transaction + 1];

            bouts.write((byte) writebuflen);
            bouts.write((byte) readlen);

            for (int i = 0; i < writebuflen; i++)
                bouts.write(writebuf[i]);
        }

        OrcResponse resp = doCommand(0x5000, bouts.toByteArray());
        assert(resp.responded);

        ByteArrayOutputStream readData = new ByteArrayOutputStream();

        try {
            for (int transaction = 0; transaction < ntransactions; transaction++) {
                int error = resp.ins.readByte() & 0xff;

                if (error != 0) {
                    System.out.printf("Orc I2C error: code = %d\n", error);
                    //		    return null;
                }

                int actualreadlen = resp.ins.readByte() & 0xff;

                for (int i = 0; i < actualreadlen; i++)
                    readData.write((byte) resp.ins.readByte());
            }

            return readData.toByteArray();

        } catch (IOException ex) {
            return null;
        }
    }

    public int[] spiTransaction(int slaveClk, int spo, int sph, int nbits, int writebuf[])
    {
        slaveClk /= 1000; // convert to kHz.

        assert(nbits<=16);
        assert(spo==0 || spo==1);
        assert(sph==0 || sph==1);
        assert(writebuf.length <= 16);

        ByteArrayOutputStream bouts = new ByteArrayOutputStream();
        bouts.write((slaveClk>>8)&0xff);
        bouts.write(slaveClk&0xff);
        bouts.write(nbits | (spo<<6) | (sph<<7));
        bouts.write(writebuf.length);
        for (int i = 0; i < writebuf.length; i++) {
            bouts.write((writebuf[i]>>8)&0xff);
            bouts.write(writebuf[i]&0xff);
        }

        OrcResponse resp = doCommand(0x4000, bouts.toByteArray());
        assert(resp.responded);
        int rx[] = null;

        try {
            int status = resp.ins.readByte();
            assert(status==0);
            int nwords = resp.ins.readByte()&0xff;
            rx = new int[nwords];
            for (int i = 0; i < nwords; i++)
                rx[i] = resp.ins.readShort()&0xffff;
        } catch (IOException ex) {
            return null;
        }

        return rx;
    }
}
