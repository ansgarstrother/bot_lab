package Vision.TargetDetection;


import java.awt.image.BufferedImage;
import java.awt.Color;

public class TargetDetectionDetector {

	// args
	BufferedImage im;
	BufferedImage out;	// bounding box w/center at triangle
	int width;
	int height;
	int delimiter;		// restricts height
	double threshold;	// RGB threshold

	

	// CONSTRUCTOR METHOD
	public TargetDetectionDetector(BufferedImage in, int delimiter, double thresh) {
		im = in;

		width = in.getWidth();
		height = in.getHeight();
		this.delimiter = delimiter;
		threshold = thresh;
		runDetection();
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
		for (int j = 0; j < height - delimiter; j++) {
			for (int i = 0; i < width - 1; i++) {
				int cRGB = im.getRGB(i,j);
				int rRGB = im.getRGB(i+1,j);
				int dRGB = im.getRGB(i,j+1);
				// convert to HSV
				float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);
				float[] rHSV = RGBtoHSV(rRGB >> 16 & 0xff, rRGB >> 8 & 0xff, rRGB & 0xff);
				float[] dHSV = RGBtoHSV(dRGB >> 16 & 0xff, dRGB >> 8 & 0xff, dRGB & 0xff);
				//System.out.println(cHSV[0]);
				if (cHSV[0] > 0.2 && cHSV[0] < 0.6 && cHSV[1] > 0.7 && cHSV[2] > 0.2) {
					im.setRGB(i,j,0xff00ff00);
				}

				

				boolean current_is_green = false;
				
			}
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
