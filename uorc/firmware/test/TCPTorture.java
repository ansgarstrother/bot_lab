import java.io.*;
import java.net.*;
import java.util.*;

public class TCPTorture
{
    public static void main(String args[])
    {
	new TCPTorture(args);
    }

    int nthreads = 1;
    InetAddress dstAddr;
    int dstPort = 53771;

    public TCPTorture(String args[])
    {
	try {
	    dstAddr = InetAddress.getByName("192.168.237.7");
	    
	    for (int i = 0; i < nthreads; i++)
		new TCPTortureThread().start();
	} catch (IOException ex) {
	    System.out.println("ex: "+ex);
	}
    }

    class TCPTortureThread extends Thread
    {
	public void run()
	{
	    Random r = new Random();

	    long starttime = System.currentTimeMillis();
	    long totalbytes = 0;

	    while (true) {
		
		try {
		    Socket s = new Socket(dstAddr, dstPort);
		    s.setTcpNoDelay(true);
		    DataInputStream ins = new DataInputStream(s.getInputStream());
		    DataOutputStream outs = new DataOutputStream(s.getOutputStream());

		    for (int iter = 0; iter < 100000; iter++) {
			int len = r.nextInt(1024) + 1;

			double dt = (System.currentTimeMillis() - starttime) / 1000.0;
			System.out.printf("%10d %10d %15f kB/s\n", iter, len, (totalbytes / dt)/1024.0);

			byte outbuf[] = new byte[2 + len];
			outbuf[0] = (byte) (len&0xff);
			outbuf[1] = (byte) ((len>>8)&0xff);

			if (true) {
			    // random bytes a better test
			    for (int i = 2; i < outbuf.length; i++)
				outbuf[i] = (byte) r.nextInt(255);
			} else {
			    // all the same for each transmission: easier to debug
			    for (int i = 2; i < outbuf.length; i++)
				outbuf[i] = (byte) iter;
			}
			outs.write(outbuf);

			byte inbuf[] = new byte[len];
			
			ins.readFully(inbuf);

			boolean bad = false;
			for (int i = 2; i < outbuf.length; i++)
			    bad = bad | (inbuf[i-2]!=outbuf[i]);

			if (bad) {
			    for (int i = 0; i < len; i++)
				System.out.printf("%04x: %02x %02x %s\n", i, inbuf[i], outbuf[i+2], inbuf[i]==outbuf[i+2] ? "" : "!");
			    System.exit(0);
			}				

			totalbytes += 2*len + 2;
		    }

		    s.close();
		} catch (IOException ex) {
		    System.out.println("Ex: "+ex);
		}
	    }
	}
    }
}
