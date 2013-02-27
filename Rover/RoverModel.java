package Rover;


import java.awt.Color;
import java.util.*;

import april.vis.*;
import april.jmat.*;

import lcmtypes.*;

public class RoverModel
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

	// LCM type information
	private static float x_pos;
	private static float y_pos;
	private static double theta;

	public RoverModel()
	{
	}

	// creates a VisChain that wraps a point (sphere) whose
	// origin is at the vertical (z-axis) bottom, horizontal (x,y axes) center
	protected static VisChain createPoint(
		final double xCoord,
		final double yCoord,
		final double zCoord)
	{
		VisChain chain = new VisChain();
		VzSphere point = new VzSphere(0.5, new VzMesh.Style(Color.yellow));
		chain.add(LinAlg.translate(xCoord,yCoord, zCoord / 2), point);
		return chain;
	}

	// creates the body of the vehicle
	// Prism can't be built in visworld :(
	protected static void createBody(VisChain chain) {
		VzBox bottom_box = new VzBox(	v1_vehicle_width, 
						v2_vehicle_width, 
						v2_vehicle_height_B, 
						new VzMesh.Style(Color.white));
		chain.add(LinAlg.translate(0, 0, v2_vehicle_height_B / 2 + v2_wheel_diameter), bottom_box);
		
	}

	// creates the housing component
	protected static void createHousing(VisChain chain) {
		VzBox housing = new VzBox(	v1_sensbox_width, 
						v1_sensbox_height, 
						v1_sensbox_depth, 
						new VzMesh.Style(Color.gray));
		chain.add(LinAlg.translate(	7*v1_sensbox_offset - v2_vehicle_width / 2, 
						0, 
						v2_vehicle_height_B / 2 + v1_sensbox_depth / 2), 
						housing);
	}

	// create a wheel
	protected static void createWheel(VisChain chain, double x_offset) {
		VzCylinder wheel = new VzCylinder(	v2_wheel_diameter / 2,
							v1_wheel_thickness,
							new VzMesh.Style(Color.black));
		chain.add(
			//LinAlg.translate(x_offset, v1_vehicle_height, v2_wheel_diameter / 4), wheel);
			 //LinAlg.translate(0,-1.9*v2_wheel_diameter,x_offset), 
			 LinAlg.rotateY(Math.toRadians(90)),
			 LinAlg.rotateX(Math.toRadians(90)), 
			 LinAlg.translate(v1_vehicle_height / 2 + v2_wheel_diameter / 1.22, 0, x_offset), wheel);
		chain.add(LinAlg.translate(0,0,-x_offset * 2), wheel);
		chain.add(LinAlg.translate(0, v1_vehicle_height / 2 + v2_wheel_diameter / 4, x_offset), wheel);
	}

	// create racing stripes
	protected static void createStripes(VisChain chain) {
		VzRectangle blueStripe = new VzRectangle( 1.5, v1_vehicle_height - 5, new VzMesh.Style(Color.blue));
		chain.add(	LinAlg.rotateY(Math.toRadians(90)), 
				LinAlg.translate(0,-v2_vehicle_width / 3.5, -1.6*v2_vehicle_height_B), 
				blueStripe);
		
		VzRectangle redStripe = new VzRectangle(1.0, v1_vehicle_height - 5, new VzMesh.Style(Color.red));
		chain.add(LinAlg.translate(1.5, 0, 0), redStripe);
		chain.add(LinAlg.translate(-3.0, 0, 0), redStripe);
	}

	protected static VisChain getChain() {
		VisChain chain = new VisChain();
		createBody(chain);
		createHousing(chain);
		createWheel(chain, v1_vehicle_width / 2 + 3*v1_wheel_spacing);
		createStripes(chain);
		return chain;
	}

	public VisChain getRoverChain(pos_t msg) {
		this.x_pos = msg.x_pos;
		this.y_pos = msg.y_pos;
		this.theta = msg.theta;

		VisChain rover = getChain();
		return rover;
	}

}
