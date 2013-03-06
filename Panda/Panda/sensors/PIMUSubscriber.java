package Panda.sensors;

import java.io.*;
import lcm.lcm.*;
import java.lang.Math;
import lcmtypes.*;

public class PIMUSubscriber implements LCMSubscriber
{
    LCM lcm;

	//pimu_t variables
   	pimu_t msg;


    public PIMUSubscriber() throws IOException
    {
        this.lcm = new LCM();
        //this.lcm.subscribe("10_POSE", this);
        //this.lcm.subscribe("10_MOTOR_FEEDBACK", this);
        this.lcm.subscribe("PIMU", this);
		this.msg = new pimu_t();
    }

    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins)
    {
		//System.out.println("Received message on channel " + channel);
		try {
			if (channel.equals ("PIMU")) {
                this.msg = new pimu_t(ins);
            }
            
        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        }
	}

	public pimu_t getMessage() {
		return msg;

	}

}
