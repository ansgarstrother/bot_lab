package Panda.Targeting;

import orc.*;
import Panda.Odometry.*;
import java.lang.Object.*;
import orc.DigitalOutput.*;

public class Destroy {

	final static float FEILD_VIEW = 90;

	public void Destroy(){
	}

	public void fire (float y, float width){

		float center = width/2;

		float conversion = width/FEILD_VIEW;

		y -= center;
		float turnAngle = y/conversion;

		PandaDrive drive = new PandaDrive();
		drive.turn(turnAngle);

		Laser();
		
		drive.turn( - turnAngle ); 

	}

	static void Laser(){
		Orc orc = Orc.makeOrc();
		DigitalOutput laser = new DigitalOutput( orc, 0, false);	
		for(int i = 0; i < 3; i++){
			laser.setValue (true);
		
			try{Thread.sleep(100);}
			catch(Exception e){}			

			laser.setValue (false);

			try{Thread.sleep(100);}
			catch(Exception e){}
		}
	}

	public static void main(String args[]){

		Destroy d = new Destroy();
		d.Laser();

		d.fire(0F, 4F);

	}
}	

	



























