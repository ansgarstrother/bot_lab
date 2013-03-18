package Panda.Targeting;


import Panda.Odometry.*;
import Panda.Targeting.*;
import Panda.VisionMapping.*;
import java.awt.image.BufferedImage;
import java.awt.Color;
import java.util.Vector;
import java.util.ArrayList;

import april.jmat.Matrix;

public class TargetDetector {

	//Pixel area of triangle must be greater then this size to be counted
    static final int BLOB_SIZE_CONSTANT = 200;
    //Triangle must be within 100 pixels of half of its bounding box
    static final int TRI_HALF_BOX_THRES_CONSTANT = 100;
	// close proximity test
	static final int Y_THRESH = 500;
	static final int CLOSE_COUNT_THRESH = 600;

	PandaDrive drive;
	PandaPositioning pp;

	BufferedImage im;
	BufferedImage out;	// bounding box w/center at triangle
	int width;
	int height;
	int delimiter;		// restricts height
	double threshold;	// RGB threshold

	double[] calibrationMatrix;

	ArrayList<Float> triangles;	// vector of triangle pixel points

    int group = 1;

    class Stats{
        int centerX;
        int centerY;
        int count;
        int maxX;
        int maxY;
        int minX;
        int minY;
    }

    Vector<Stats> stats = new Vector<Stats>();
    
	Destroy destroy;

	// CONSTRUCTOR METHOD
	public TargetDetector(PandaDrive pd, double[] cm, PandaPositioning pp) {
		this.drive = pd;
		this.calibrationMatrix = cm;
		this.pp = pp;
       	delimiter = 150;
		threshold = .7;
		destroy = new Destroy(pd);
		triangles = new ArrayList<Float>();
    }

    void expand(int[][] green, int i, int j){
       	
		if (j < 2 || j > height -2)
			return;
		if (i < 2 || i > width -2)
			return;


        green[i][j] = group;
        Stats s = stats.get(group-1);
        s.count++;
        stats.set(group-1, s);

	

    	if(s.count > 1000){
            return;
        }

	   if(green[i+1][j] == 0){
	        int cRGB = im.getRGB(i+1,j);

            // convert to HSV
		    float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);
		    //System.out.println(cHSV[0]);
		    if (cHSV[0] > 0.15 && cHSV[0] < 0.4 && cHSV[1] > 0.4 && cHSV[2] > 0.3) {

                s = stats.get(group-1);
                if(i+1 > s.maxX){
                    s.maxX = i+1;
                    stats.set(group-1, s);
                }
                expand(green, i+1, j);
            }
        }

        if(green[i-1][j] == 0){

            int cRGB = im.getRGB(i-1,j);

            // convert to HSV
		    float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);
		    //System.out.println(cHSV[0]);
	    	if (cHSV[0] > 0.15 && cHSV[0] < 0.4 && cHSV[1] > 0.4 && cHSV[2] > 0.3) {

                s = stats.get(group-1);
                if(i-1 < s.minX){
                    s.minX = i-1;
                    stats.set(group-1, s);
                }
                expand(green, i-1, j);
            }
        }


        if(green[i][j+1] == 0){

            int cRGB = im.getRGB(i,j+1);

            // convert to HSV
		    float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);
	    	//System.out.println(cHSV[0]);
		    if (cHSV[0] > 0.15 && cHSV[0] < 0.4 && cHSV[1] > 0.4 && cHSV[2] > 0.3) {

                s = stats.get(group-1);
                if(j+1 > s.maxY){
                    s.maxY = j+1;
                    stats.set(group-1, s);
                }
                expand(green, i, j+1);
            }
        }


        if(green[i][j-1] == 0){

            int cRGB = im.getRGB(i,j-1);

             // convert to HSV
	    	float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);
	    	//System.out.println(cHSV[0]);
		    if (cHSV[0] > 0.15 && cHSV[0] < 0.4 && cHSV[1] > 0.4 && cHSV[2] > 0.3) {

                s = stats.get(group-1);

                if(j-1 < s.minY){
                    s.minY = j-1;
                    stats.set(group-1, s);
                }
                expand(green, i, j-1);
            }
        }



    }


	// PROTECTED ETHODS
	public void runDetection(BufferedImage in) {
		// Union Find searching for pixels of high green intensity
		// Traverse Entire Image
		//	a. Retrieve current pixel, right pixel, and down pixel
		//		- current pixel must pass threshold test for the following to occur:
		//	b. If right pixel passes threshold test
		//		1. if parent to right pixel == self, then update to current pixel
		//		2. else if parent to right pixel != cur_pixel, trace parent back and link roots to cur_pixel
		//	c. Repeat with down pixel
		// Build Hash Table of stats useful to detect a triangle
		// Traverse Entire Image
		//	a. For each pixel, find root node and update stats in hash map
		// For all root nodes in hash map
		//	a. Build a covariance matrix and use stats to determine shape of pixel blob

		im = in;
		width = in.getWidth();
		height = in.getHeight();

		group = 1;

		int green[][] = new int[width][height];
		for (int j = height/2; j < height - delimiter; j += 10) {
			for (int i = (j%10); i < width - 1; i += 40) {
				int cRGB = im.getRGB(i,j);
				// convert to HSV
				float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);
				//System.out.println(cHSV[0]);
				if (cHSV[0] > 0.15 && cHSV[0] < 0.4 && cHSV[1] > 0.4 && cHSV[2] > 0.3) {
			            Stats temp = new Stats();
			            temp.minX = 1000000;
			            temp.minY = 1000000;
						temp.count = 0;
			            stats.add(temp);
			            expand(green, i, j);
			            group++;
			        }
			}
		}

		int count = 0;
		Float theta = 0.0F;
		for(Stats s : stats){
            if(s.count > 50){
	             //System.out.println("Target level 1");
   
             if( s.maxX - s.minX  < 1.2 * (s.maxY - s.minY) &  s.maxX - s.minX  > .3 * (s.maxY - s.minY)){
                // System.out.println("Target level 2");
   				   	double area = (s.maxX - s.minX)  * (s.maxY - s.minY) ;
                	System.out.println(area + "     " + s.count);
   
                if( (.3 * area < s.count & .7 * area > s.count)){

						double[] center = new double[2];
						center[0] = (s.maxX + s.minX)/2; 
						center[1] = (s.minY + s.maxY)/2;
            		
						// transform points
            			// Ray Projection Implementation
            			Matrix vec = pp.getLocalPoint(calibrationMatrix, center, false);
 
            			// add real world coordinate
           		 		double[] coord = {vec.get(0,0), vec.get(1,0), vec.get(2,0)};
            			theta =  (float)Math.atan2(coord[1], coord[0]);
						theta *= (180.0F/3.145F);
						theta = theta;
						destroy.fire(-theta);		
				}

            			//triangles.add(-theta);
             
            }
         }
     }

	//destroy.fireZeMissiles(triangles);
   }


	// HELPER FUNCTIONS
	protected float[] RGBtoHSV(int r, int g, int b) {
		float[] HSV = new float[3];
		Color.RGBtoHSB(r,g,b,HSV);
		return HSV;
	}


	// PUBLIC METHODS
	public BufferedImage getImage() {
		return im;
	}
}
