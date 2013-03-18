

import java.util.*;



public class PathPlanning {

	protected class Pos{
		int x;
		int y;
	}

	// const
	private static final double BORDER_DIST = 0.30; //(m)
	private static final double DELTA_X = 0.20; 	//(m)
	private static final double DELTA_THETA = 110;		// (degrees)

	
	// args
	protected ArrayList<double[][]> barriers;
	protected double[] triangle;
	protected double next_angle;
	protected double next_dist;
	protected double prev_angle;

	// CONSTRUCTOR METHOD
	public PathPlanning() {
		this.barriers = new ArrayList<double[][]>();
		

	}

	// CLASS METHODS
	public double Plan(ArrayList<double[][]> barriers) {
		this.barriers = barriers;

		// Check triangle proximity
		// double[] triangle = [x y z]
		// if triangle is within 0.3m to 0.4m, shoot triangle
		Pos cur_pos = new Pos();
		cur_pos.x = 0;
		cur_pos.y = 0;

		
		// Check barrier proximity
		// returns true if we can move distance
		if (!checkBarrierProximity(cur_pos, barriers)) {
			// turn 110 degrees to the left
			// remove current settings
			barriers.clear();
			return DELTA_THETA;
		}

		else {
			// go straight
			// remove current settings
			barriers.clear();
			return 0;
		}

	}


	// PROTECTED METHODS
	protected boolean checkBarrierProximity(Pos p, ArrayList<double[][]> barriers) {
  		// for all barriers
		// model a line
		// check to see if i project a line if it intersects the barrier line
		// if it does, return false
		// else continue
		// return true

		// z = mx + b		

		System.out.println("Barrier Proximity Detection Beginning");
		for (int i = 0; i < barriers.size(); i++) {
			double[][] cur_barrier = new double[2][2];
			cur_barrier = barriers.get(i);
			//System.out.println("Barrier at i: ");
			double m = (cur_barrier[1][1] - cur_barrier[0][1]) / (cur_barrier[1][0] - cur_barrier[0][0]);
			double b = cur_barrier[0][1] - m * (cur_barrier[0][0]);
			if (b < DELTA_X) {
				return false;
			}
		}

		return true;
	}

}

