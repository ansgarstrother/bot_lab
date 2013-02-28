package Rover;

import java.awt.Color;
import java.util.*;

import april.vis.*;
import april.jmat.*;

import lcmtypes.*;


public class RoverPath {

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

	// LCM type variables
	private static boolean triangle;
	private static double x_pos_new;
	private static double y_pos_new;
	private static double x_pos_old;
	private static double y_pos_old;

	private static double length;

	// BLANK CONSTRUCTOR
	public RoverPath() {
		// initiate length
		length = 0;
	}


	// BUILD VISCHAIN
	protected static void createPath(VisChain chain) {
		VzRectangle greenStripe = new VzRectangle( length, 3, new VzMesh.Style(Color.green));
		// implement translations and rotations based on pose
		chain.add(LinAlg.translate(length/2,0,0),
				greenStripe);
	}
	protected static void createWaypoint(VisChain chain) {
		VzCircle purpleWaypoint = new VzCircle( 5.0, new VzMesh.Style(Color.magenta));
		if (triangle) {
			chain.add(LinAlg.translate(length/2,0,0),
					purpleWaypoint);
		}
	}
	
	protected static VisChain getChain() {
		VisChain chain = new VisChain();
		createPath(chain);
		createWaypoint(chain);
		return chain;
	}






	// RETURN FUNCTION
	public VisChain getRoverPath(double[] prev_pos, double[] new_pos, pos_t new_msg) {
		// retrieve lcm type information
		triangle = new_msg.triangle_found;
		x_pos_new = new_pos[0];
		y_pos_new = new_pos[1];
		x_pos_old = prev_pos[0];
		y_pos_old = prev_pos[1];

		VisChain segment = getChain();	


		return segment;		
	}

}

