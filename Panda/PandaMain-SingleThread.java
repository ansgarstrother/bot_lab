package Panda;

import Vision.LineDetection.*;
import Vision.calibration.*;
import lcm.lcm.*;

public class PandaMain{

//=================================================================//
// main of the panda bot                                           //
//                                                                 //
//=================================================================//
	public static void main(String [] args){

		//Read in Calibration of Panda Bot
		Projection projection = new Projection();
		
		while(run){

			//find and add barriers to the maps
			lineDetection();

			if(triangleDetected()){
				align();
				fireLaser();
			}

			//plan path and move robot
			Move();
		}
	}
}
