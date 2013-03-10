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

		//Panda Driver
    	Drive drive = new Drive();

		Path path = new Path();

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

			//Detect any triangles and then fire on them
			Target target = new Target(im);
			target.destroy();

			//Line Detector finds barriers and adds them to the map
			BarrierMap barrierMap = new BarrierMap(im);
			barrierMap.add();

			//Plans path 
			path.plan();

			//turns robot
			double angle = path.turn();
			drive.turn( angle );

			// moves robot foward 
			double forward = path.forward();
			drive.foward( forward );

		}
	}
}
