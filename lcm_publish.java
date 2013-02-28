import java.io.*;
import lcm.lcm.*;
import java.lang.Math;


public class lcm_publish {
    
    public static void main(String args[]) {

            LCM lcm = LCM.getSingleton();
            
            // instantiate a new lcm position message
            lcmtypes.pos_t pos_msg = new lcmtypes.pos_t();
            pos_msg.timestamp = System.nanoTime();
            pos_msg.delta_x = (float)(Math.sqrt(2));    // dummy x coord
            pos_msg.theta = Math.toRadians(45); // dummy theta
	    	pos_msg.finished = false;
	    	pos_msg.triangle_found = true;
            
            //publish
            lcm.publish("10_POSE", pos_msg);
    }
}
