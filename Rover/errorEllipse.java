package Rover;


import java.awt.Color;
import java.util.*;

import april.vis.*;
import april.jmat.*;


public class errorEllipse
{
	// MEASUREMENTS (cm)
	// Orientation View #1
	static final double v1_vehicle_width = 15.5575;
	static final double v1_vehicle_height = 16.8275;
	static final double v1_sensbox_width = 6.35;
	static final double v1_sensbox_height = 5.08;
	static final double v1_sensbox_offset = 0.79;
	static final double v1_sensbox_depth = 8.89;
	static final double v1_wheel_spacing = 0.635;
	static final double v1_wheel_thickness = 1.27;
	// Orientation View #2
	static final double v2_wheel_diameter = 7.62;
	static final double v2_vehicle_width = 17.78;
	static final double v2_vehicle_height_A = 5.08;
	static final double v2_vehicle_height_B = 6.50875;
	static final double v2_theta = 0.08011; 	// radians

	public errorEllipse()
	{
	}

	protected static VzEllipse getEllipse(double[] covar_vec) {
		// Do something with covariance vector to create ellipse
		// mu = vector of means?
		// P = points?

		// TEST CASE
		double[][] test = {{1, 3}, {3, 1}};
		double[] mu = {2, 2};
		VzEllipse ellipse = new VzEllipse(mu, test, new VzMesh.Style(Color.pink));
		return ellipse;
	}

}
