package Panda;

import lcmtypes.*;
import lcm.lcm.*;
import april.util.TimeUtil;
import sensors.*;

public class PandaDrive
{
    static final boolean DEBUG = true;

    static final float KP = 0.1F;
    static final float STOP = 0.0F;
    static final float MAX_SPEED = 0.5F;
    static final float MIN_SPEED = -0.5F;
    static final float REG_SPEED = 0.4F;
    
    LCM lcm;
    MotorSubscriber ms;

    diff_drive_t msg;
    
    float leftSpeed;
    float rightSpeed;
    
    public PandaDrive(){
        
        try{
			System.out.println("Hello World!");
			
            ms = new MotorSubscriber();

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
        
        if (DEBUG){
            System.out.print("Left wheel speed: ");
            System.out.print(leftSpeed);
            System.out.print("   Right wheel speed: ");
            System.out.println(rightSpeed);
        }
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
