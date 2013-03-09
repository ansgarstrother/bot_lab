package Panda.Odometry;

import lcm.lcm.*;
import java.util.Vector;

import lcmtypes.*;
import Panda.VisionMapping.*;
import Panda.Odometry.*;
import Panda.util.*;
import Panda.sensors.*;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;

import april.jcam.*;
import april.util.*;
import april.jmat.*;

public class PandaOdometry {

	// const
	private static final double left_ticks_per_meter = 5050;	// left wheel ticks per meter
	private static final double right_ticks_per_meter = 4904;	// right wheel ticks per meter
	private static final double wheel_radius = 0.0381;		// wheel radius (meters)
	private static final double base_distance = 0.168335;	//distance between wheels (meters)
	private static final int interval = 0;

	//args
	private MotorSubscriber ms;
	private PIMUSubscriber ps;
	
	private volatile motor_feedback_t prev_mf_msg;
	private volatile pimu_t prev_pimu_msg;
	private volatile motor_feedback_t cur_mf_msg;
	private volatile pimu_t cur_pimu_msg;

	private boolean mf_flag;
	private boolean pimu_flag;
	private boolean initialized;
	
	private volatile pos_t prev_pos_msg;
	private volatile pos_t cur_pos_msg;

	private long previousTimeMillis;


	// CONSTRUCTOR METHOD
	public PandaOdometry(MotorSubscriber ms, PIMUSubscriber ps) {
		// initialize subscribers
		this.ms = ms;
		this.ps = ps;
		// initialize messages
		this.prev_mf_msg = new motor_feedback_t();
		this.prev_pimu_msg = new pimu_t();
		this.prev_pos_msg = new pos_t();
		this.cur_mf_msg = new motor_feedback_t();
		this.cur_pimu_msg = new pimu_t();
		this.cur_pos_msg = new pos_t();

		// initialize flags
		mf_flag = false;
		pimu_flag = false;
		initialized = false;

		// initialize time
		previousTimeMillis = System.currentTimeMillis();


		// RUN ROBOT
		// while loop terminates when signal thrown saying robot is done
		while (true) {
			// sample msg, look for update
			// if a message has updated, set flag
			// calculate pose when pimu and motor feedback have both been updated

			// sample messages
			cur_mf_msg = ms.getMessage();
			cur_pimu_msg = ps.getMessage();
			if (!initialized && cur_mf_msg.utime != 0 && cur_pimu_msg.utime != 0) {
				prev_mf_msg = cur_mf_msg;
				prev_pimu_msg = cur_pimu_msg;
				System.out.println(cur_mf_msg.utime);
				//System.out.println(prev_mf_msg.encoders[0]);
				initialized = true;
			}
			// test motor feedback
			if (prev_mf_msg != cur_mf_msg && initialized) {
				//System.out.println("Different Motor Messages");
				mf_flag = true;
			}
			// test pimu msg
			if (prev_pimu_msg != cur_pimu_msg && initialized) {
				//System.out.println("Different PIMU Messages");
				pimu_flag = true;
			}
			// attempt to push message over lcm
			if (pimu_flag && mf_flag && (previousTimeMillis + interval <= System.currentTimeMillis())) {
				// CALCULATE POSE
				calculatePose(cur_mf_msg, cur_pimu_msg, prev_pimu_msg, prev_mf_msg);
				// PUBLISH TO GUI
				sendPose();	
				// bump cur msg to prev msg
				prev_mf_msg = cur_mf_msg;
				prev_pimu_msg = cur_pimu_msg;
				// reset flags
				pimu_flag = mf_flag = false;
				// reset time
				previousTimeMillis = System.currentTimeMillis();
			}
			
		}
	}


	// PROTECTED METHODS
	private void calculatePose(motor_feedback_t mf, pimu_t pimu, pimu_t prev_pimu, motor_feedback_t prev_mf) {
		// RETRIEVE ALL DATA FROM LCM MESSAGES
		// PIMU
		//	- gyro derivative data (integrator[4] & integrator[5] & utime_pimu)
		// MOTOR FEEDBACK
		//	- encoder derivative data (encoder[0] & encoder[1])
		//		a. theta = (right wheel distance - left wheel distance) / base distance
		//		b. axis distances:	dL = arL*theta (may not be needed)
		//							dR = arR*theta (may not be needed)
		//							arC = (arL + arR) / 2 (may not be needed)
		//		c. delta x = (dR + dL) / 2

		// TODO: GYRO SENSOR IMPLEMENTATION
		double gyro1 = (pimu.integrator[4] - pimu.integrator[4]) / 
							(pimu.utime_pimu - prev_pimu.utime_pimu);
		double gyro2 = (pimu.integrator[5] - pimu.integrator[5]) / 
							(pimu.utime_pimu - prev_pimu.utime_pimu);

		double encoL = (mf.encoders[0] - prev_mf.encoders[0]);
		double encoR = (mf.encoders[1] - prev_mf.encoders[1]);
		double dL = encoL * left_ticks_per_meter;
		double dR = encoR * right_ticks_per_meter;

		// setup pos msg
		// TODO: TRIANGLE FOUND AND FINISHED FLAGS MUST BE PASSED IN
    	cur_pos_msg.triangle_found = false;
    	cur_pos_msg.finished = false;
    	cur_pos_msg.timestamp = mf.utime;
		cur_pos_msg.delta_x = (float)((dL + dR) / 2);
		cur_pos_msg.theta = (dR - dL) / base_distance; // in radians

	}

	protected void sendPose() {
		LCM lcm = LCM.getSingleton();
                        
		//publish
		if (cur_pos_msg.delta_x != 0 && cur_pos_msg.theta != 0) {
			lcm.publish("10_POSE", cur_pos_msg);
		}

		// send cur_pos to prev_pos
		prev_pos_msg = cur_pos_msg;
	
	}



}
