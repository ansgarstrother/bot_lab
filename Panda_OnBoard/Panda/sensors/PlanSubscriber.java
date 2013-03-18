package Panda.sensors;


import java.io.*;
import lcm.lcm.*;
import java.lang.Math;
import lcmtypes.*;

public class PlanSubscriber implements LCMSubscriber
{
    LCM lcm;

    // pos_t variables
    plan_t msg;


    
    public PlanSubscriber()
    throws IOException
    {
        this.lcm = new LCM("udpm://239.255.76.10:7667?ttl=1");
        this.lcm.subscribe("10_PLAN", this);
		this.msg = new plan_t();
    }
    
    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins)
    {   
	System.out.println("Received message on channel " + channel);

        try {
            if (channel.equals("10_PLAN")) {
                this.msg = new plan_t(ins);
            }
            
        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        }
    }

    public plan_t getMessage() {
    	return msg;
    }
}


