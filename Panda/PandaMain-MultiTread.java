package Panda;

import Vision.LineDetection.*;
import Vision.calibration.*;
import lcm.lcm.*;

public class PandaMain implements Runnable{

	boolean run = true;
	boolean alignBot = false;
	
//=================================================================//
// run                                                             //
//                                                                 //
//      T2                                                         //
//      Triangle Detection -> if detected -> take controls turns & //
//	fires on target.                                           //
//=================================================================//
	public void run(){
		while(run){

			if(triangleDetected()){
				align();
				alignBot = true;
			}
		}
	}

//=================================================================//
// main of the panda bot                                           //
//                                                                 //
// Run Sequence                                                    //
// 1. Read in Calibration file                                     //
// 2. Create 2: T1 general movement, T2 Triangle Detection         //
// 	T1                                                         //
//	Line Detection -> add Barrier -> chose new location ->     //
//      move robot -> wait till robot has moved repeat             //
//      T2                                                         //
//      Triangle Detection -> if detected -> take controls turns & //
//	fires on target.                                           //
//=================================================================//
	public static void main(String [] args){

		//Read in Calibration of Panda Bot
		Projection projection = new Projection();
		
		//Creates a new tread to run Triangle Detection on
		//starts run().
		Thread TriDetection = new Thread(new PandaMain(), "Thread");
		TriDetection.start();

		while(run){

			//find and add barriers to the maps
			lineDetection();

			//plan path and move robot
			Move();

		}
	}
}
