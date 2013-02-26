package Rover;

import java.io.*;

import lcm.lcm.*;
import lcmtypes.*;

public class MotorSubscriber implements LCMSubscriber
{
    LCM lcm;
    float LEncoder;
    float REncoder;

    float LMotorCurrent;
    float RMotorCurrent;

    float LMotorAppliedVoltage;
    float RMotorAppliedVoltage;

    public MotorSubscriber() throws IOException
    {
        this.lcm = new LCM();
        this.lcm.subscribe("10_MOTOR_FEEDBACK", this);
    }

    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins)
    {

        try {
            if (channel.equals("10_MOTOR_FEEDBACK")) {
                motor_feedback_t msg = new motor_feedback_t(ins);

                LEncoder = msg.encoders[0];
                REncoder = msg.encoders[1];

                LMotorCurrent = msg.current[0];
                RMotorCurrent = msg.current[1];

                LMotorAppliedVoltage = msg.applied_voltage[0];
                RMotorAppliedVoltage = msg.applied_voltage[1];
            }
            //Sleep 20ms
	        Thread.sleep(20);

        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        } catch (InterruptedException e) {
            System.err.println(e.getMessage());
    	}
    }

    float getLEncoder(){
        return LEncoder;
    }

    float getREncoder(){
        return REncoder;
    }

    float getLMotorCurrent(){
        return LMotorCurrent;
    }

    float getRMotorCurrent(){
        return RMotorCurrent;
    }

    float getLMotorAppliedVoltage(){
        return LMotorAppliedVoltage;
    }

    float getRMotorAppliedVoltage(){
        return RMotorAppliedVoltage;
    }

}
