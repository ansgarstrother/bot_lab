package orc;

import java.io.*;
import java.util.*;

/** A response received from the uOrc hardware. Most users won't need
 * to use these directly, as helper classes like Motor handle this
 * automatically.
 **/
public final class OrcResponse
{
    public boolean responded = false;
    public int     transactionId;
    public long    utimeOrc;       // uorc time.
    public long    utimeHost;      // time (compatible with currentTimeMillis()*1000
    public int     responseId;

    public DataInputStream ins; // response can be read from here

    public byte    responseBuffer[];
    public int     responseBufferOffset;
    public int     responseBufferLength;

    public synchronized void gotResponse()
    {
        responded = true;
        notifyAll();
    }

    public synchronized boolean waitForResponse(int timeoutms)
    {
        if (responded)
            return true;

        try {
            wait(timeoutms);
        } catch (InterruptedException ex) {
        }

        return responded;
    }

    public void print()
    {
        for (int i = 0; i < responseBufferLength; i++) {
            if ((i%16)==0)
                System.out.printf("%04x: ", i);

            System.out.printf("%02x ", responseBuffer[responseBufferOffset+i]);

            if ((i%16)==15 || i == (responseBufferLength-1))
                System.out.printf("\n");
        }
    }
}
