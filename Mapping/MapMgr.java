package Mapping;

import java.util.ArrayList;
import javax.swing.*;
import java.awt.Point;
import java.math.*;
import java.util.HashMap;

import april.util.*;
import april.jmat.*;

public class MapMgr {

	protected HashMap<Point, Integer> map;
    protected Integer UNKNOWN;
	protected Integer BARRIER;
    protected Integer KNOWN;
	protected Integer TRIANGLE;
    protected int RADIUS = 10; // radius of robot
	protected int TRIANGLE_CONVOLVE = 4;

//=================================================================//
// Map                                                             //
//                                                                 //
// Initailizes the size of the robot base and a new map array.     //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public MapMgr() {
		map = new HashMap<Point, Integer>(1000*1000);
        KNOWN = new Integer (0);
        UNKNOWN = new Integer (1);
        BARRIER = new Integer (2);
		TRIANGLE = new Integer (3);
        for (int i=-500; i < 500; i++) {
            for (int j=-500; j < 500; j++) {
                Point pt = new Point (i,j);
                map.put (pt, UNKNOWN);
            }
        }

	}


    public void updateMap (double[] globalPos, double[][] barriers, double[][] triangles) {

        double[][] globalTrans = { {1, 0, globalPos[0] },
                                {0, 1, globalPos[1] },
                                {0, 0, 1} };
        double[][] globalRot = { {Math.cos(globalPos[2]), -Math.sin(globalPos[2]), 0} ,
                                    { Math.sin(globalPos[2]), Math.cos(globalPos[2]), 0} ,
                                        {0, 0, 1} };
        Matrix globalTransMat = new Matrix (globalTrans);
        Matrix globalRotMat = new Matrix (globalRot);


        // bounding view box is 30 cm by 30 cm
        for (float i = -.15F; i < .15F; i+=.01F) {
            for (float j = 0F; j < .30F; j+=.01F) {
                double[][] curLocalCoord = { {j}, {i}, {1} };
                Matrix curLocalCoordMat = new Matrix (curLocalCoord);
                //System.out.println ("looking at local coord" + curLocalCoordMat);

                Matrix rotated = globalRotMat.times (curLocalCoordMat);
                Matrix curGlobalCoordMat = globalTransMat.times (rotated);
                //curGlobalCoordMat = curGlobalCoordMat.times (globalRotMat);


                //System.out.println ("looking at global coords" + curGlobalCoordMat);
                int globalX = (int) (curGlobalCoordMat.get(0,0) * 100);
                int globalY = (int) (curGlobalCoordMat.get(1,0) * 100);
                //System.out.println ("global Coords " + globalX+ " " + globalY);
                Point pt = new Point(globalX, globalY);
                //System.out.println(map.get(pt));
                map.put(pt, KNOWN);

            }

        }

        addBarrier(barriers);
		//addTriangles(triangles);
        //printMap(globalPos);


    }

    // brings barriers closer to robot by 3 in each
    private void thickenBarriers (double[][] pre_barrier) {



    }



//=================================================================//
// addTriangle		                                               //
//                                                                 //
// 	adds triangle locations convolved into the map				   //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//

	private void addTriangle(double[][] triangles) {
		for (int i = 0; i < triangles.length; i++) {
			double[] cur_point = triangles[i];
			int x = (int)cur_point[0]; int y = (int)cur_point[1];
			
			// plot point into map
			for (int j = x - TRIANGLE_CONVOLVE; j < x + TRIANGLE_CONVOLVE; j++) {
				for (int k = y - TRIANGLE_CONVOLVE; k < y + TRIANGLE_CONVOLVE; k++) {
					Point new_point = new Point(i,j);
					map.put(new_point, TRIANGLE);
				}
			}
		}

	}



//=================================================================//
// addBarrier                                                    //
//                                                                 //
// Bresenham's line algorithm to mark an new barrier on the map.   //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	private void addBarrier(double[][] barriers){

        // BarrierMap:
        // NOTE: BarrierMap is in meter
        //      CONVERSION OCCURS HERE
        //


        for (int i = 0; i < barriers.length; i++) {

            double[] pre_barrier = barriers[i];
            int[] barrier = {   (int) (pre_barrier[0] * 100),   //mult 100 = m to cm
                                (int) (pre_barrier[1] * 100),
                                (int) (pre_barrier[2] * 100),
                                (int) (pre_barrier[3] * 100)};

            convolveAndDraw(barrier);

        }

    }

    private void convolveAndDraw (int[] barrier) {

            double deltaX = Math.abs(barrier[2] - barrier[0]);
            double deltaY = Math.abs(barrier[3] - barrier[1]);

            double error = 0;
            if(deltaX == 0)
                deltaX = .000001;

            int start, end, base;
            boolean horizontal = false;
            boolean decreasing = false;
            double deltaError;
            // Width of the robot is ~20cm, therefore any barriers less than 20 cm apart should not be traversable
           // convolve barriers with a 10cm radius circle
            if (deltaX >= deltaY) {
                // barrier is more horizontal than vertical
                if (barrier[0] <= barrier[2]) {
                    start = barrier[0];
                    end = barrier[2];
                    base = barrier[1];
                    if (barrier[1] > barrier[3]) {
                        decreasing = true;
                    }
                } else {
                    start = barrier[2];
                    end = barrier[0];
                    base = barrier[3];
                    if (barrier[3] > barrier[1]) {
                        decreasing = true;
                    }
                }
                horizontal = true;
                deltaError = deltaY / deltaX;

            } else {
                // barrier is more vertical than horizontal
                if (barrier[1] <= barrier[3]) {
                    start = barrier[1];
                    end = barrier[3];
                    base = barrier[0];
                    if (barrier[0] > barrier[2]) {
                        decreasing = true;
                    }
                } else {
                    start = barrier[3];
                    end = barrier[1];
                    base = barrier[2];
                    if (barrier[2] > barrier[0]) {
                        decreasing = true;
                    }
                }
                deltaError = Math.abs(deltaX / deltaY);
            }

            System.out.println (start + " " + end + " " + base);

            // from x_starting position to y_starting position
            for(int x = start-RADIUS; x < end+RADIUS; x++){
                // increase thickness of barrier
                for (int y = base-RADIUS; y < base+RADIUS; y++) {
                    Point pt;
                    if (horizontal) {
                        // set x is x-coord, y is y-coord
                        pt = new Point (x, y);
                    } else {
                        // other way around
                        pt = new Point (y, x);
                    }
                    map.put(pt,BARRIER);
                }

                error += deltaError;
                if(error > .5){
                    if (decreasing == true) {
                        base--;
                    } else {
                        base++;
                    }
                     error = error - 1;
                }

            }



    }


    public void printMap (Matrix globalPos) {
        int X =(int) (globalPos.get(0,0) * 100);
        int Y = (int) (globalPos.get(1,0)*100);
        for (int i=100; i > -100; i--) {
            for (int j=0; j < 150; j++) {
                if (i == Y && j == X)
                    System.out.printf("X");
                else {
                    Point pt = new Point (j,i);
                    System.out.printf("%d",map.get(pt));
                }
            }
            System.out.printf("\n");
        }



    }

	public HashMap<Point, Integer> getMap() {
		return map;
	}



}





