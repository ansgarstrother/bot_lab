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
    private Matrix historyTrans;
    private Matrix curGlobalPos;
	private double scale;		// scale factor for calibration


	public PandaPositioning() {
		this.history = new ArrayList<Matrix>();	// A series of transformations since beginning
        double[] origin = { 0, 0, 1};
        this.curGlobalPos = new Matrix(origin);
        double[][] identitiy = { {1, 0, 0},
                                {0, 1, 0},
                                {0, 0, 1} };
        this.historyTrans = new Matrix (identity);
		this.scale = 0;
	}

    public void setNewGlobalPosition (pos_t msg) {
		double[][] newRelPos = {{Math.cos(-msg.theta),	-Math.sin(-msg.theta),	-msg.delta_x},
							{Math.sin(-msg.theta), 	Math.cos(-msg.theta),	0		},
							{0, 					0, 						1		}};


        Matrix relPosMat = new Matrix (newRelPos);
        curGlobalPos = relPosMat.times (curGlobalPos);
        historyTrans = historyTrans.times (newRelPos);
    }

    public Matrix getGlobalPos () {
        return curGlobalPos;

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
		double first_X = scale * (pixels[0] - intrinsics[1]);
		double first_Y = scale * (intrinsics[2] - pixels[1]);
		double first_Z = scale * intrinsics[0];

		double[][] res = new double[3][1];

        double scaled_Z = calcZ (first_Z, pixels[1]);
        double scaled_X = first_Z*(pixels[0] - intrinsics[1]) / intrinsics[0];

        res[0][0] = scaled_X;
        res[1][0] = scaled_Z;
        res[2][0] = first_Y;


		Matrix ret_mat = new Matrix(res);
        //Matrix global_Coords = historyTrans.times (ret_mat);
		//return global_Coords;
        return ret_mat;
	}


    public double calcZ (double calc_val, double pixel_y) {

        double offset_factor = pixel_y * -.00209 + 2.85903;
        double actual_z = calc_val / offset_factor;
        return actual_z;


    }




}
