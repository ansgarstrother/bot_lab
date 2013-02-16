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

import april.jmat.MathUtil;
import april.jmat.Matrix;
import april.util.ParameterGUI;
import april.util.ParameterListener;
import april.util.TimeUtil;
import april.vis.VisChain;
import april.vis.VisWorld;

public class RoverApplicationController implements RoverControllerDelegate {

	protected RoverFrame frame;
	protected RoverModel roverModel;

	public RoverApplicationController(RoverFrame frame) {
		this.frame = frame;
		roverModel = new RoverModel();

		// GUI Sliders
		this.frame.pg.addString("roverStatus", "Speed Racer, What is Your Status?", "Idle");
		this.frame.pg.setEnabled("roverStatus", false);

		// callbacks
		// BUTTON ACTION LISTENERS
		// Execute Button
		this.frame.executeButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				// perform something on execute
			}
		});
		// Reset View Button
		this.frame.resetViewButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				resetCameraView();
			}
		});
		// PG Listener
		this.frame.pg.addListener(new ParameterListener() {
			@Override
			public void parameterChanged(ParameterGUI arg0, String arg1) {
				update();
			}
		});
		
		// initial update -> pass in dummy as t1
		resetCameraView();

		this.frame.setSize(800, 800);
		this.frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		this.frame.setVisible(true);

		update();

	}

	public void update() {
		// build visChain arm
		VisChain rover = new VisChain();
		rover.add(roverModel.getRoverChain());
		VisWorld.Buffer vb = this.frame.vw.getBuffer("rover");
		vb.addBack(rover);
		vb.swap();
	}

	protected void resetCameraView()
	{
		/* Point vis camera the right way */
		this.frame.vl.cameraManager.uiLookAt(
				new double[] {-22.7870, -63.5237, 60.5098 },
				new double[] { 0,  0, 0.00000 },
				new double[] { 0.13802,  0.40084, 0.90569 }, true);
	}

}
