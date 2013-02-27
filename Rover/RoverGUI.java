package Rover;


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
				// init controller
				RoverApplicationController controller = new RoverApplicationController(mainWindow, secondWindow);
			}
		});
	}
}
