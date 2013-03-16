package Panda.Odometry;

import lcmtypes.*;
import lcm.lcm.*;
import april.util.TimeUtil;
import Panda.sensors.*;
import java.util.ArrayList;

public class PandaDrive
{
    static final boolean DEBUG = true;

    //Speed Constants
    static final float KP = 0.0015F;
    static final float STOP = 0.0F;
    static final float MAX_SPEED = 1.0F;
    static final float MIN_SPEED = 0.0F;
    static final float REG_SPEED = 0.7F;
    static final float TPM = 4900F;
	static final float STRAIGHT_GYRO_THRESH = 7.0F;
	static final float ANGLE = 7.8F;
    static final float TURN_SPEED = 0.4F;
    static final float TURN_RANGE = 0.5F;
	LCM lcm;

	MotorSubscriber ms;
	PIMUSubscriber ps;
    Gyro gyro;

    diff_drive_t msg;

    float leftSpeed;
    float rightSpeed;

    private double gyroOffset;
    private double gyroAngle;

    private long prevTimeInMilli;

    public PandaDrive(){
        try{
			ms = new MotorSubscriber();
			ps = new PIMUSubscriber();
			// Get an LCM Object
            lcm = LCM.getSingleton();

			// CrestLeftEncoder = curLeftEncoder;


			msg = new diff_drive_t();

			// Set motors enabled
			// False means Enabled (lcm weirdness)
			msg.left_enabled = false;
			msg.right_enabled = false;
		}
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


	public void turn(float angle) {
		// gyro derivatives are positive


		double angled_turned = 0;

		int curRight = 0;
		int curLeft = 0;

        motor_feedback_t motorFeedback;
        motorFeedback = new motor_feedback_t();

        motorFeedback = ms.getMessage();

		int initLeft = motorFeedback.encoders[0];
        int initRight = motorFeedback.encoders[1];

		boolean neg = false;
		if( angle < 0F){
			neg = true;
			angle = -angle;
		}

		while(angled_turned < angle) {
				msg.utime = TimeUtil.utime();

				System.out.println("angle " + angle);
				// right turn if angle is negative
				if (neg) {
					System.out.println("here");
					msg.left = 0.3F;
					msg.right = -0.3F;
				}
				else {
					System.out.println("no here");
					msg.left = -0.3F;
					msg.right = 0.3F;
				}

				lcm.publish ("10_DIFF_DRIVE", msg);

				motorFeedback = ms.getMessage();
        		curLeft = motorFeedback.encoders[0];
    	    	curRight = motorFeedback.encoders[1];

				angled_turned = (initRight - curRight)/ANGLE;

				if(angled_turned < 0)
					angled_turned *= -1;

		}

		Stop();
	}

    public void gyroTurn(double angle, float K){
        leftSpeed = TURN_SPEED;
        rightSpeed = TURN_SPEED;

        boolean outsideRange = true;
        while (outsideRange){
            double curAngle = gyro.getGyroAngle();
            if ((curAngle > (angle - TURN_RANGE)) &&
                (curAngle < (angle + TURN_RANGE))){
                outsideRange = false;
            }
            else{
                float KERROR = K*(float)(angle - curAngle);
                msg.left = speedCheck(leftSpeed - KERROR);
                msg.right = speedCheck(rightSpeed - KERROR);
                msg.utime = TimeUtil.utime();
                lcm.publish("10_DIFF_DRIVE", msg);
            }
        }
        //Make sure one of the stop messages gets through
        for (int i = 0; i < 50; i++){
            Stop();
        }
    }


    public void driveForward (float distance) {

		float distanceTraveled = 0;
		int curLeftEncoder = 0, curRightEncoder = 0;
		float leftDistance = 0, rightDistance = 0;
		float[] pimuDerivs = new float[2];


        motor_feedback_t motorFeedback;
		motorFeedback = new motor_feedback_t();

        motorFeedback = ms.getMessage();

        int initLeftEncoder = motorFeedback.encoders[0];
        int initRightEncoder = motorFeedback.encoders[1];

		while (distanceTraveled < distance - .05) {
            // get updated encoder data

			motorFeedback = ms.getMessage();

          	curLeftEncoder = motorFeedback.encoders[0] - initLeftEncoder;
           	curRightEncoder = motorFeedback.encoders[1] - initRightEncoder;

            leftSpeed = REG_SPEED ;
            rightSpeed = REG_SPEED;

            float KError = KP*( curRightEncoder - curLeftEncoder);
			System.out.println("Right " + curRightEncoder + "    " + curLeftEncoder);
            System.out.println(KError + "  " + KP + "     " + (curRightEncoder - curLeftEncoder));
			leftSpeed = speedCheck(leftSpeed + KError);
            rightSpeed = speedCheck(rightSpeed - KError);


			msg.utime = TimeUtil.utime();
			msg.left = leftSpeed;
			msg.right = rightSpeed;

			lcm.publish("10_DIFF_DRIVE", msg);

			System.out.println ("current encoders " + leftSpeed + " " + rightSpeed);

            // distance traveled
			distanceTraveled = (curLeftEncoder + curRightEncoder) / (2 * TPM);

		}
		Stop();
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
