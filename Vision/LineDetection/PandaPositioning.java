package Vision.LineDetection;

import lcmtypes.*;

import java.util.ArrayList;
import java.lang.*;

import april.jmat.*;



public class PandaPositioning {

	// constants
	private static final double static_height = 0.2032;	// camera height from Z=0

	// args
	private ArrayList<Matrix> history;
	private double scale;		// scale factor for calibration
	

	public PandaPositioning() {
		this.history = new ArrayList<Matrix>();	// A series of transformations since beginning
		this.scale = 0;
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
	
	public Matrix getGlobalPoint(double[] intrinsics, double[] pixels) {
		// intrinsics = [f, cx, cy]
		// pixels = [u, v]
		double scale = Math.abs(static_height / (pixels[1] - intrinsics[2]));
		double X = scale * (intrinsics[1] - pixels[0]);
		double Y = scale * (intrinsics[2] - pixels[1]);
		double Z = scale * intrinsics[0];
		double[][] res = new double[3][1];
		res[0][0] = X; res[1][0] = Y; res[2][0] = Z;
		Matrix ret_mat = new Matrix(res);
		return ret_mat;
	}

}
