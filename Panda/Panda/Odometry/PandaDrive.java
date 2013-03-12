package Panda.Odometry;

import lcmtypes.*;
import lcm.lcm.*;
import april.util.TimeUtil;
import Panda.sensors.*;

public class PandaDrive
{
    static final boolean DEBUG = true;

    //Speed Constants
    static final float KP = 0.0015F;
    static final float STOP = 0.0F;
    static final float MAX_SPEED = 1.0F;
    static final float MIN_SPEED = 0.0F;
    static final float REG_SPEED = 0.7F;
    static final float FASTER_SPEED = 0.85F;
    static final float TPM_RIGHT = 4904.373F;
    static final float TPM_LEFT = 5050.003F;

    //Left encoder: 128.27 ticks/inch
    //Right encoder: 124.571 ticks/inch
	static final float CORRECTION = 1.0F;

	LCM lcm;

	MotorSubscriber ms;
    Gyro gyro;

    diff_drive_t msg;

    float leftSpeed;
    float rightSpeed;

    public PandaDrive(){
        try{
			System.out.println("Hello World!");

            ms = new MotorSubscriber();
            gyro = new Gyro();
			// Get an LCM Object
            lcm = LCM.getSingleton();

			msg = new diff_drive_t();

			// Set motors enabled
			// False means Enabled (lcm weirdness)
			msg.left_enabled = false;
			msg.right_enabled = false;
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
        lcm.publish("10_DIFF_DRIVE", msg);
    }

	public void turn(double angle) {
		// gyro derivatives are positive

		double angle_turned = 0;

		while (angle_turned < angle) {
				msg.utime = TimeUtil.utime();

				// right turn if angle is negative
				if (angle <=0 ) {
					msg.left = 0.25F;
					msg.right = -0.25F;

				}
				// left turn
				else {
					msg.left = -0.25F;
					msg.right = 0.25F;

				}

				lcm.publish ("10_DIFF_DRIVE", msg);


		}

	}

	public void turnRight() {
		// gyro derivatives are negative

		msg.utime = TimeUtil.utime();
		msg.left = 0.25F;
		msg.right = -0.25F;
		lcm.publish ("10_DIFF_DRIVE", msg);


	}


    public void driveForward (double distance) {
        //Needs to be called in a loop
        //Left encoder: 128.27 ticks/inch
        //Right encoder: 124.571 ticks/inch
        double desiredHeading = gyro.getGyroAngle();

        double distanceTraveled = 0;
		int lastLeftEncoder = 0, lastRightEncoder = 0;
		int curLeftEncoder = 0, curRightEncoder = 0;
		float leftDistance = 0, rightDistance = 0;
		double[] pimuDerivs = new double[2];


        motor_feedback_t motorFeedback = new motor_feedback_t();
        motorFeedback = ms.getMessage();
        int initLeftEncoder = motorFeedback.encoders[0];
        int initRightEncoder = motorFeedback.encoders[1];
		lastLeftEncoder = 0;
		lastRightEncoder = 0;
		System.out.println ("last encoders " + lastLeftEncoder + " " + lastRightEncoder);


		boolean update = true;

            leftSpeed = REG_SPEED ;
            rightSpeed = REG_SPEED;
            msg.utime = TimeUtil.utime();
            msg.left = leftSpeed;
            msg.right = rightSpeed;

            lcm.publish("10_DIFF_DRIVE", msg);

		while (distanceTraveled < distance - .01) {
            // get updated encoder data
			while(update){
				motorFeedback = ms.getMessage();
            	curLeftEncoder = motorFeedback.encoders[0] - initLeftEncoder;
            	curRightEncoder = motorFeedback.encoders[1] - initRightEncoder;


				if(lastLeftEncoder != curLeftEncoder || lastRightEncoder != curRightEncoder){
					update = false;
				}

				try{Thread.sleep(1);}
				catch(Exception e){}
			}
			update = true;


            leftSpeed = REG_SPEED ;
            rightSpeed = REG_SPEED;

            //TODO: Tweak this
            double curHeading = gyro.getGyroAngle();
            float Error = (float)(curHeading - desiredHeading);
            leftSpeed = leftSpeed + (KP * Error);
            rightSpeed = rightSpeed + (KP * Error);

			msg.utime = TimeUtil.utime();
			msg.left = leftSpeed;
			msg.right = rightSpeed;

			lcm.publish("10_DIFF_DRIVE", msg);

			System.out.println ("current encoders " + curLeftEncoder + " " + curRightEncoder);

            // distance traveled is
			leftDistance = (curLeftEncoder - lastLeftEncoder) / TPM_LEFT;
			rightDistance = (curRightEncoder - lastRightEncoder) / TPM_RIGHT;
			distanceTraveled += (leftDistance + rightDistance) / 2;

			lastLeftEncoder = curLeftEncoder;
			lastRightEncoder = curRightEncoder;

			if (DEBUG){
				//System.out.println("Error: " + KError);
				System.out.println("Left Speed: " + leftSpeed +"    Right Speed: " + rightSpeed);
				//System.out.println ("delta distances " + leftDistance + " " + rightDistance);
				//System.out.println ("distance traveled " + distanceTraveled);
			}
		}

        boolean botStillMoving = true;
        while (botStillMoving){
            Stop();
            motorFeedback = ms.getMessage();
            curLeftEncoder = motorFeedback.encoders[0];

            if (lastLeftEncoder > curLeftEncoder){
                lastLeftEncoder = curLeftEncoder;
            }
            else{
                botStillMoving = false;
            }
        }


    }


    private float speedCheck (float speed){
        //If speed is greater then max
        if (speed > MAX_SPEED)
            return MAX_SPEED;
        if (speed < MIN_SPEED)
            return MIN_SPEED;
        return speed;
    }

}
