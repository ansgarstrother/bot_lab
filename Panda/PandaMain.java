import lcm.lcm.*;
import java.util.Vector;

import Vision.*;

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

		ImageSource is;
		try{is = ImageSource.make(url);}
		catch(Exception e){}
		
		ImageSourceFormat fmt = is.getCurrentFormat();

		//Read in Calibration of Panda Bot
		Projection projection = new Projection();

		while(run){
			is.start();
            // read a frame
           	byte buf[] = is.getFrame().data;
           		if (buf == null)
        	      		continue;
		BufferedImage im = ImageConvert.convertToImage(fmt.format, fmt.width, fmt.height, buf);
			is.stop();

			//get a new image

			//find and add barriers to the maps
			PandaLineSegmentation lineSeg = new PandaLineSegmentation(im, 100, 155);
			lineSeg.traverse();

			/*
			//Detect any triangles and then fire on them
			PandaTargetDetector Tdetector = new PandaTargetDetector(im);
			Tdetector.runDetection();
			Targets = Tdetector.GetTriangleLocation();
			
			while( ! Target.isEmpty()){
				//align();
				//fireLaser();
			}*/

			//plan path and move robot
			//Move();
		}
	}
}
