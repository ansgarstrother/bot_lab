package Panda.VisionMapping;

import lcmtypes.*;

import java.util.ArrayList;
import java.lang.*;
import Panda.sensors.*;
import april.jmat.*;



public class PandaPositioning {

	// constants
	private static final double static_height_wall = 0.2032;		// camera height from Z=0
	private static final double static_height_triangle = static_height_wall - 0.1524;	// camera height from triangle

	// args
    private Matrix curGlobalPos;
    private double curGlobalTheta;

    private double[][] origin = { {0},{0},{1} };
    Matrix originMat;

	public PandaPositioning() {
		//this.history = new ArrayList<Matrix>();	// A series of transformations since beginning
        originMat = new Matrix (origin);
        this.curGlobalPos = originMat;
        this.curGlobalTheta = 0;
	}

    public void updateGlobalPosition (double distance, double theta) {
        // translate
        double[][] translate = { {1, 0, distance},
                                    {0, 1, 0},
                                    {0, 0, 1} };
		double[][] rotate = {{Math.cos(curGlobalTheta),	-Math.sin(curGlobalTheta),	0},
							{Math.sin(curGlobalTheta), 	Math.cos(curGlobalTheta),	0},
							{0, 					0, 						1}};
        Matrix translateMat = new Matrix (translate);
        Matrix rotateMat = new Matrix (rotate);
        Matrix tempPos = rotateMat.times (originMat);

        curGlobalPos = translateMat.times(tempPos);
        curGlobalTheta = theta;
        // get current global position by multiplying relative transformation with previous

    }

    public Matrix getGlobalPos() {
        return curGlobalPos;

    }

    public double getGlobalTheta() {
        return curGlobalTheta;
    }



    public Matrix getLocalPoint(double[] intrinsics, double[] pixels, boolean detectingLine) {
		// intrinsics = [f, cx, cy]
		// pixels = [u, v]
		// type = true for wall, false for triangle
		double static_height = 0;
		if (detectingLine == true) {
			static_height = static_height_wall;
		} else {
			static_height = static_height_triangle;
		}


		double scale = Math.abs(static_height / (pixels[1] - intrinsics[2]));
		double first_X = scale * (intrinsics[1] - pixels[0]);
		double first_Y = scale * (intrinsics[2] - pixels[1]);
		double first_Z = scale * intrinsics[0];

		double[][] res = new double[3][1];

        double scaled_Z = calcZ (first_Z, pixels[1]);
        double scaled_X = first_Z*(intrinsics[1] - pixels[0]) / intrinsics[0];

        if (detectingLine == true) {
            res[0][0] = first_Z;
            res[1][0] = first_X;
            res[2][0] = 1;
        } else {
            System.out.println ("getting triangle coords!! " + first_Z);
            double triangle_Z = calcZ_triangle (first_Z);

            res[0][0] = triangle_Z;
            res[1][0] = first_X;
            res[2][0] = 1;

        }


        // in robot's coordinate frame
		Matrix ret_mat = new Matrix(res);
		return ret_mat;
        //return translateToGlobal (ret_mat);
	}

	public Matrix getGlobalPoint (double[] intrinsics, double[] pixels, boolean detectingLine) {

		Matrix localPoint = getLocalPoint (intrinsics, pixels, detectingLine);
		Matrix globalPoint = translateToGlobal (localPoint);

		return globalPoint;


	}

    public Matrix translateToGlobal (Matrix coordinate) {


        double[][] translate = { {1, 0, curGlobalPos.get(0,0)},
                                    {0, 1, curGlobalPos.get(1,0)},
                                    {0, 0, 1} };

		double[][] rotate = {{Math.cos(curGlobalTheta),	-Math.sin(curGlobalTheta),	0},
							{Math.sin(curGlobalTheta), 	Math.cos(curGlobalTheta),	0},
							{0, 					0, 						1}};

        Matrix translateMat = new Matrix (translate);
        Matrix rotateMat = new Matrix (rotate);
        Matrix tempPos = rotateMat.times (coordinate);

        Matrix globalCoord = translateMat.times(tempPos);

        return globalCoord;

    }


    public double calcZ (double calc_val, double pixel_y) {

        double offset_factor = pixel_y * -.15008 + .9122;
        double actual_z = calc_val / offset_factor;
        return actual_z;


    }

    public double calcZ_triangle (double first_Z) {

        double offset_factor = first_Z * -0.15008 + 0.91222;
        double actual_z = first_Z * offset_factor;
        return actual_z;


    }


}
