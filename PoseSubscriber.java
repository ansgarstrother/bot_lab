import java.io.*;

import lcm.lcm.*;
import lcmtypes.*;

public class PoseSubscriber implements LCMSubscriber
{
    LCM lcm;
    
    public PoseSubscriber()
    throws IOException
    {
        this.lcm = new LCM();
        this.lcm.subscribe("10_POSE", this);
    }
    
    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins)
    {
        System.out.println("Received message on channel " + channel);
        if (channel.equals("10_POSE")) {
			try {
            	pos_t msg = new pos_t(ins);
                
            	System.out.println("  timestamp    = " + msg.timestamp);
            	System.out.println("  delta x     = [ " + msg.delta_x + " ]");
            	System.out.println("  theta  = [ " + msg.theta + " ]");
			} catch (Exception e) {
				System.err.println("Error: " + e.getMessage());
			}
        }
    }
    
    public static void main(String args[])
    {
        try {
            PoseSubscriber m = new PoseSubscriber();
            while(true) {
                Thread.sleep(1000);
            }
        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        } catch (InterruptedException ex) { }
    }
}
