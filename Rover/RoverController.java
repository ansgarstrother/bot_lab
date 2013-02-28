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

import javax.swing.*;

import lcm.lcm.*;
import lcmtypes.*;

import april.jmat.MathUtil;
import april.jmat.Matrix;
import april.util.ParameterGUI;
import april.util.ParameterListener;
import april.util.TimeUtil;
import april.vis.*;


public class RoverController implements Runnable {

	//args
	protected class ConstraintException extends Exception {};

	final protected RoverControllerDelegate delegate;
	protected boolean isExecuting;

	protected RoverSubscriber subscriber;
	
	protected volatile pos_t new_msg;
	protected pos_t prev_msg;
	protected double[] covar_vec;




	// CONSTRUCTOR METHOD
	public RoverController(RoverControllerDelegate delegate) {
		this.delegate = delegate;
		this.isExecuting = false;
		this.prev_msg = new pos_t();
		this.new_msg = new pos_t();
		this.covar_vec = new double[3];

		try { this.subscriber = new RoverSubscriber(); }
		catch (Exception e) { System.err.println("Error initializing Subscriber: " + e.getMessage()); }

	}

	// CLASS METHODS
/*
	// Retrieve new position
	public pos_t getNewPosition() {
		pos_t msg = null;
		synchronized(this) {
			msg = new_msg;
			new_msg = null;
		}
		return msg;
	}
	public synchronized void setNewPosition(pos_t newMessage) {
		this.new_msg = newMessage;
	}
*/


	// Execution Method
	public void execute() {
		synchronized (this) {
			if (this.isExecuting) return;
			this.isExecuting = true;

		}
		
        // init
        delegate.frame.getParameterGUI().ss("roverStatus", "Running");
        boolean finished = false;
        final long startTime = System.currentTimeMillis();
        
        // run
        while (!finished) {
            // Retrieve current coordinates of rover
            // Calculate covariance vector
            // Test for triangle location/finished booleans
            // Update GUI
            this.prev_msg = new_msg;
            this.new_msg = subscriber.getPose();
            // CALCULATE COVARIANCE
            
            
            if (!new_msg.finished) {
                update();
            }
            else {
                finished = true;
                delegate.frame.getParameterGUI().ss("roverStatus", "Idle");
            }
        }
        
        // finish
        final long endTime = System.currentTimeMillis();
        System.out.println("Mission Complete!");
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                delegate.finished(endTime - startTIme);
            }
        });
        
        synchronized (this) {
            this.isExecuting = false;
        }
		
	}


	// GUI Update Method
	public void update() {
		SwingUtilities.invokeLater(new Runnable() {
			@Override
			public void run() {
				delegate.update(prev_msg, new_msg, covar_vec);
			}
		});
	}


	// Runnable Method
	@Override
	public void run() {
		System.out.println("GO SPEED RACER GO!");
		execute();
	}

	




}	
