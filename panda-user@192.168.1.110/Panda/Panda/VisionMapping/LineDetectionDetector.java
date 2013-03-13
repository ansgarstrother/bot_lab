package Panda.VisionMapping;

import Panda.util.*;

import april.jcam.*;
import java.awt.image.BufferedImage;
import java.io.*;

public class LineDetectionDetector {
    
	// args
	BufferedImage in;
	BufferedImage out;
	int[] boundaryMap;
    
    
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
		
		edgeDetection ed = new edgeDetection(out);
		out = ed.getImage();
        
		//sobel sb = new sobel(blurred);
		//BufferedImage edges = sb.detectEdges();
		//sobel2 sb = new sobel2();
		//sb.init(out);
		//out = sb.process();
        
        boundaryMap = ed.getBoundaryMap();
        /*
         // save boundary map to txt file
         boundaryMap = ed.getBoundaryMap();
         try {
         FileWriter fstream = new FileWriter("color10.txt");
         BufferedWriter out = new BufferedWriter(fstream);
         for (int i = 0; i < boundaryMap.length; i++) {
         out.write(i + ", " + boundaryMap[i] + "\n");
         }
         out.close();
         }
         catch (Exception e) {
         System.err.println("Error: " + e.getMessage());
         }
         */
		
        
	}
	
	public BufferedImage getProcessedImage() {
		return out;
	}
	public int[] getBoundaryMap() {
		return boundaryMap;
	}
    
	
    
	
    
    
}


