package Vision.TargetDetection;


import java.awt.image.BufferedImage;
import java.awt.Color;
import java.util.Vector;

public class TargetDetectionDetector {

	// args
    static final int BLOB_SIZE_CONSTANT = 100;

	BufferedImage im;
	BufferedImage out;	// bounding box w/center at triangle
	int width;
	int height;
	int delimiter;		// restricts height
	double threshold;	// RGB threshold

    int group = 1;

    class Stats{
        int count;
        int maxX;
        int maxY;
        int minX;
        int minY;
    }

    Vector<Stats> stats = new Vector<Stats>();

	// CONSTRUCTOR METHOD
	public TargetDetectionDetector(BufferedImage in, int delimiter, double thresh) {
		im = in;

		width = in.getWidth();
		height = in.getHeight();
		this.delimiter = delimiter;
		threshold = thresh;
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
		    //System.out.println(cHSV[0]);
		    if (cHSV[0] > 0.15 && cHSV[0] < 0.4 && cHSV[1] > 0.4 && cHSV[2] > 0.3) {

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
		    //System.out.println(cHSV[0]);
	    	if (cHSV[0] > 0.15 && cHSV[0] < 0.4 && cHSV[1] > 0.4 && cHSV[2] > 0.3) {
                
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
		    if (cHSV[0] > 0.15 && cHSV[0] < 0.4 && cHSV[1] > 0.4 && cHSV[2] > 0.3) {

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
	    	//System.out.println(cHSV[0]);
		    if (cHSV[0] > 0.15 && cHSV[0] < 0.4 && cHSV[1] > 0.4 && cHSV[2] > 0.3) {

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

        int green[][] = new int[width][height];
		for (int j = (height/2); j < height - delimiter; j += 10) {
			for (int i = (j%10); i < width - 1; i += 40) {
				int cRGB = im.getRGB(i,j);
				// convert to HSV
				float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);
				//System.out.println(cHSV[0]);
				if (cHSV[0] > 0.15 && cHSV[0] < 0.4 && cHSV[1] > 0.4 && cHSV[2] > 0.3) {
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
                mark(im, bounds, 0xffff0000);
                System.out.print("MinY: ");
                System.out.println(s.minY);
                System.out.print("MinY: ");
                System.out.println(s.minY);
                System.out.print("MinX: ");
                System.out.println(s.minX);
                System.out.print("MaxY: ");
                System.out.println(s.maxY);
                System.out.print("MaxX: ");
                System.out.println(s.maxX);
            }
        }




	}
	
	public void mark(BufferedImage img, int[] bounds, int color){
        // draw the horizontal lines
        for (int x = bounds[0]; x <=bounds[2]; x++) {
            img.setRGB(x,bounds[1], color); //Go Blue!
            img.setRGB(x,bounds[3], color); //Go Blue!
        }

        // draw the horizontal lines
        for (int y = bounds[1]; y <=bounds[3]; y++) {
            img.setRGB(bounds[0],y, color); //Go Blue!
            img.setRGB(bounds[2],y, color); //Go Blue
        }
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
