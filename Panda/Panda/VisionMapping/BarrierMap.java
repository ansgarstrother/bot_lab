package Panda.VisionMapping;

import Vision.util.*;
import Vision.calibration.*;

import java.io.*;

import javax.imageio.ImageIO;
import javax.swing.*;
import javax.swing.filechooser.FileNameExtensionFilter;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseAdapter;
import java.awt.image.BufferedImage;
import java.awt.Graphics2D;
import java.awt.Point;
import java.util.ArrayList;
import java.util.List;
import java.io.IOException;
import java.awt.geom.*;

import april.jcam.ImageSource;
import april.jcam.ImageConvert;
import april.jcam.ImageSourceFormat;
import april.jcam.ImageSourceFile;
import april.jmat.Matrix;

public class BarrierMap{

	public static BarrierMap( BufferedImage im ){


// Image Processing
    protected BufferedImage processImage(BufferedImage image) {
        // detect and retrieve boundary map
        detector = new LineDetectionDetector(image, binaryThresh, lwPassThresh);
        boundaryMap = detector.getBoundaryMap();
        
        // segment points
        LineDetectionSegmentation lds = new LineDetectionSegmentation(boundaryMap);
        ArrayList<int[][]> segments = lds.getSegments();
        
        // rectify
        Rectification rectifier = new Rectification(segments, image.getWidth(), image.getHeight());
        rectifiedMap = rectifier.getRectifiedPoints();
      
        // transform to real world coordiantes
        // from calibrationMatrix
        // coordinates should now be in 3D
        realWorldMap = new ArrayList<double[][]>();
        Matrix calibMat = new Matrix(calibrationMatrix);
        for (int i = 0; i < rectifiedMap.size(); i++) {
            int[][] segment = rectifiedMap.get(i);
	    double[][] init_point = {{segment[0][0]}, {segment[0][1]}, {1}};
	    double[][] fin_point = {{segment[1][0]}, {segment[1][1]}, {1}};
	
	    Matrix init_pixel_mat = new Matrix(init_point);
	    Matrix fin_pixel_mat = new Matrix(fin_point);
	    Matrix affine_vec = calibMat.transpose()
					.times(calibMat)
					.inverse()
					.times(calibMat.transpose());
	    Matrix init_vec = calibMat.times(init_pixel_mat);
	    Matrix fin_vec = calibMat.times(fin_pixel_mat);
	

	    // add real world coordinate
	    double[][] real_segment = {{init_vec.get(0,0), init_vec.get(0,1), init_vec.get(0,2)},
					{fin_vec.get(0,0), fin_vec.get(0,1), fin_vec.get(0,2)}};
	    realWorldMap.add(real_segment);

	
        }
     
        return detector.getProcessedImage();
    }

}
