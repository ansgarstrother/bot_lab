package Panda.Targeting;

import DigitalOutput.*;
//import Panda.Odemetry;

public class Destroy {

	final static double FEILD_VIEW;

	//TargetDetector detector;
	//PandaDrive drive;
	Orc orc;

	public void Destroy(){
		//PandaDrive drive = new PandaDrive();
		orc = Orc.makeOrc();
		DigitalOutput laser = new DigitalOutput( orc, 0);

	}
/*
	public void fire (double y, double width){

		double center = width/2;
		double conversion = width/FEILD_VIEW;

		y -= center;
		double turnAngle = y/conversion;

		drive.turn(turnAngle);

		laser();
		
		drive.turn( - turnAngle ); 

	}*/

	static void laser(){
		
		for(int i; i < 3; i++){
			laser.setValue (true);
		
			Thread.sleep(100);

			laser.setValue (false);

			Thread.sleep(100);
		}
	}

	public static void main(String args[]){

		Destroy d = new Destroy();
		d.laser();

	}

	

	

























}

