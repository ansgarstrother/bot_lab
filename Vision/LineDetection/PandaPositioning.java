package Vision.LineDetection;


import lcmtypes.*;

import java.util.ArrayList;
import java.lang.*;

import april.jmat.*;



public class PandaPositioning {

	// args
	ArrayList<Matrix> history;
	double cur_x;	// current x coordinate in robot coordinate frame
	double cur_y;	// current y coordinate in robot coordinate frame
	double cur_t;	// current theta orientation in robot coordinate frame
	double prev_x;
	double prev_y;
	double prev_t;
	

	public PandaPositioning() {
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
		// add current transform to history
		double[][] trans = {{Math.cos(-msg.theta),	-Math.sin(-msg.theta),	-msg.delta_x},
							{Math.sin(-msg.theta), 	Math.cos(-msg.theta), 	0		},
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
		double[] current_pos = new double[3];
		current_pos[0] = cur_mat.get(0,0);
		current_pos[1] = cur_mat.get(1,0);
		current_pos[2] = (cur_t + msg.theta) % (2*Math.PI);
		prev_x = cur_x;
		prev_y = cur_y;
		prev_t = cur_t;
		cur_x = current_pos[0];
		cur_y = current_pos[1];
		cur_t = current_pos[2];
		
		// return current position
		return current_pos;
	}

	public ArrayList<Matrix> getHistory() {
		return history;
	}

}
