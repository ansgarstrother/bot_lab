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
    static final float TPM_RIGHT = 5050.003F;
    static final float TPM_LEFT = 494904.3733F;

    //Left encoder: 128.27 ticks/inch
    //Right encoder: 124.571 ticks/inch
	static final float CORRECTION = 1.0F;

    //Gyro Constants
    static final float CALIBRATION_TIME = 300; //Approx 3 sec
    static final double SENSITIVITY_CONSTANT = 1.0; //This needs to be calculated

	LCM lcm;

	MotorSubscriber ms;
	PIMUSubscriber ps;

    diff_drive_t msg;

    float leftSpeed;
    float rightSpeed;

    private double gyroOffset;
    private double gyroAngle;

    private long prevTimeInMilli;

    public PandaDrive(){
        try{
			System.out.println("Hello World!");

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
		// Thread.sleep throws things
		// Java forces you to be ready to catch them.
		catch(Throwable t) {
			System.out.println("Error: Exception thrown");
		}
    }

    private void initGyro(){
        System.out.println("Calibrating Gyro..");
        int counter = 0;
        ArrayList<Double> data = new ArrayList<Double>();

        //This loop will take 100 gyro data points
        while(counter < CALIBRATION_TIME){
            pimu_t pimuData = ps.getMessage();
            //TODO: Add gyro integrator value to data ArrayList here
            double curGyroIntegrator = 0;
            data.add(curGyroIntegrator);
            try{ Thread.sleep(10); }
            catch(Exception e){};
            counter++;
        }

        double temp = 0;
        for(double dataPoint : data){
            temp += dataPoint;
        }

        gyroOffset = temp / (CALIBRATION_TIME / 10);
        System.out.println("Finished Gyro Calibration");

        //Note: You could improve this offset calibration and limit
        //drift using this: New offset = (samplesize – 1) * old offset + (1 / samplesize) * gyro
        //every time the robot is stopped
    }

    public double getGyroAngle(){
        //This should be called continuously in short time intervals to minimize error

        pimu_t pimuData = ps.getMessage();
        //TODO: Get current gyro value
        double curGyroIntegrator = 0;

        //Final units of gyroRate should be degrees per second
        double gyroRate = (curGyroIntegrator - gyroOffset) / SENSITIVITY_CONSTANT;
        gyroAngle += gyroRate * (System.currentTimeMillis() - prevTimeInMilli) / 1000;

        prevTimeInMilli = System.currentTimeMillis();
        return gyroAngle;

        //Note: Can also integrate the accelerometer and use a Kalman filter to
        //      minimize drift
    }

    public void Stop(){
        // Add Timestamp (lcm really cares about this)
        msg.utime = TimeUtil.utime();
        msg.left = STOP;
        msg.right = STOP;
        lcm.publish("DIFF_DRIVE", msg);
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


        double distanceTraveled = 0;
		int lastLeftEncoder = 0, lastRightEncoder = 0;
		int curLeftEncoder = 0, curRightEncoder = 0;
		float leftDistance = 0, rightDistance = 0;
		double[] pimuDerivs = new double[2];

        motor_feedback_t motorFeedback = ms.getMessage();
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

            lcm.publish("DIFF_DRIVE", msg);

		while (distanceTraveled < distance) {

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

            float KError = KP*( curRightEncoder - curLeftEncoder);
            leftSpeed = speedCheck(leftSpeed + KError);
            rightSpeed = speedCheck(rightSpeed - KError);

            //TODO: Fix below
			//pimuDerivs = ps.getMessage();
			//System.out.printf ("gyro derivs " + pimuDerivs[0] + " "  + pimuDerivs[1]);
			/*
			if (pimuDerivs[0] > 3) {

			}
			*/

			msg.utime = TimeUtil.utime();
			msg.left = leftSpeed;
			msg.right = rightSpeed;

			lcm.publish("DIFF_DRIVE", msg);

			System.out.println ("current encoders " + curLeftEncoder + " " + curRightEncoder);


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
