import lcm.lcm.*;
import java.util.Vector;


import Panda.VisionMapping.*;
import Panda.Odometry.*;

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

		ImageSource is = null;
		try{is = ImageSource.make(url);}
		catch(Exception e){}

		ImageSourceFormat fmt = is.getCurrentFormat();

		//Read in Calibration of Panda Bot
		Projection projection = new Projection();

        PandaDrive pDrive = new PandaDrive();

		while(run){

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

			//Detect any triangles and then fire on them
			//TODO if a target is found turn and fire on the target then return to
			//orginal position
			TargetDetector Tdetector = new TargetDetector(im);

			//plan path and move robot
			// TODO create a class to idenify where we are going next check path
			// via map class then order robot ot move.
            pDrive.driveForward();
		}
	}
}
