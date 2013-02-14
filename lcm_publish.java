import java.io.*;
import lcm.lcm.*;
import java.lang.Math;


public class lcm_publish {
    
    public static void main(String args[]) {

            LCM lcm = LCM.getSingleton();
            
            // instantiate a new lcm position message
            lcmtypes.pos_t pos_msg = new lcmtypes.pos_t();
            pos_msg.timestamp = System.nanoTime();
            pos_msg.x_pos = 1.0f;    // dummy x coord
            pos_msg.y_pos = 2.0f;    // dummy y coord
            pos_msg.theta = Math.toRadians(50); // dummy theta
            
            //publish
            lcm.publish("10_POSE", pos_msg);
    }
}
