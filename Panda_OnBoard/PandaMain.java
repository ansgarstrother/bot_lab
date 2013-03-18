import lcm.lcm.*;
import java.util.Vector;

import Panda.sensors.*;
import Panda.VisionMapping.*;
import Panda.Odometry.*;
import Panda.util.*;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;

import april.jcam.*;
import april.util.*;
import april.jmat.*;

public class PandaMain{

	static boolean run = true;
    static double sampleRate = 200;    //microseconds

//=================================================================//
// main of the panda bot                                           //
//                                                                 //
//=================================================================//
	public static void main(String [] args){

        ArrayList<String> urls = ImageSource.getCameraURLs();

        String url = null;
        if (urls.size()==1)
            url = urls.get(0);

        if (args.length > 0)
            url = args[0];

        if (url == null) {
            System.out.printf("Cameras found:\n");
            for (String u : urls)
                System.out.printf("  %s\n", u);
            System.out.printf("Please specify one on the command line.\n");
            return;
        }

		ImageSource is = null;
		try{is = ImageSource.make(url);}
		catch(Exception e){}

		ImageSourceFormat fmt = is.getCurrentFormat();

		//Read in Calibration of Panda Bot
		Projection projection = new Projection();

        	PandaDrive pDrive = new PandaDrive();

		while(run){

            // Implement Sampling Rate
            //Thread.sleep(sampleRate);?

			//Get a new image
			is.start();
           		byte buf[] = is.getFrame().data;
           		if (buf == null)
        	      		continue;
			BufferedImage im = ImageConvert.convertToImage(fmt.format, fmt.width, fmt.height, buf);
			is.stop();

			//find and add barriers to the maps
			//TODO Needs to add segments it finds to the map via Map Class
			LineDetector lineSeg = new LineDetector(im, 100, 155);
            ArrayList<int[][]> boundaryMap = lineSeg.getSegments();
            //LineDetectionSegmentation lds = new LineDetectionSegmentation(boundaryMap);
            //ArrayList<int[][]> segments = lds.getSegments();
            // rectify
            //Rectification rectifier = new Rectification(segments, im.getWidth(), im.getHeight());
            //ArrayList<int[][]> rectifiedMap = rectifier.getRectifiedPoints();
            /*
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
             */


			//Detect any triangles and then fire on them
			//TODO if a target is found turn and fire on the target then return to
			//orginal position
			TargetDetector Tdetector = new TargetDetector(im);

			//plan path and move robot
			// TODO create a class to idenify where we are going next check path
			// via map class then order robot ot move.

			pDrive.driveForward(1);
		}
	}
}
