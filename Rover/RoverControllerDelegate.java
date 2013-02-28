package Rover;

import lcmtypes.*;

public interface RoverControllerDelegate {
	
	// Delegate Class Methods
	public void update(double[] prev_pos, double[] new_pos, pos_t new_msg, double[] covar_vec);
    public void finished(long time);
	public RoverFrame getFrame();
	
}
