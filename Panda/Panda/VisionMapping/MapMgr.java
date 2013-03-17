package Panda.VisionMapping;

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

//=================================================================//
// Map                                                             //
//                                                                 //
// Initailizes the size of the robot base and a new map array.     //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	public MapMgr() {
		map = new HashMap<Point, Integer>(1000*1000);
        UNKNOWN = new Integer (0);
        BARRIER = new Integer (1);
        KNOWN = new Integer (2);
        for (int i=-500; i < 500; i++) {
            for (int j=-500; j < 500; j++) {
                Point pt = new Point (i,j);
                map.put (pt, UNKNOWN);
            }
        }

	}

    public void updateMap (/*BarrierMap bm,*/ Matrix globalPos, double globalTheta) {

        double[][] globalTrans = { {1, 0, globalPos.get(0,0) },
                                {0, 1, globalPos.get(1,0) },
                                {0, 0, 1} };
        double[][] globalRot = { {Math.cos(globalTheta), -Math.sin(globalTheta), 0} ,
                                    { Math.sin(globalTheta), Math.cos(globalTheta), 0} ,
                                        {0, 0, 1} };
        Matrix globalTransMat = new Matrix (globalTrans);
        Matrix globalRotMat = new Matrix (globalRot);


        // bounding view box is 1 m by 1 m
        for (float i = -.50F; i < .50F; i+=.01F) {
            for (float j = 0F; j < 1F; j+=.01F) {
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

        addBarrier();
        printMap(globalPos);


    }

    // brings barriers closer to robot by 3 in each
    private void thickenBarriers (double[][] pre_barrier) {



    }

//=================================================================//
// addBarrier                                                    //
//                                                                 //
// Bresenham's line algorithm to mark an new barrier on the map.   //
//                                                                 //
// Returns: VOID                                                   //
//=================================================================//
	private void addBarrier(/*BarrierMap bm*/){

        // BarrierMap:
        // NOTE: BarrierMap is in meter
        //      CONVERSION OCCURS HERE
        //



        ArrayList<double[][]> realWorldMap = new ArrayList<double[][]>();// = bm.getBarriers();
        double[][] barrier1 = { {0.75, 0.25}, {1.4, -0.25} };
        realWorldMap.add(barrier1);


        for (int i = 0; i < realWorldMap.size(); i++) {

            //thickenBarriers (pre_barrier);

            double[][] pre_barrier = realWorldMap.get(i);
            int[] barrier = {   (int) (pre_barrier[0][0] * 100),   //mult 100 = m to cm
                                (int) (pre_barrier[0][1] * 100),
                                (int) (pre_barrier[1][0] * 100),
                                (int) (pre_barrier[1][1] * 100)};

            double deltaX = Math.abs(barrier[2] - barrier[0]);
            double deltaY = Math.abs(barrier[3] - barrier[1]);

            double error = 0;
            if(deltaX == 0)
                deltaX = .000001;


            int base_y = barrier[1];
            int start, end, base;
            boolean horizontal = false;
            boolean decreasing = false;
            double deltaError;

            if (deltaX >= deltaY) {
                // barrier is more horizontal than vertical
             /*   start = Math.min (barrier[0], barrier[2]);
                end = Math.max (barrier[0], barrier[2]);
                base = Math.min(barrier[1], barrier[3]);
            */
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
              /*  start = Math.min (barrier[1], barrier[3]);
                end = Math.max (barrier[1], barrier[3]);
                base = Math.min(barrier[0], barrier[2]);
            */
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
            for(int x = start-5; x < end+5; x++){
                // increase thickness of barrier
                for (int y = base-5; y < base+5; y++) {
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



}





