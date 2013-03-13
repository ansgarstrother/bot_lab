package Panda.util;

import java.awt.image.BufferedImage;

public class edgeDetection {
	
	// args
	BufferedImage in;
	int height;
	int width;
	int[] boundaryMap;

	public edgeDetection(BufferedImage im) {
		in = im;
		height = im.getHeight();
		width = im.getWidth();
		boundaryMap = new int[width];	// index = x coord, value = y coord
		detectEdges();
	}

	protected void detectEdges() {
		// Scan image going up, left to right
		// Search for solid black pixels (5 after first encounter
		// Set pixel to white and move to next column
		// White pixels denote boundary
		for (int i = 0; i < width; i++) {
			int x = i;
			for (int j = 0; j < (height/2) + 100; j++) {
				int y = height - 1 - j;
				int rgb = in.getRGB(x,y);
				int r = (rgb >> 16) & 0xff;
				int g = (rgb >> 8) & 0xff;
				int b = rgb & 0xff;
				if ((r + g + b) == 0) {		// black pixel
					in.setRGB(x,y,0xffffffff);
					in.setRGB(x,y-1,0xffffffff);	// add thickness
					boundaryMap[x] = y;
					break;
				}
			}
		}
					
				
	}

	public BufferedImage getImage() {
		return in;
	}
	public int[] getBoundaryMap() {
		return boundaryMap;
	}

}
