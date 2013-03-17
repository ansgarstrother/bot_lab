package Rover;


import lcmtypes.*;

import java.util.ArrayList;
import java.lang.*;

import april.jmat.*;



public class RoverPositioning {

	// args
	ArrayList<Matrix> history;
	double cur_x;	// current x coordinate in robot coordinate frame
	double cur_y;	// current y coordinate in robot coordinate frame
	double cur_t;	// current theta orientation in robot coordinate frame
	double prev_x;
	double prev_y;
	double prev_t;
	

	public RoverPositioning() {
		this.history = new ArrayList<Matrix>();	// A series of transformations since beginning
		this.cur_x = this.cur_y = 0;	// init to origin
		this.cur_t = 0;
		this.prev_x = this.prev_y = 0;	// init to origin
		this.prev_t = 0;
	}

	public double[] getPrevPosition() {
		double[] prev = {prev_x, prev_y, prev_t};
		return prev;
	}


	public double[] getNewPosition(pos_t msg) {

		double[] current_pos = new double[3];
		current_pos[0] = cur_x + msg.delta_x * Math.cos(msg.theta);
		current_pos[1] = cur_y + msg.delta_x * Math.sin(msg.theta);
		current_pos[2] = (msg.theta) % (2*Math.PI);
		prev_x = cur_x;
		prev_y = cur_y;
		prev_t = cur_t;
		cur_x = current_pos[0];
		cur_y = current_pos[1];
		cur_t = current_pos[2];
		
		// return current position
		return current_pos;
	}

}
