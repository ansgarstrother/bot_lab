package Panda.VisionMapping;

import java.util.ArrayList;
import javax.swing.*;
import java.awt.*;
import java.math.*;

public class Map{

	protected int map[][];
	protected final int BARRIER = 1;

//=================================================================//
// Map                                                             //
//                                                                 //
// Initailizes the size of the robot base and a new map array.     //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public void Map(){
		map = new int[1000][1000];

	}

//=================================================================//
// addBarrier                                                    //
//                                                                 //
// Bresenham's line algorithm to mark an new barrier on the map.   //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	private void addBarrier(BarrierMap bm){

        // BarrierMap:
        // NOTE: BarrierMap is in meters
        //      CONVERSION OCCURS HERE
        //
        ArrayList<double[][]> realWorldMap = bm.getBarriers();
        for (int i = 0; i < realWorldMap.size(); i++) {

            double[][] pre_barrier = realWorldMap.get(i);
            int[] barrier = {   (int)pre_barrier[0][0] * 100,   //mult 100 = m to cm
                                (int)pre_barrier[0][1] * 100,
                                (int)pre_barrier[1][0] * 100,
                                (int)pre_barrier[1][1] * 100};

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
    }

}





