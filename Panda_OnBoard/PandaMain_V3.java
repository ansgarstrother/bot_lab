import lcm.lcm.*;
import java.util.Vector;

import lcmtypes.*;

import Panda.Targeting.*;
import Panda.VisionMapping.*;
import Panda.sensors.*;
import Panda.Odometry.*;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;
import java.util.HashMap;
import java.lang.Thread;

import april.jcam.*;
import april.util.*;
import april.jmat.*;

public class PandaMain_V3{

    static double sampleRate = 200;    //microseconds

	static double[] calibrationMatrix = { 640.1483, 676.0408, 480.3221};

	static BufferedImage im;

//=================================================================//
// main of the panda bot                                           //
//                                                                 //
//=================================================================//
	public static void main(String [] args){

		// ARGS
		boolean run = true;	// main loop boolean
		plan_t prev_plan_msg = new plan_t();
		plan_t new_plan_msg = new plan_t();

        try {
            // start motor, pimu, gyro subscribers
            //MotorSubscriber ms = new MotorSubscriber();
            PIMUSubscriber ps = new PIMUSubscriber();
			PlanSubscriber planSubscriber = new PlanSubscriber();
            Gyro g = new Gyro();

            System.out.println ("Starting Odometry Thread");


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

			PandaPositioning positioner = new PandaPositioning();
			PandaDrive drive = new PandaDrive();
		    //TargetDetector target = new TargetDetector(drive, calibrationMatrix, positioner);
		
			is.start();
			System.out.printf("AAHHHHHHHHH\n");
			while(run){

				//Get a new image
		       	byte buf[] = is.getFrame().data;
		       	if (buf == null)
		       		continue;

				im = ImageConvert.convertToImage(fmt.format, fmt.width, fmt.height, buf);
		
				//Detect any triangles and then fire on them
				//target.runDetection(im);
				ArrayList<double[]> triangle_points = new ArrayList<double[]>();// target.getTrianglePoints();
				double[][] send_triangle = new double[triangle_points.size()][2];

				for (int i = 0; i < triangle_points.size(); i++) {
					double[] triangle = triangle_points.get(i);
					send_triangle[i][0] = triangle[0]; send_triangle[i][1] = triangle[1];
				}
			
				// Get Barriers
				BarrierMap barrierMap = new BarrierMap(im, calibrationMatrix, positioner);
				ArrayList<double[][]> barriers = barrierMap.getBarriers();
				double[][] send_barrier = new double[barriers.size()][4];

				for (int i = 0; i < barriers.size(); i++) {
					double[][] barrier = barriers.get(i);
					send_barrier[i][0] = barrier[0][0];
					send_barrier[i][1] = barrier[0][1];
					send_barrier[i][2] = barrier[1][0];
					send_barrier[i][3] = barrier[1][1];
				}
		
				// publish LCM to map
				LCM lcm_send = new LCM("udpm://239.255.76.10:7667?ttl=1");
				map_t msg = new map_t();
				msg.numTriangles = (byte)triangle_points.size();
				msg.numBarriers = (byte)barriers.size();
				msg.barriers = send_barrier;
				msg.triangles = send_triangle;
				lcm_send.publish("10_MAP", msg);

				// wait for a message received from 10_PLAN
				while (new_plan_msg == prev_plan_msg || new_plan_msg == null) {
							new_plan_msg = planSubscriber.getMessage();
				}
				// update message and plan
				// new_plan_msg = [    int64_t timestamp; boolean finished; float delta_x; double theta; ]
				prev_plan_msg = new_plan_msg;
			
				// test finished
				run = new_plan_msg.finished;
				// retrieve new location
				float delta_x = new_plan_msg.delta_x;
				double theta = new_plan_msg.theta;
			

				// drive to new location
				// first turn, then drive
				drive.turn((float)Math.toDegrees(theta));
				drive.driveForward(delta_x);
			
			

				System.out.printf("loop done\n");
			}
		} catch (Exception e) {
            System.out.println(e);
        }
	}
}
