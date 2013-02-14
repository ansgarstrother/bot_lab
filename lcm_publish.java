import java.io.*;
import lcm.lcm.*;
import java.Math;


public class LCM_PUBLISH {
    
    public static void main(String args[]) {
        try {
            LCM lcm = LCM.getSingleton();
            
            // instantiate a new lcm position message
            lcm.pos_t pos_msg = new lcm.pos_t();
            pos_msg.timestamp = System.nanoTime();
            pos_msg.x_pos = 1.0;    // dummy x coord
            pos_msg.y_pos = 2.0;    // dummy y coord
            pos_msg.theta = Math.toRadians(45); // dummy theta
            
            //publish
            lcm.publish("10_POSE", pos_msg);
        }
        catch (IOException e) {
            System.err.println("Exception: " + e);
        }
    }
}