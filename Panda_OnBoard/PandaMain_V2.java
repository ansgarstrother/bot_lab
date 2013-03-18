import lcm.lcm.*;
import java.util.Vector;

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

public class PandaMain_V2{

    static double sampleRate = 200;    //microseconds

	static double[] calibrationMatrix = { 640.1483, 676.0408, 480.3221};

	//static BufferedImage im;

//=================================================================//
// main of the panda bot                                           //
//                                                                 //
//=================================================================//
	public static void main(String [] args){

		boolean run = true;	// main loop boolean
		BufferedImage im;
        try {
            // start motor, pimu, gyro subscribers
            //MotorSubscriber ms = new MotorSubscriber();
            Gyro g = new Gyro();

            System.out.println ("Starting Odometry Thread");

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

		PandaPositioning positioner = new PandaPositioning();
    	PandaDrive drive = new PandaDrive();
        TargetDetector target = new TargetDetector(drive, calibrationMatrix, positioner);

		is.start();
		byte prev_buf[] = is.getFrame().data;
		while(run){

			//Get a new image
           	byte buf[] = is.getFrame().data;
			for(int i = 0; i <10; i ++) {
				buf = is.getFrame().data;
			}

           	if (buf == null){
				System.out.println("ERROR CAMERA !!!!!!!!!!!!!!");
           		continue;
			}

			im = ImageConvert.convertToImage(fmt.format, fmt.width, fmt.height, buf);
		
			//Detect any triangles and then fire on them
			target = new TargetDetector(drive, calibrationMatrix, positioner);

			target.runDetection(im);

			System.out.printf("loop done\n");

			prev_buf = buf;
			//try{Thread.sleep(100);}
			//catch(Exception e){}
		}

	}
}
