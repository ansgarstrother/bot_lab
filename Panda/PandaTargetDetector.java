package Panda;

import java.awt.image.BufferedImage;
import java.awt.Color;
import java.util.Vector;

public class PandaTargetDetector {

	//Pixel area of triangle must be greater then this size to be counted
    static final int BLOB_SIZE_CONSTANT = 150;
    //Triangle must be within 100 pixels of half of its bounding box
    static final int TRI_HALF_BOX_THRES_CONSTANT = 100;
    
	// HSV LIMITS

	//TODO need to actually find out what these values are
	static double HLB = 0;		// hue lower bound
	static double HUB = 50;		// hue upper bound
	static double SLB = 0;		// saturation lower bound
	static double VLB = 50;

	BufferedImage im;
	BufferedImage out;	// bounding box w/center at triangle
	int width;
	int height;

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
    Vector<Stats> finalTriVec = new Vector<Stats>();

	// CONSTRUCTOR METHOD
	public PandaTargetDetector(BufferedImage in) {
		im = in;
		width = in.getWidth();
		height = in.getHeight();
       	group = 1;
		runDetection();
	}

    void expand(int[][] green, int i, int j){
        im.setRGB(i,j,0xff00ff00);
        green[i][j] = group;
        Stats s = stats.get(group-1);
        s.count++;
        stats.set(group-1, s);

        if(green[i+1][j] == 0){
	        int cRGB = im.getRGB(i+1,j);

            // convert to HSV
		    float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);

		    if (cHSV[0] > HLB && cHSV[0] < HUB && cHSV[1] > SLB && cHSV[2] > VLB) {

                s = stats.get(group-1);
                if(i+1 > s.maxY){
                    s.maxY = i+1;
                    stats.set(group-1, s);
                }
                expand(green, i+1, j);
            }
        }

        if(green[i-1][j] == 0){

            int cRGB = im.getRGB(i-1,j);

            // convert to HSV
		    float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);
	
	    	if (cHSV[0] > HLB && cHSV[0] < HUB && cHSV[1] > SLB && cHSV[2] > VLB) {                
              
				s = stats.get(group-1);
                if(i+1 < s.minY){
                    s.minY = i-1;
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
		    if (cHSV[0] > HLB && cHSV[0] < HUB && cHSV[1] > SLB && cHSV[2] > VLB) {

                s = stats.get(group-1);
                if(j+1 > s.maxX){
                    s.maxX = j+1;
                    stats.set(group-1, s);
                }
                expand(green, i, j+1);
            }
        }


        if(green[i][j-1] == 0){

            int cRGB = im.getRGB(i,j-1);

             // convert to HSV
	    	float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);

		    if (cHSV[0] > HLB && cHSV[0] < HUB && cHSV[1] > SLB && cHSV[2] > VLB) {

                s = stats.get(group-1);
                
                if(j-1 < s.minX){
                    s.minX = j-1;
                    stats.set(group-1, s);
                }
                expand(green, i, j-1);
            }
        }



    }


	// PROTECTED METHODS
	protected void runDetection() {

        int green[][] = new int[width][height];

		for (int j = (height/2); j < height; j += 10) {
			for (int i = (j%10); i < width - 1; i += 40) {

				int cRGB = im.getRGB(i,j);

				// convert to HSV
				float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);

				if (cHSV[0] > HLB && cHSV[0] < HUB && cHSV[1] > SLB && cHSV[2] > VLB) {
                    Stats temp = new Stats();
                    temp.minX = 1000000;
                    temp.minY = 1000000;
                    stats.add(temp);
                    expand(green, i, j);
                    group++;
                }
			}
		}

        for (Stats s : stats){
            if (s.count > BLOB_SIZE_CONSTANT){
                int [] bounds = {s.minY, s.minX, s.maxY, s.maxX};
                s.centerX = (s.maxX - s.minX) / 2;
                s.centerY = (s.maxY - s.minY) / 2;

                //Check if blob is approx half the box size to be added to finalTriVec
                int approxTriangleSize = ((s.maxX - s.minX) * (s.maxY - s.minY)) / 2;
                int minTriangleSize = approxTriangleSize - TRI_HALF_BOX_THRES_CONSTANT;
                int maxTriangleSize = approxTriangleSize - TRI_HALF_BOX_THRES_CONSTANT;
                if ((s.count < maxTriangleSize) && (s.count > minTriangleSize)){
                    finalTriVec.add(s);
                }
            }
        }




	}
	
	protected float[] RGBtoHSV(int r, int g, int b) {
		float[] HSV = new float[3];
		Color.RGBtoHSB(r,g,b,HSV);
		return HSV;
	}


	// PUBLIC METHODS
	public BufferedImage getImage() {
		return im;
	}

    public Vector<Stats> GetTriangleLocations(){
        return finalTriVec;
    }


}
