package Panda.VisionMapping;

import Panda.util.*;
import java.io.*;

import lcmtypes.*;
import lcm.lcm.*;

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
    private double[] calibrationMatrix;
	private int turn;
    //public map_t map_msg;

    private PandaPositioning pp;

	public BarrierMap( BufferedImage image, double[] cm, PandaPositioning pp ) {

		this.calibrationMatrix = cm;
		System.out.println("starting rectification");		
        this.turn = 0;	

			// detect and retrieve boundary map
			System.out.println("creating detector");
        	detector = new LineDetectionDetector(image, binaryThresh, lwPassThresh);
        	boundaryMap = detector.getBoundaryMap();

			int count_left = 0;
			for (int i = 400; i < 464; i++) {
				count_left = count_left + boundaryMap[i];
			}

			int count_right = 0;
			for (int i = 832; i < 896; i++) {
				count_right = count_right + boundaryMap[i];
			}
			
			int left_mean = count_left / 64;
			int right_mean = count_right / 64;

			System.out.println("STRAIGHT " + left_mean + "    " + right_mean);

			if (left_mean < 650 && right_mean < 650) {
				turn =  0;
			}
			else{
				turn = 1;
			}
			

			/*
			else if (right_mean > 700 && right_mean > left_mean) {
				turn = 2; // turn left
			}
			else if (left_mean > 700 && left_mean > right_mean) {
				turn = 1; // turn right
			}
			else if (right_mean < left_mean) {
				turn = 2;
			}
			else {
				turn = 1;
			}*/
				
				
	}
    public ArrayList<double[][]> getBarriers() {
        // Method returns real world map
        return realWorldMap;
    }
	public int getTurn() {
		return turn;
	}

}
