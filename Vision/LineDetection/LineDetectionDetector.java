package Vision.LineDetection;
import Vision.util.*;

import april.jcam.*;
import java.awt.image.BufferedImage;

public class LineDetectionDetector {

	// args
	BufferedImage in;
	BufferedImage out;


	// CONSTRUCTOR METHOD
	public LineDetectionDetector(BufferedImage im, double thresh, double lwpass) {
		in = im;
		out = im;
		grayscale g = new grayscale(im);
		out = g.getGrayScale();
		binarize b = new binarize(out, thresh);
		out = b.getBinarizedImage();
		//colored_binarize cb = new colored_binarize(out, thresh, lwpass);
		//out = cb.getBinarizedImage();


		//sobel sb = new sobel(blurred);
		//BufferedImage edges = sb.detectEdges();
		sobel2 sb = new sobel2();
		sb.init(out);
		out = sb.process();

	}
	
	public BufferedImage getProcessedImage() {
		return out;
	}

	

	


}


