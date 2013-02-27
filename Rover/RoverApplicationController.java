package Rover;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentListener;
import java.awt.geom.AffineTransform;
import java.awt.geom.Point2D;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.BufferedReader;
import java.io.FileReader;

import javax.swing.JFrame;
import javax.swing.JOptionPane;

import lcm.lcm.*;
import lcmtypes.*;

import april.jmat.MathUtil;
import april.jmat.Matrix;
import april.util.ParameterGUI;
import april.util.ParameterListener;
import april.util.TimeUtil;
import april.vis.*;

public class RoverApplicationController implements RoverControllerDelegate {

	protected RoverFrame frame;
	protected ellipseFrame errorFrame;
	protected RoverModel roverModel;
	protected RoverSubscriber subscriber;

	protected boolean finished;
	protected pos_t prev_msg;


	public RoverApplicationController(RoverFrame frame, ellipseFrame errorFrame) {
		finished = true;

		this.frame = frame;
		this.errorFrame = errorFrame;
		roverModel = new RoverModel();
		try {
			this.subscriber = new RoverSubscriber();
		} catch (Exception e) {
			System.err.println("Error initializing Subscriber: " + e.getMessage());
		}

		// GUI
		this.frame.pg.addString("roverStatus", "Speed Racer, What is Your Status?", "Idle");
		this.frame.pg.setEnabled("roverStatus", false);
		this.errorFrame.pg.addDouble("var_x", "Variance in X: ", 0);
		this.errorFrame.pg.setEnabled("var_x", false);
		this.errorFrame.pg.addDouble("var_y", "Variance in Y: ", 0);
		this.errorFrame.pg.setEnabled("var_y", false);
		this.errorFrame.pg.addDouble("covar", "Covariance: ", 0);
		this.errorFrame.pg.setEnabled("covar", false);

		// callbacks
		// BUTTON ACTION LISTENERS
		// Execute Button
		this.frame.executeButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				// begin listening for updates from lcm
				// if there is a change in position, mark it in GUI
				// if a triangle has been shot, mark it in GUI
				finished = false;
				while (!finished) {
					pos_t msg = subscriber.getPose();
					if (msg != prev_msg) {
						double[] init = {0, 0, 0};
						update(msg, init);
					}
					prev_msg = msg;
				}
				
			}
		});
		// Reset View Button
		this.frame.resetViewButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				resetCameraView();
			}
		});
		this.errorFrame.resetViewButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				resetErrorView();
			}
		});
		
		// initial update -> pass in dummy as t1
		resetCameraView();

		this.frame.setSize(800, 800);
		this.frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		this.frame.setVisible(true);
		this.errorFrame.setSize(300, 300);
		this.errorFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		this.errorFrame.setVisible(true);

		// initial covar = 0
		double[] init = {0, 0, 0};
		update(prev_msg, init);

	}

	public void update(pos_t msg, double[] covar_vec) {
		// covar_vec = [var_x, var_y, a] -> straight from LCM
		// build rover chain
		VisChain rover = new VisChain();
		rover.add(roverModel.getRoverChain());
		VisWorld.Buffer vb = this.frame.vw.getBuffer("rover");
		vb.addBack(rover);
		vb.swap();

		// BUILD ROVER ERROR ELLIPSE
		VisWorld.Buffer vbe = this.errorFrame.vw.getBuffer("error");
		VzEllipse ellipse = errorEllipse.getEllipse(covar_vec);
		vbe.addBack(ellipse);
		vbe.swap();

		// Update Parameter GUI
		this.errorFrame.pg.sd("var_x", covar_vec[0]);
		this.errorFrame.pg.sd("var_y", covar_vec[1]);
		this.errorFrame.pg.sd("covar", covar_vec[2]);
	}

	protected void resetCameraView()
	{
		/* Point vis camera the right way */
		this.frame.vl.cameraManager.uiLookAt(
				new double[] {-22.7870*4, -63.5237*4, 60.5098*4 },
				new double[] { 0,  0, 0.00000 },
				new double[] { 0.13802*4,  0.40084*4, 0.90569*4 }, true);
	}
	protected void resetErrorView()
	{
		this.errorFrame.vl.cameraManager.uiDefault();
	}

}
