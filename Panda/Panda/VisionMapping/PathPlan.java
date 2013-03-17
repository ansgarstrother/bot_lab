package Panda.VisionMapping;

import java.util.ArrayList;
import javax.swing.*;
import java.awt.*;
import java.math.*;

public class PathPlan{
	//40 cm
	protected static final int FORWARD_DISTANCE = 40;
	protected static final int COUNT_THRESH = 4;	// full 360 degree turn

    int[][] m;
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
        prevAngle = 0;
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
	public void plan(int[][] map, int x, int y, int t) {
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
		//





	}



//=================================================================//
// advancedPlan                                                    //
//                                                                 //
// Runs a more advanced path planning algorithm to get around maze //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public void advancedPlan(int[][] map, int x, int y, int t) {
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
            p.x = (int)(FORWARD_DISTANCE * Math.sin(new_angle)) + botXPos;
            p.y = (int)(FORWARD_DISTANCE * Math.cos(new_angle)) + botYPos;

            if (checkPath(p)){
                pathAngle = new_angle;
                pathDistance = FORWARD_DISTANCE;
				no_move_count = 0;	//reset no move count
                return;
            }
        }

		// if no move is available, rotate 90 and search
		// the return statement in the for loop allows us to do this if statement
		if (pathAngle == heading) {
			pathAngle = new_angle + Math.toRadians(90);
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
	public void simplePlan(int[][] map, int x, int y, int t){
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

            if (checkPath(p)){
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
	protected boolean checkPath(Pos p){
        Pos temp = new Pos();
		ArrayList<Pos> pathPoints = new ArrayList<Pos>();

		//Run Bresenham's
		double deltaX = p.x - botXPos;
		double deltaY = p.y - botYPos;
		double error = 0;
		double deltaError = Math.abs(deltaY / deltaX);

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
			if (m[pos.x][pos.y] > 0){
				return false;
			}
		}

		//Path is good return true
		return true;
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





