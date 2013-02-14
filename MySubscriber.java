import java.io.*;

import lcm.lcm.*;
import lcmtypes.*;

public class MySubscriber implements LCMSubscriber
{
    LCM lcm;

    public MySubscriber()
        throws IOException
    {
        this.lcm = new LCM();
        this.lcm.subscribe("10_POSE", this);
    }

    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins)
    {
        System.out.println("Received message on channel " + channel);

        try {
            if (channel.equals("10_POSE")) {
                pos_t msg = new pos_t(ins);

                System.out.println("  timestamp    = " + msg.timestamp);
                System.out.println("  x coord     = [ " + msg.x_pos + " ]");
                System.out.println("  y coord  = [ " + msg.y_pos + " ]");
		System.out.println("  theta  = [ " + msg.theta + " ]");
            }

        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        }
    }

    public static void main(String args[])
    {
        try {
            MySubscriber m = new MySubscriber();
            while(true) {
                Thread.sleep(1000);
            }
        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        } catch (InterruptedException ex) { }
    }
}
