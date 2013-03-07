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
			/*
			// instantiate a new lcm motor message
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
			*/
			
			//instantiate a new lcm pimu message
			lcmtypes.pimu_t msg = new lcmtypes.pimu_t();
			msg.utime = System.nanoTime();
			msg.utime_pimu = System.nanoTime();
			msg.integrator = new int[8];
			msg.integrator[0] = msg.integrator[1] = msg.integrator[2] = msg.integrator[3] = msg.integrator[4] = msg.integrator[5] =
					msg.integrator[6] = msg.integrator[7] = 0;
    		msg.accel = new int[3];
			msg.accel[0] = msg.accel[1] = msg.accel[2] = 0;
    		msg.mag = new int[3];
			msg.mag[0] = msg.mag[1] = msg.mag[2] = 0;
    		msg.alttemp = new int[2];
			msg.alttemp[0] = msg.alttemp[1] = 0;

			//publish
			lcm.publish("10_PIMU", msg);
			
    }
}
