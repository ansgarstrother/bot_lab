package Rover;
import lcmtypes.*;

public interface RoverControllerDelegate {
	
	// Delegate Class Methods
	public void update(pos_t prev_msg, pos_t new_msg, double[] covar_vec);
	
}
