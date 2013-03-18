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
    protected int RADIUS = 10; // radius of robot

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

    public HashMap<Point, Integer> getMap() {
        return this.map;


    }


    public void updateMap (BarrierMap bm, Matrix globalPos, double globalTheta) {

        double[][] globalTrans = { {1, 0, globalPos.get(0,0) },
                                {0, 1, globalPos.get(1,0) },
                                {0, 0, 1} };
        double[][] globalRot = { {Math.cos(globalTheta), -Math.sin(globalTheta), 0} ,
                                    { Math.sin(globalTheta), Math.cos(globalTheta), 0} ,
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

        addBarrier(bm);
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
	private void addBarrier(BarrierMap bm){

        // BarrierMap:
        // NOTE: BarrierMap is in meter
        //      CONVERSION OCCURS HERE
        //



        ArrayList<double[][]> realWorldMap = bm.getBarriers();
        //realWorldMap = new ArrayList<double[][]>();
        //double[][] barrier1 = { {0, 1}, {0.25, 0.5} };
        //double[][] barrier2 = { {.55, 0.2}, {1, 0} };
        //double[][] barrier2 = { {
        //realWorldMap.add(barrier1);
        //realWorldMap.add(barrier2);


        for (int i = 0; i < realWorldMap.size(); i++) {


            double[][] pre_barrier = realWorldMap.get(i);
            int[] barrier = {   (int) (pre_barrier[0][0] * 100),   //mult 100 = m to cm
                                (int) (pre_barrier[0][1] * 100),
                                (int) (pre_barrier[1][0] * 100),
                                (int) (pre_barrier[1][1] * 100)};

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



}





