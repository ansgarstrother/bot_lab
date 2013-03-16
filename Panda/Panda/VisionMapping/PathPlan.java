package Panda.VisionMapping;

import java.util.ArrayList;
import javax.swing.*;
import java.awt.*;
import java.math.*;

public class PathPlan{
	//40 cm
	protected static final int FORWARD_DISTANCE = 40;

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

    protected double prevAngle;

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
	}

//=================================================================//
// plan                                                            //
//                                                                 //
// Runs A* algorithm and and sets pathAngle and pathDistance       //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public void plan(int[][] map){

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
	public void simplePlan(int[][] map){
        m = map;
		//TODO: Set X, Y and heading here

        Pos p = new Pos();

        int[] possibleAngleArray = {0, -45, 45};

        for (int i : possibleAngleArray){
            //Calculate point based on possible angle and go there if no barriers
            double angleFromPrev = heading - prevAngle + possibleAngleArray[i];
            p.x = (int)(FORWARD_DISTANCE * Math.sin(angleFromPrev)) + botXPos;
            p.y = (int)(FORWARD_DISTANCE * Math.cos(angleFromPrev)) + botYPos;

            if (checkPath(p)){
                pathAngle = heading + angleFromPrev;
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

}





