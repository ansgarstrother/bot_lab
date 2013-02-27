package Panda;

import Vision.LineDetection.*;
import Vision.calibration.*;
import lcm.lcm.*;
import java.utils.Vector

public class PandaMain{

	Vector<Stats> Tagets = new Vector<Stats>();

//=================================================================//
// main of the panda bot                                           //
//                                                                 //
//=================================================================//
	public static void main(String [] args){

		//Read in Calibration of Panda Bot
		Projection projection = new Projection();
		
		while(run){

			//get a new image

			//find and add barriers to the maps
			lineDetection();


			//Detect any triangles and then fire on them
			PandaTargetDetector Tdetector = new PandaTargetDetector(im);
			Tdetector.runDetection();
			Targets = Tdetector.GetTriangleLocation();
			
			while( ! Target.isEmpty()){
				//align();
				//fireLaser();
			}

			//plan path and move robot
			Move();
		}
	}
}
