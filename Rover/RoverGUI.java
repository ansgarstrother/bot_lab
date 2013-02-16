package Rover;

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
				RoverFrame mainWindow = new RoverFrame();
				RoverApplicationController controller = new RoverApplicationController(mainWindow);
			}
		});
	}
}
