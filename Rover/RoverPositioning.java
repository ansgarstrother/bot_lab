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
	

	public RoverPositioning() {
		this.history = new ArrayList<Matrix>();	// A series of transformations since beginning
		this.cur_x = this.cur_y = 0;	// init to origin
	}

	public double[] getPrevPosition() {
		double[] prev = {cur_x, cur_y};
		return prev;
	}


	public double[] getNewPosition(pos_t msg) {
		// add current transform to history
		double[][] trans = {{Math.cos(msg.theta),	-Math.sin(msg.theta),	msg.x_pos},
							{Math.sin(msg.theta), 	Math.cos(msg.theta), 	msg.y_pos},
							{0, 					0, 						1		}};


		Matrix T = new Matrix(trans);
		history.add(T);

		// use current transform to find current location
		double[][] previous_pos = {{cur_x}, {cur_y}, {1}};
		Matrix prev_mat = new Matrix(previous_pos);
		Matrix affine_vec = T.transpose()
								.times(T)
								.inverse()
								.times(T.transpose());
		Matrix cur_mat = affine_vec.times(prev_mat);
		double[] current_pos = new double[2];
		current_pos[0] = cur_mat.get(0,0);
		current_pos[1] = cur_mat.get(1,0);
		cur_x = current_pos[0];
		cur_y = current_pos[1];
		
		// return current position
		return current_pos;
	}

}
