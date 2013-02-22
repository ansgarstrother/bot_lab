package Vision.TargetDetection;

import java.util.ArrayList;

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

	// Statistical Analysis
	int sum_x;
	int sum_y;
	int n;
	int mean_x;
	int mean_y;

	

	// CONSTRUCTOR METHOD
	public TargetDetectionDetector(BufferedImage in, int delimiter, double thresh) {
		im = in;

		width = in.getWidth();
		height = in.getHeight();
		this.delimiter = delimiter;
		threshold = thresh;

		sum_x = 0;
		sum_y = 0;
		mean_x = 0;
		mean_y = 0;
		n = 0;
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
		for (int j = (height/2); j < height - delimiter; j++) {
			for (int i = 0; i < width - 1; i++) {
				int cRGB = im.getRGB(i,j);
				int rRGB = im.getRGB(i+1,j);
				int dRGB = im.getRGB(i,j+1);
				// convert to HSV
				float[] cHSV = RGBtoHSV(cRGB >> 16 & 0xff, cRGB >> 8 & 0xff, cRGB & 0xff);
				float[] rHSV = RGBtoHSV(rRGB >> 16 & 0xff, rRGB >> 8 & 0xff, rRGB & 0xff);
				float[] dHSV = RGBtoHSV(dRGB >> 16 & 0xff, dRGB >> 8 & 0xff, dRGB & 0xff);
				//System.out.println(cHSV[0]);
				if (cHSV[0] > 0.15 && cHSV[0] < 0.7 && cHSV[1] > 0.5 && cHSV[2] > 0.3) {
					// Check next 5
					boolean blob = true;
					for (int c = 1; c < 5; c++) {
						int next_RGB = im.getRGB(i+c,j);
						float[] next_HSV = RGBtoHSV(next_RGB >> 16 & 0xff, 
										next_RGB >> 8 & 0xff, 
										next_RGB & 0xff);
						if (next_HSV[0] > 0.15 && next_HSV[0] < 0.7 && 
							next_HSV[1] > 0.4 && next_HSV[2] > 0.3) {
							blob = true;
						}
						else {
							blob = false;
							break;
						}
					}
					if (blob) {
						im.setRGB(i,j,0xff00ff00);
						n = n + 1;
						sum_x = sum_x + i;
						sum_y = sum_y + j;
					}
				}

				

				boolean current_is_green = false;
				
			}
		}
		/*	
		// Get mean, plot mean
		mean_x = (int) sum_x / n;
		mean_y = (int) sum_y / n;
		for (int i = mean_x - 2; i < mean_x + 2; i++) {
			for (int j = mean_y - 2; j < mean_y + 2; j++) {
				im.setRGB(i,j,0xffff0000);
			}
		}
		*/


	
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
