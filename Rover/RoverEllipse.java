package Rover;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentListener;
import java.awt.geom.AffineTransform;
import java.awt.geom.Point2D;
import java.io.BufferedReader;
import java.io.FileReader;

import javax.swing.*;

import lcm.lcm.*;
import lcmtypes.*;

import april.jmat.MathUtil;
import april.jmat.Matrix;
import april.util.ParameterGUI;
import april.util.ParameterListener;
import april.util.TimeUtil;
import april.vis.*;


public class RoverEllipse {

	//args
	private Matrix A;
	private final static double alpha = 0.1;
	private static double stdev;

	// CONSTRUCTOR METHOD
	public RoverEllipse(pos_t msg) {
		// retrieve A matrix
		double[][] trans = {{Math.cos(-msg.theta),	-Math.sin(-msg.theta),	-msg.delta_x},
							{Math.sin(-msg.theta), 	Math.cos(-msg.theta), 	0		},
							{0, 					0, 						1		}};
		this.A = new Matrix(trans);

		// retrieve stdev
		this.stdev = msg.delta_x * alpha;


	}
	



}
