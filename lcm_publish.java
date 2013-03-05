import java.io.*;
import lcm.lcm.*;
import java.lang.Math;


public class lcm_publish {
    
    public static void main(String args[]) {

            LCM lcm = LCM.getSingleton();
            /*
            // instantiate a new lcm position message
            lcmtypes.pos_t pos_msg = new lcmtypes.pos_t();
            pos_msg.timestamp = System.nanoTime();
            pos_msg.delta_x = (float)(Math.sqrt(2));    // dummy x coord
            pos_msg.theta = Math.toRadians(45); // dummy theta
	    	pos_msg.finished = false;
	    	pos_msg.triangle_found = true;
            
            //publish
            lcm.publish("10_POSE", pos_msg);
			*/
			// instantiate a new lcm position message
            lcmtypes.motor_feedback_t msg = new lcmtypes.motor_feedback_t();
    		msg.utime = System.nanoTime();
    		msg.estop = false;

    		msg.nmotors = 2;
			msg.encoders = new int[2];
    		msg.encoders[0] = 700;
			msg.encoders[1] = 700;
			msg.current = new float[2];
    		msg.current[0] = 0f;
			msg.current[1] = 0f;
			msg.applied_voltage = new float[2];
    		msg.applied_voltage[0] = 0f;
			msg.applied_voltage[1] = 0f;
            
            //publish
            lcm.publish("10_MOTOR_FEEDBACK", msg);
    }
}
