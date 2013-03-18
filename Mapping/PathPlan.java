package Mapping;

import java.util.ArrayList;
import javax.swing.*;
import java.awt.*;
import java.math.*;
import java.util.HashMap;

public class PathPlan{
	//40 cm
	protected static final int FORWARD_DISTANCE = 40;
	protected static final int COUNT_THRESH = 4;	// full 360 degree turn
	protected static final int TRAVERSE_STEP = 2;	// traverse step along line to search for unexplored space/barriers
	protected static final int UNEXPLORED_REP = 1;	// representation of unexplored is an integer (1)
	protected static final int BARRIER_REP = 2;		// representation of a barrier is an integer (2)

    HashMap<Point, Integer> m;
	protected class Pos{
		int x;
		int y;
	}

	protected int botXPos;
	protected int botYPos;
	protected double heading;
	protected double pathAngle;
	protected double pathDistance;

	protected int no_move_count;	// keeps track of the number of times no move has been found in a row
									// if we do a complete 360 turn (count = 4), send a finished flag
	protected boolean finished;


//=================================================================//
// PathPlan                                                        //
//                                                                 //
// Initailizes A* path plan class.                                 //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public void PathPlan(){
		pathAngle = 0;
		pathDistance = 0;

		no_move_count = 0;
		finished = false;
	}

//=================================================================//
// plan                                                            //
//                                                                 //
// Runs A* algorithm and and sets pathAngle and pathDistance       //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public void plan(HashMap<Point, Integer> map, int x, int y, double t) {
		// init local variables
        m = map;
		botXPos = x;	// current x
		botYPos = y;	// current y
		heading = t;	// current orientation





	}



//=================================================================//
// advancedPlan                                                    //
//                                                                 //
// Runs a more advanced path planning algorithm to get around maze //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public void advancedPlan(HashMap<Point, Integer> map, int x, int y, double t) {
		// init local variables
        m = map;
		botXPos = x;	// current x
		botYPos = y;	// current y
		heading = t;	// current orientation

		// INITAL IDEA:
		//	- have the priority queue for searching for new angles
		//	- travel along the line of a given angle in search of
		//		unexplored space while checking for barrier
		//	- return new angle from priority queue
		//	- return new distance (dist from unexplored coord to cur coord)

		//	TEST FOR NO POSSIBLE MOVE:
		//	- if there is no unexplored area in this current orientation:
		//	- force the robot to turn 90 degrees, no change in distance
		//	- increment a count
		//	- if the count is passed a threshold, throw finished flag

		if (no_move_count >= COUNT_THRESH) {
			//throw a finished flag
			finished = true;
			return;
		}

        Pos p = new Pos();

        int[] possibleAngleArray = {0, -30, 30, -45, 45, -60, 60};	//priority array

		// check if we can move forward
        for (int i : possibleAngleArray){
            //Calculate point based on possible angle and go there if no barriers
            double new_angle = heading + possibleAngleArray[i];
			// Traverse line until we run in to a border or unexplored space
            p.x = (int)(FORWARD_DISTANCE * Math.cos(new_angle)) + botXPos;
            p.y = (int)(FORWARD_DISTANCE * Math.sin(new_angle)) + botYPos;

            if (checkPath(p, true)){
                pathAngle = new_angle;
                // pathDistance assigned in checkPath
				no_move_count = 0;	//reset no move count
                return;
            }
        }

		// if no move is available, rotate 90 and search
		// the return statement in the for loop allows us to do this if statement
		if (pathAngle == heading) {
			pathAngle = heading + Math.toRadians(90);
			pathDistance = 0;
			no_move_count++;	// increment no move count
			return;
		}

	}




//=================================================================//
// simplePlan                                                      //
//                                                                 //
// Runs a simple path planning algorithm to get around maze        //
//      Go straight until you can't.  Turn in direction with       //
//      fewest walls.                                              //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public void simplePlan(HashMap<Point, Integer> map, int x, int y, double t){
        m = map;
		//bot's current orientation is passed in from main (calculated in Map)
		botXPos = x;
		botYPos = y;
		heading = t;

        Pos p = new Pos();

        int[] possibleAngleArray = {0, -30, 30, -45, 45, -60, 60};	//priority array

		// check if we can move forward
        for (int i : possibleAngleArray){
            //Calculate point based on possible angle and go there if no barriers
            double new_angle = heading + possibleAngleArray[i];
            p.x = (int)(FORWARD_DISTANCE * Math.sin(new_angle)) + botXPos;
            p.y = (int)(FORWARD_DISTANCE * Math.cos(new_angle)) + botYPos;

            if (checkPath(p, false)){
                pathAngle = new_angle;
                pathDistance = FORWARD_DISTANCE;
                return;
            }
        }

	}

//=================================================================//
// checkPath                                                       //
//                                                                 //
// Uses bresenham algorithm to get path on map to x and y cord     //
// the path for barriers then checks the                           //
//                                                                 //
// Returns: boolean                                                //
//=================================================================//
	protected boolean checkPath(Pos p, boolean unexploredFlag){
        Pos temp = new Pos();
		ArrayList<Pos> pathPoints = new ArrayList<Pos>();

		//Run Bresenham's
		double deltaX = Math.abs(p.x - botXPos);
		double deltaY = Math.abs(p.y - botYPos);
		double error = 0;
		double deltaError = deltaY / deltaX;

        int start, end, base;
        boolean decreasing = false;
        boolean horizontal = false;

            if (deltaX >= deltaY) {
                // barrier is more horizontal than vertical
                if (botXPos <= p.x) {
                    start = botXPos;
                    end = p.x;
                    base = botYPos;
                    if (botYPos > p.y) {
                        decreasing = true;
                    }
                } else {
                    start = p.x;
                    end = botXPos;
                    base = p.y;
                    if ( p.y > botYPos) {
                        decreasing = true;
                    }
                }
                horizontal = true;
                deltaError = deltaY / deltaX;

            } else {
                // barrier is more vertical than horizontal
                if (botYPos <= p.y) {
                    start = botYPos;
                    end = p.y;
                    base = botXPos;
                    if ( p.x > botXPos) {
                        decreasing = true;
                    }
                } else {
                    start = p.y;
                    end = botYPos;
                    base = p.x;
                    if (botXPos > p.x) {
                        decreasing = true;
                    }
                }
                deltaError = Math.abs(deltaX / deltaY);
            }

            System.out.println (start + " " + end + " " + base);

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

//=================================================================//
//getPathAngle                                                     //
//                                                                 //
// Returns path calculated by plan                                 //
//                                                                 //
// Returns: double                                                 //
//=================================================================//
	public double getPathAngle(){
		return pathAngle;
	}

//=================================================================//
//getPathDistance                                                  //
//                                                                 //
// Returns path distance calculated in plan                        //
//                                                                 //
// Returns: double                                                 //
//=================================================================//
	public double getPathDistance(){
		return pathDistance;
	}

//=================================================================//
//getFinishedTest                                                  //
//                                                                 //
// Returns finished boolean signifying no more moves left          //
//                                                                 //
// Returns: boolean                                                //
//=================================================================//
	public boolean getFinishedTest() {
		return finished;
	}


}





