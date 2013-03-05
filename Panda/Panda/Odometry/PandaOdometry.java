package Panda.Odometry;

import lcm.lcm.*;
import java.util.Vector;


import Panda.VisionMapping.*;
import Panda.Odometry.*;
import Panda.util.*;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;

import april.jcam.*;
import april.util.*;
import april.jmat.*;

public class RoverOdometry {

	//args
	private MotorSubscriber ms;
	private PIMUSubscriber ps;
	
	private motor_feedback_t prev_mf_msg;
	private pimu_t prev_pimu_msg;
	private motor_feedback_t cur_mf_msg;
	private pimu_t cur_pimu_msg;

	private boolean mf_flag;
	private boolean pimu_flag;
	private boolean initialize;
	
	private pos_t prev_pos_msg;
	private pos_t cur_pos_msg;


	// CONSTRUCTOR METHOD
	public RoverOdometry(MotorSubscriber ms, PIMUSubscriber ps) {
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
		initialize = true;


		// RUN ROBOT
		// while loop terminates when signal thrown saying robot is done
		while (true) {
			// sample msg, look for update
			// if a message has updated, set flag
			// calculate pose when pimu and motor feedback have both been sent

			// sample messages
			cur_mf_msg = ms.getMessage();
			cur_pimu_msg = ps.getMessage();
			// if no prev msg has been set, initialize system
			if (initialize) {
				prev_mf_msg = cur_mf_msg;
				prev_pimu_msg = cur_pimu_msg;
				initialize = false;
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
		double gyro1 = (cur_pimu_msg.integrator[4] - prev_pimu_msg.integrator[4]) / 
							(cur_pimu_msg.utime_pimu - prev_pimu_msg.utime_pimu);
		double gyro2 = (cur_pimu_msg.integrator[5] - prev_pimu_msg.integrator[5]) / 
							(cur_pimu_msg.utime_pimu - prev_pimu_msg.utime_pimu);
		double encoL = (cur_mf_msg.encoders[0] - prev_mf_msg.encoders[0]) /
							(cur_mf_msg.utime - prev_mg_msg.utime);
		double encoR = (cur_mf_msg.encoders[1] - prev_mf_msg.encoders[1]) /
							(cur_mf_msg.utime - prev_mg_msg.utime);

	}

	protected void sendPose() {
		LCM lcm = LCM.getSingleton();
                        
		//publish
		lcm.publish("10_POSE", cur_pos_msg);

		// send cur_pos to prev_pos
		prev_pos_msg = cur_pos_msg;
	
	}



}
