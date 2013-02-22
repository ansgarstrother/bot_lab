package Rover;

import Vision.LineDetection.*;

import javax.swing.SwingUtilities;

import lcm.lcm.*;
import april.jmat.MathUtil;


public class RoverGUI
{
	public static void main(String[] args)
	{
		// initialize things here
		
		SwingUtilities.invokeLater(new Runnable() {
			@Override
			public void run() {
				// init java frames
				RoverFrame mainWindow = new RoverFrame();
				ellipseFrame secondWindow = new ellipseFrame();
				LineDetectionFrame frame = new LineDetectionFrame();
				// init controllers
				LineDetectionController appController = new LineDetectionController(frame);
				RoverApplicationController controller = new RoverApplicationController(mainWindow, secondWindow);
			}
		});
	}
}
