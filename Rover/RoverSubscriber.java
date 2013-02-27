package Rover;

import java.io.*;
import lcm.lcm.*;
import java.lang.Math;
import lcmtypes.*;

public class RoverSubscriber implements LCMSubscriber
{
    LCM lcm;

    // pos_t variables
    pos_t msg;


    
    public RoverSubscriber()
    throws IOException
    {
        this.lcm = new LCM();
        this.lcm.subscribe("10_POSE", this);
        this.lcm.subscribe("10_MOTOR_FEEDBACK", this);
        this.lcm.subscribe("10_PIMU", this);
	this.msg = new pos_t();
    }
    
    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins)
    {   
        try {
            if (channel.equals("10_POSE")) {
                this.msg = new pos_t(ins);
            }
            
        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        }
    }

    public pos_t getPose() {
    	return msg;
    }
}

