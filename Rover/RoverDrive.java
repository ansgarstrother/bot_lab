package Rover;

import lcmtypes.*;
import lcm.lcm.*;
import april.util.TimeUtil;
import Rover.sensors.MotorSubscriber;

public class RoverDrive
{
    static final boolean DEBUG = true;

    static final float KP = 0.1;
    static final float STOP = 0.0;
    static final float MAX_SPEED = 0.5;
    static final float MIN_SPEED = -0.5;
    static final float REG_SPEED = 0.4;
    
    LCM lcm;
    MotorSubscriber ms;

    diff_drive_t msg;
    
    float leftSpeed;
    float rightSpeed;
    
    public RoverDrive(){
        ms = new MotorSubscriber();
        
        try{
			System.out.println("Hello World!");

			// Get an LCM Object
            lcm = LCM.getSingleton();
            
			// Create a diff_drive message
			msg = new diff_drive_t();
			
			// Set motors enabled
			// False means Enabled (lcm weirdness)
			msg.left_enabled = false;
			msg.right_enabled = false;

            leftSpeed = REG_SPEED * -1;
            rightSpeed = REG_SPEED;
		}
		// Thread.sleep throws things
		// Java forces you to be ready to catch them.
		catch(Throwable t) {
			System.out.println("Error: Exception thrown");
		}
    }

    public void Stop(){
        // Add Timestamp (lcm really cares about this)
        msg.utime = TimeUtil.utime();
        msg.left = STOP;
        msg.right = STOP;
        lcm.publish("DIFF_DRIVE", msg);
    }
    
    public void driveForward(){
        //Needs to be called in a loop
        //Left encoder: 128.27 ticks/inch
        //Right encoder: 124.571 ticks/inch
        float KError = KP*(ms.getREncoder() - ms.getLEncoder());
        leftSpeed = speedCheck(leftSpeed + KError);
        rightSpeed = speedCheck(rightSpeed + KError);

        // Add Timestamp (lcm really cares about this)
        msg.utime = TimeUtil.utime();
        msg.left = leftSpeed;
        msg.right = rightSpeed;

        lcm.publish("DIFF_DRIVE", msg);
    }
    
    private float speedCheck(float speed){
        //If speed is greater then max
        if (speed > MAX_SPEED)
            return MAX_SPEED;
        if (speed < MIN_SPEED)
            return MIN_SPEED;
        return speed;
    }
    
}
