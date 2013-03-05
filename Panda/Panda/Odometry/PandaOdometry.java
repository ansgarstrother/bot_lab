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

	//args
	private MotorSubscriber ms;
	private PIMUSubscriber ps;
	
	private motor_feedback_t prev_mf_msg;
	private pimu_t prev_pimu_msg;
	private motor_feedback_t cur_mf_msg;
	private pimu_t cur_pimu_msg;

	private boolean mf_flag;
	private boolean pimu_flag;
	private boolean initialize_mf;
	private boolean initialize_pimu;
	
	private pos_t prev_pos_msg;
	private pos_t cur_pos_msg;


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
		initialize_mf = true;
		initialize_pimu = true;


		// RUN ROBOT
		// while loop terminates when signal thrown saying robot is done
		while (true) {
			// sample msg, look for update
			// if a message has updated, set flag
			// calculate pose when pimu and motor feedback have both been updated

			// sample messages
			cur_mf_msg = ms.getMessage();
			cur_pimu_msg = ps.getMessage();
			// if no prev msg has been set, initialize system
			if (initialize_mf && prev_mf_msg != cur_mf_msg) {
				prev_mf_msg = cur_mf_msg;
				initialize_mf = false;
			}
			if (initialize_pimu && prev_pimu_msg != cur_pimu_msg) {
				prev_pimu_msg = cur_pimu_msg;
				initialize_pimu = false;
			}
			// test motor feedback
			if (prev_mf_msg != cur_mf_msg) {
				mf_flag = true;
			}
			// test pimu msg
			if (prev_pimu_msg != cur_pimu_msg) {
				pimu_flag = true;
			}
			// attempt to push message over lcm
			if (pimu_flag && mf_flag) {
				// CALCULATE POSE
				calculatePose();
				// PUBLISH TO GUI
				sendPose();
				// bump cur msg to prev msg
				prev_mf_msg = cur_mf_msg;
				prev_pimu_msg = cur_pimu_msg;
				// reset flags
				pimu_flag = mf_flag = false;
			}
			
		}
	}


	// PROTECTED METHODS
	protected void calculatePose() {
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
		double gyro1 = (cur_pimu_msg.integrator[4] - prev_pimu_msg.integrator[4]) / 
							(cur_pimu_msg.utime_pimu - prev_pimu_msg.utime_pimu);
		double gyro2 = (cur_pimu_msg.integrator[5] - prev_pimu_msg.integrator[5]) / 
							(cur_pimu_msg.utime_pimu - prev_pimu_msg.utime_pimu);

		double encoL = (cur_mf_msg.encoders[0] - prev_mf_msg.encoders[0]);
		double encoR = (cur_mf_msg.encoders[1] - prev_mf_msg.encoders[1]);
		double dL = encoL / left_ticks_per_meter;
		double dR = encoR / right_ticks_per_meter;


		// setup pos msg
		// TODO: TRIANGLE FOUND AND FINISHED FLAGS MUST BE PASSED IN
    	cur_pos_msg.triangle_found = false;
    	cur_pos_msg.finished = false;
    	cur_pos_msg.timestamp = cur_mf_msg.utime;
		cur_pos_msg.delta_x = (float)((dL + dR) / 2);
		cur_pos_msg.theta = (dR - dL) / base_distance; // in radians

	}

	protected void sendPose() {
		LCM lcm = LCM.getSingleton();
                        
		//publish
		lcm.publish("10_POSE", cur_pos_msg);

		// send cur_pos to prev_pos
		prev_pos_msg = cur_pos_msg;
	
	}



}
