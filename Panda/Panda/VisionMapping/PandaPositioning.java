package Panda.VisionMapping;

import lcmtypes.*;

import java.util.ArrayList;
import java.lang.*;

import april.jmat.*;



public class PandaPositioning {

	// constants
	private static final double static_height = 0.2032;	// camera height from Z=0
    private final double Z_OFFSET_SLOPE = -0.00147;

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
				{0, 		0, 		0,		1	}};


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
		double first_X = scale * (pixels[0] - intrinsics[1]);
		double first_Y = scale * (intrinsics[2] - pixels[1]);
		double first_Z = scale * intrinsics[0];


		double[][] res = new double[3][1];

        //res[0][0] = calcX (first_X, pixels[0]);
        res[0][0] = first_X;
        res[1][0] = first_Y; //res[2][0] = first_Z;
        res[2][0] = calcZ (first_Z, pixels[1]);
		Matrix ret_mat = new Matrix(res);
		return ret_mat;
	}

    public double calcX (double calc_val, double pixel_x) {

        double offset_factor = pixel_x * .00513 - 3.69726;
        double actual_x = calc_val / offset_factor;
        return actual_x;

    }

    public double calcZ (double calc_val, double pixel_y) {

        double offset_factor = pixel_y * -.00209 + 2.85903;
        double actual_z = calc_val / offset_factor;
        return actual_z;


    }




}
