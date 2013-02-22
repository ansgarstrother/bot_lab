package Vision.TargetDetection;


import java.awt.image.BufferedImage;

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
				//float[] cHSV = Color.RGBtoHSV		// CONVERT TO HSV
				int cRGB = im.getRGB(i,j);
				int rRGB = im.getRGB(i+1,j);
				int dRGB = im.getRGB(i,j+1);
				boolean current_is_green = false;
				int cR = cRGB >> 16 & 0xff;
				int cG = cRGB >> 8 & 0xff;
				int cB = cRGB & 0xff;
				double total = cR + cG + cB;
				if (cG < cR | cG < cB) {
				
				}
				else if ((double)cR / total > threshold) {
					// object has too much red
				}
				else if ((double)cB / total > threshold) {
					// object has too much blue
				}
				else if ((double)cG / total > threshold && cG > cB && cG > cR) {
					current_is_green = true;
					im.setRGB(i,j,0xff00ff00);	// GREEN
				}
			}
		}
				




	
	}
	

	// PUBLIC METHODS
	public BufferedImage getImage() {
		return im;
	}

	
	





}
