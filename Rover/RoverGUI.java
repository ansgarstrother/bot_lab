package Rover;

import Vision.LineDetection.*;
import Vision.calibration.*;

import javax.swing.SwingUtilities;

import lcm.lcm.*;
import april.jmat.MathUtil;


public class RoverGUI
{
	// args
	static double[][] calibMatrix;

	public static void main(String[] args)
	{

		// initialize things here
		// retrieve calibration matrix from file
		
		SwingUtilities.invokeLater(new Runnable() {
			@Override
			public void run() {
				// init java frames
				RoverFrame mainWindow = new RoverFrame();
				ellipseFrame secondWindow = new ellipseFrame();
				LineDetectionFrame frame = new LineDetectionFrame();
				// init controllers
				LineDetectionController ldc = new LineDetectionController(frame, calibMatrix);
				RoverApplicationController controller = new RoverApplicationController(mainWindow, secondWindow, ldc);
			}
		});
	}
}
