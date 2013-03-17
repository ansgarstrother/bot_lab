import lcm.lcm.*;
import java.util.Vector;

import Panda.Odometry.*;
import Panda.Targeting.*;
import Panda.VisionMapping.*;
import Panda.sensors.*;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;

import april.jcam.*;
import april.util.*;
import april.jmat.*;

public class PandaMain_V2{

	static boolean run = true;
    static double sampleRate = 200;    //microseconds

	private final static double f = 640.1483;
	private final static double c_x = 676.0408;
	private final static double c_y = 480.3221;
    private static double[] calibrationMatrix =
		{ 	f, c_x, c_y	};

//=================================================================//
// main of the panda bot                                           //
//                                                                 //
//=================================================================//
	public static void main(String [] args){


        try {
        // start motor, pimu, gyro subscribers
            MotorSubscriber ms = new MotorSubscriber();
            PIMUSubscriber ps = new PIMUSubscriber();
            Gyro g = new Gyro();

            System.out.println ("Starting Odometry Thread");
            Thread odometryThread = new Thread (new PandaOdometry (ms, ps, g));
            odometryThread.start();

        } catch (Exception e) {
            System.out.println(e);
        }

        System.out.printf ("Main thread running\n");
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
		//Projection projection = new Projection();


		// get matrix transform history
		// get calibrated coordinate transform
        PandaPositioning positioner = new PandaPositioning();


		//Panda Driver
    	//Drive drive = new Drive();
		//Path path = new Path();
        MapMgr map = new MapMgr();    // init random int
        TargetDetector target = new TargetDetector();
        double globalTheta;

		PathPlan path = new PathPlan();

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

            Matrix globalPos = positioner.getGlobalPos();
            globalTheta = positioner.getGlobalTheta();

            // global transformation matrix used to calculate points
			//Detect any triangles and then fire on them
			target.runDetection(im);

			//Line Detector finds barriers and adds them to the map
			BarrierMap barrierMap = new BarrierMap(im, calibrationMatrix, positioner);
			//map.addBarrier(barrierMap);

            // set barriers, update known positions
            map.updateMap (barrierMap, globalPos, globalTheta);

/*
			//Plans path
			path.plan();

			//turns robot
			double angle = path.turn();
			drive.turn( angle );

			// moves robot foward
			double forward = path.forward();
			drive.foward( forward );
*/

            // calculate new global position
            // positioner.updateGlobalPosition (distance, theta);
            // positioner.updateGlobalTheta (g);


		}
	}
}
