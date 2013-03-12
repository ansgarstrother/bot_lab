package Panda.VisionMapping;

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
	}


	public void setNewPosition(pos_t msg) {
		// add current transform to history
		double[][] trans = {{Math.cos(-msg.theta),	-Math.sin(-msg.theta),	0,		-msg.delta_x},
							{Math.sin(-msg.theta), 	Math.cos(-msg.theta), 	0, 		0		},
							{0, 					0, 						0,		1		}};


		Matrix T = new Matrix(trans);
		history.add(T);
	}

	public ArrayList<Matrix> getHistory() {
		return history;
	}

}
