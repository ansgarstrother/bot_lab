import lcm.lcm.*;
import java.util.Vector;

import Panda.*;
import Panda.Targeting.*;
import Panda.VisionMapping.*;
import Panda.sensors.*;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;
import java.util.HashMap;

import april.jcam.*;
import april.util.*;
import april.jmat.*;

public class PandaMain_V2{

    static double sampleRate = 200;    //microseconds

	static double[][] calibrationMatrix = {{0, 0, 1}, {0, 1, 0}, {0, 0, 1}};

	static BufferedImage im;

//=================================================================//
// main of the panda bot                                           //
//                                                                 //
//=================================================================//
	public static void main(String [] args){

		boolean run = true;	// main loop boolean


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


		// Map Manager
        MapMgr map = new MapMgr();    // init random int
        TargetDetector target = new TargetDetector();
        double globalTheta;

		// Path Planning
		PathPlan path = new PathPlan();

		// Drive Application
    	PandaDrive drive = new PandaDrive();

		is.start();
		while(run){

            // Implement Sampling Rate
            //Thread.sleep(sampleRate);?

			//Get a new image
           	byte buf[] = is.getFrame().data;
           	if (buf == null)
           		continue;

			im = ImageConvert.convertToImage(fmt.format, fmt.width, fmt.height, buf);
		
				

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

			//Plans path
			// requires map, current x, y, and orientation of bot in global frame
			path.advancedPlan(map.getMap(), (int)globalPos.get(0,0), (int)globalPos.get(1,0), globalTheta);
			double angle = path.getPathAngle();
			double dist = path.getPathDistance();
			run = !(path.getFinishedTest());

			//turns robot
			float in_angle = (float)angle;
			drive.turn( in_angle );

			// moves robot foward
			float in_dist = (float)dist;
			drive.driveForward( in_dist );


            // calculate new global position
            positioner.updateGlobalPosition (dist, angle);


		}
	}
}
