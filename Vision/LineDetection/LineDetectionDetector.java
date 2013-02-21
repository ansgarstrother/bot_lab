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
		out = im;
		grayscale g = new grayscale(im);
		out = g.getGrayScale();
		binarize b = new binarize(out, thresh);
		out = b.getBinarizedImage();
		//sobel sb = new sobel(blurred);
		//BufferedImage edges = sb.detectEdges();
		//sobel2 sb = new sobel2();
		//sb.init(out);
		//out = sb.process();

	}
	
	public BufferedImage getProcessedImage() {
		return out;
	}

	

	


}


