package Rover;

import javax.swing.SwingUtilities;

import lcm.lcm.LCM;
import lcmtypes.dynamixel_command_list_t;
import lcmtypes.dynamixel_command_t;
import april.jmat.MathUtil;


public class RoverGUI
{
	public static void main(String[] args)
	{
		// initializations go here
		
		SwingUtilities.invokeLater(new Runnable() {
			@Override
			public void run() {
				RoverFrame mainWindow = new RoverFrame();
				RoverApplicationController controller = new RoverApplicationController(mainWindow);
			}
		});
	}
}