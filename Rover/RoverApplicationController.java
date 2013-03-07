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

	// args
	protected Thread roverControllerThread;

	protected RoverFrame frame;
	protected ellipseFrame errorFrame;
	protected RoverController roverController;

	protected RoverModel roverModel;
	protected RoverPath roverPath;
	//protected VisChain green_path;
	protected pos_t init_msg;


	// CONSTRUCTOR METHOD
	public RoverApplicationController(RoverFrame frame, ellipseFrame errorFrame) {

		this.frame = frame;
		this.errorFrame = errorFrame;
		this.roverModel = new RoverModel();
		this.roverPath = new RoverPath();
		//this.green_path = new VisChain();
		this.init_msg = new pos_t();

		// callbacks
		// BUTTON ACTION LISTENERS
		// Execute Button
		this.frame.executeButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				// Change status to Running
				// Disable Execute button
				startRoverController();
				/*
				RoverApplicationController.this.frame.pg.ss("roverStatus", "Running");
				RoverApplicationController.this.frame.pg.notifyListeners("roverStatus");
				RoverApplicationController.this.frame.executeButton.setEnabled(false);	
				*/		
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
		
		// initial update
		resetCameraView();

		this.frame.setSize(800, 800);
		this.frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		this.frame.setVisible(true);
		this.errorFrame.setSize(300, 300);
		this.errorFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		this.errorFrame.setVisible(true);

		double[] init = {0, 0, 0};
		double[] initLocation = {0,0,0};
		update(initLocation, initLocation, init_msg, init);
		

	}

	protected void startRoverController() {
		if (this.roverControllerThread != null) { return; }
		this.roverController = new RoverController(this);
		this.roverControllerThread = new Thread(this.roverController);
		this.roverControllerThread.start();
	}

	protected void resetCameraView() {
		/* Point vis camera the right way */
		this.frame.vl.cameraManager.uiLookAt(
				new double[] {-22.7870*4, -63.5237*4, 60.5098*4 },
				new double[] { 0,  0, 0.00000 },
				new double[] { 0.13802*4,  0.40084*4, 0.90569*4 }, true);
	}
    
	protected void resetErrorView() {
		this.errorFrame.vl.cameraManager.uiDefault();
	}
    
    @Override
	public void update(double[] prev_pos, double[] new_pos, pos_t new_msg, double[] covar_vec) {

		// covar_vec = [var_x, var_y, a] -> straight from LCM
		// build rover chain
		VisChain rover = new VisChain();
		rover.add(roverModel.getRoverChain(new_pos));
		VisWorld.Buffer vb = this.frame.vw.getBuffer("rover");
		
		// update green path tracking
		//VisChain green_path = new VisChain();
		//green_path.add(roverPath.getRoverPath(prev_pos, new_pos, new_msg));
        
		// build world chain & swap
		//VisChain world = new VisChain();
		//world.add(green_path, rover);
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
    
    @Override
    public void finished(long duration) {
        double seconds = (duration * 1.0 / 1000.0);
        JOptionPane.showMessageDialog(
                                      this.frame,
                                      "Finished in " + seconds + " seconds.",
                                      "Mission Complete",
                                      JOptionPane.INFORMATION_MESSAGE);
        
    }

    @Override
    public RoverFrame getFrame() {
		return frame;
    }

}
