package Vision.LineDetection;
import Vision.util.*;

import april.jcam.*;
import java.awt.image.BufferedImage;

public class LineDetectionDetector {

	// args
	BufferedImage in;
	BufferedImage out;


	// CONSTRUCTOR METHOD
	public LineDetectionDetector(BufferedImage im, double thresh, float radius) {
		in = im;
		grayscale g = new grayscale(im);
		BufferedImage gray = g.getGrayScale();
		binarize b = new binarize(gray, thresh);
		BufferedImage binarized = b.getBinarizedImage();
		//GaussianFilter gf = new GaussianFilter(radius);
		//BufferedImage blurred = binarized;
		//gf.filter(binarized, blurred);
		//sobel sb = new sobel(blurred);
		//BufferedImage edges = sb.detectEdges();
		//sobel2 sb = new sobel2();
		//sb.init(blurred);
		//BufferedImage edges = sb.process();

		out = binarized;

	}
	
	public BufferedImage getProcessedImage() {
		return out;
	}

	

	


}


