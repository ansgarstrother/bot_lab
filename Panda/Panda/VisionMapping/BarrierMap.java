package Panda.VisionMapping;

import Panda.util.*;

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
    
    // args
    private LineDetectionDetector   detector;
    
    // PIXEL COORDINATE LOCATIONS
    private int[] boundaryMap;
    private ArrayList<int[][]> rectifiedMap;    // rectified boundary map
    private ArrayList<double[][]> realWorldMap;    // in real world coordinates
    
    // constants
    final static double binaryThresh = 155;
    final static double lwPassThresh = 100;
	private final static double f = -269.30751168;
	private final static double c_x = 560.081029;
	private final static double c_y = 280.264223576;
    private double[][] calibrationMatrix =
		{ 	{f, 0, c_x, 0}, {0, f, c_y, 0}, {0, 0, 1, 0}	};
    
    
	public BarrierMap( BufferedImage im ){
        
        // LINE DETECTION ALGORITHM
        // detect and retrieve boundary map
        detector = new LineDetectionDetector(im, binaryThresh, lwPassThresh);
        boundaryMap = detector.getBoundaryMap();
        
        // segment points
        LineDetectionSegmentation lds = new LineDetectionSegmentation(boundaryMap);
        ArrayList<int[][]> segments = lds.getSegments();
        
        // rectify
        Rectification rectifier = new Rectification(segments, im.getWidth(), im.getHeight());
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
              
        
    }
    
    public ArrayList<double[][]> getBarriers() {
        // Method returns real world map
        return realWorldMap;
    }

}