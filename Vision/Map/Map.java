package Vision.LineDetection;
import javax.swing.*;
import java.awt.*;
import java.math.*;

public class Map{

	int map[][];

	int BASE;
	final int BARRIER = 1;

//=================================================================//
// Map                                                             //
//                                                                 //
// Initailizes the size of the robot base and a new map array.     //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public void Map(int size){
		map = new int[1000][1000];
		BASE = size;

	}

//=================================================================//
// addBarrier                                                    //
//                                                                 //
// Bresenham's line algorithm to mark an new barrier on the map.   //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public void addBarrier(int[] barrier){
		double deltaX = barrier[2] - barrier[0];
		double deltaY = barrier[3] - barrier[1];

		double error = 0;
		if(deltaX == 0)
			deltaX = .000001;

		double deltaError = Math.abs(deltaY / deltaX);

		int y = barrier[1];

		for(int x = barrier[0]; x < barrier[2]; x++){
			map[x][y] = BARRIER;
			error += deltaError;
			if(error > .5){
				y++;
				error = error - 1;
			}
		}
	}

//=================================================================//
// BarrierCheck                                                    //
//                                                                 //
// Checks if possible robot path will come across a barrier. Uses  //
// Bresenham's line algorithm to check array locations that fall   //
// on the path, only checks it the right and left edge will be     // 
// clear                                                           //
//                                                                 //
// Returns: TRUE if no barriers found in path                      //
//          FALSE if a barrier is found in R / L path              //
//=================================================================//
	public boolean BarrierCheck(int[] barrier){
		
		//calculates slope
		double deltaX = barrier[2] - barrier[0];
		double deltaY = barrier[3] - barrier[1];
		
		//calcualtes offset for R and L paths
		double angle = Math.atan2(deltaX,deltaY);
		int offsetX = BASE * (int) Math.cos(angle);
		int offsetY = BASE * (int) Math.sin(angle);
		
		double error = 0;

		if(deltaX == 0)
			deltaX = .000001;
		double deltaError = Math.abs(deltaY / deltaX);

		int y = barrier[1];

		for(int x = barrier[0]; x < barrier[2]; x++){
			//right side check
			if(map[x + offsetX][y + offsetY] == BARRIER)
				return false;

			//left side check
			if(map[x - offsetX][y - offsetY] == BARRIER)
				return false;

			error += deltaError;
			if(error > .5){
				y++;
				error = error - 1;
			}
		}
		return true;
	}
}



		
		
