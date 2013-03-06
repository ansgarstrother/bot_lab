package Panda.sensors;

import java.io.*;
import lcm.lcm.*;
import java.lang.Math;
import lcmtypes.*;

public class MotorSubscriber implements LCMSubscriber
{
    LCM lcm;

	//pimu_t variables
   	motor_feedback_t msg;


    public MotorSubscriber() throws IOException
    {
        this.lcm = new LCM();
        //this.lcm.subscribe("10_POSE", this);
        this.lcm.subscribe("MOTOR_FEEDBACK", this);
        //this.lcm.subscribe("10_PIMU", this);
		this.msg = new motor_feedback_t();
    }

    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins)
    {
		//System.out.println("Received message on channel " + channel);
		try {
			if (channel.equals ("MOTOR_FEEDBACK")) {
                this.msg = new motor_feedback_t(ins);
            }
            
        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        }
	}

	public motor_feedback_t getMessage() {
		return msg;

	}

}
