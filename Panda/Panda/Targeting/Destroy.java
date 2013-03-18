package Panda.Targeting;

import orc.*;
import Panda.Odometry.*;
import java.lang.Object.*;
import orc.DigitalOutput.*;
import java.*;

public class Destroy {

	final static double FEILD_VIEW = 135;
	protected PandaDrive drive;

	public Destroy(PandaDrive pd){
		this.drive = pd;
	}

    public void fireZeMissiles(ArrayList<float> angles){
        drive.setHome();
        for (float launchAngle : angles){
            drive.turn(launchAngle)
            Laser()
        }
        drive.returnHome();
    }

	public void fire (float x, float width){

	   double angle =  (x - (width/2));

       double factor = 115.0/(double)width;


        angle =  - (angle * factor) ;

		drive.turn((float)angle);

		try{Thread.sleep(500);}
        catch(Exception e){}

		Laser();

		drive.turn( (float)- angle );

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

}




























