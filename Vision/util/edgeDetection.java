package Vision.util;

import java.awt.image.BufferedImage;

public class edgeDetection {
	
	// args
	BufferedImage in;
	int height;
	int width;

	public edgeDetection(BufferedImage im) {
		in = im;
		height = im.getHeight();
		width = im.getWidth();
	}

}
