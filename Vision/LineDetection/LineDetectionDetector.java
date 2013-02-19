package Vision.LineDetection;
import Vision.util.*;

import april.jcam.*;
import java.awt.image.BufferedImage;

public class LineDetectionDetector {

	// args
	BufferedImage in;
	BufferedImage out;


	// CONSTRUCTOR METHOD
	public LineDetectionDetector(BufferedImage im, double thresh) {
		in = im;
		grayscale g = new grayscale(im);
		BufferedImage gray = g.getGrayScale();
		binarize b = new binarize(gray, thresh);
		BufferedImage binarized = b.getBinarizedImage();

		out = binarized;

	}
	
	public BufferedImage getProcessedImage() {
		return out;
	}

	

	


}


