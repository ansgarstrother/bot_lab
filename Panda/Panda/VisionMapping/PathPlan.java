package Panda.VisionMapping;

import java.util.ArrayList;
import javax.swing.*;
import java.awt.*;
import java.math.*;

public class PathPlan{
	//40 cm
	protected static final int forwardDistance = 40; 

	protected class Pos{
		int x;
		int y;
	}

	protected int botXPos;
	protected int botYPos;	
	protected double heading;
	protected double pathAngle;
	protected double pathDistance;

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
		//TODO: Set X, Y and heading here
		
		//Calculate point straight ahead
			
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
		ArrayList<Pos> pathPoints = new ArrayList<Pos>();

		//Run Bresenham's
		double deltaX = p.x - botXPos;
		double deltaY = p.y - botYPos;
		double error = 0;
		double deltaError = Math.abs(deltaY / deltaX);

		double y = botYPos;
		for (int x = botXPos; i <= p.x; i++){
			pathPoints.add(x, y);
			error = error + deltaError;
			if (error >= 0.5){
				y += 1;
				error = error - 1;
			}
		}

		//Check path points for barriers
		for (Pos p : pathPoints){
			//Barriers are positive numbers
			if (map[p.x][p.y] > 0){
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




		
