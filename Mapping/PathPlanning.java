

public class PathPlanning {

	protected class Pos{
		int x;
		int y;
	}

	// const
	private static final double BORDER_DIST = 0.30; //(m)
	private static final double DELTA_X = 0.20; 	//(m)
	private static final double 

	
	// args
	protected ArrayList<double[][]> barriers;
	protected double[] triangle;
	protected double next_angle;
	protected double next_dist;
	protected double prev_angle;

	// CONSTRUCTOR METHOD
	public PathPlanning() {
		this.barriers = new ArrayList<double[][]>();
		this.triangle = new double[3];	// x y and z
		this.prev_angle = 0;
		this.next_angle = 0;
		this.next_dist = 0;
		this.prev_angle = 0;
		

	}

	// CLASS METHODS
	public void Plan(ArrayList<double[][]> barriers, double[] triangle, double prev_angle) {
		this.barriers = barriers;
		this.triangle = triangle;
		this.prev_angle = prev_angle;

		// Check triangle proximity
		// double[] triangle = [x y z]
		// if triangle is within 0.3m to 0.4m, shoot triangle
		Pos cur_pos = new Pos();
		cur_pos.x = 0;
		cur_pos.y = 0;

		
		// Check barrier proximity
		// returns true if we can move distance
		if (!checkBarrierProximity(cur_pos, barriers)) {
			// turn 90 degrees to the left
			this.next_angle = prev_angle + 90;
			this.next_dist = 0;
		}

		else {
			// Move 0.2 meter straight
			this.next_dist = DELTA_X;
			this.next_angle = prev_angle;
		}

		// remove current settings
		barriers.clear();

	}

	public double getAngle() {
		return next_angle;
	}
	
	public double getDistance() {
		return next_dist;
	}


	// PROTECTED METHODS
	protected boolean checkBarrierProximity(Pos p, ArrayList<double[][]> barriers) {
        Pos temp = new Pos();
		ArrayList<Pos> pathPoints = new ArrayList<Pos>();

		//Run Bresenham's
		double deltaX = Math.abs(p.x);
		double deltaY = Math.abs(p.y + BORDER_DIST);

        // from x_starting position to y_starting position
        for(int x = start; x < end; x++){
            // increase thickness of barrier
            Point pt;
            if (horizontal) {
               // set x is x-coord, y is y-coord
                pt = new Point (x, y);
            } else {
                // other way around
                pt = new Point (y, x);
            }
                m.put(pt,BARRIER);

                error += deltaError;
                if(error > .5) {
                    if (decreasing == true) {
                        base--;
                    } else {
                       base++;
                    }
                    error = error - 1;
                }

            }










		int y = botYPos;
		for (int x = botXPos; x <= p.x; x++){
            temp.x = x;
            temp.y = y;
			pathPoints.add(temp);
			error = error + deltaError;
			if (error >= 0.5){
				y += 1;
				error = error - 1;
			}
		}

		//Check path points for barriers
		for (Pos pos : pathPoints){
			//Barriers are positive numbers
			Point cur_point = new Point(pos.x, pos.y);
			if (m.get(cur_point) == BARRIER_REP){
				return false;
			}
			// unexplored found
			if (m.get(cur_point) == UNEXPLORED_REP && unexploredFlag) {
				// SET PATH DISTANCE
				double dx = pos.x - p.x; double dy = pos.y - p.y;
				pathDistance = Math.floor(Math.sqrt(dx*dx + dy*dy));
				return true;
			}
		}

		// No barrier hit
		if (!unexploredFlag) {
			return true;
		}

		//No unexplored set found
		return false;
	}

	}

