import java.io.*;
import lcm.lcm.*;
import java.lang.Math;
import lcmtypes.*;

public class odometry implements LCMSubscriber {

    // LCM
    LCM lcm;
    float odom_timestamp;
    float odom_timestamp_pimu;

    // motor feedback
    int nmotors;
    int odom_encoders[];
    double  odom_current[];
    double  odom_applied_voltage[];

    // pimu feedback
    int odom_integrator[];
    int odom_accel[];
    int odom_mag[];
    int odom_alttemp[];

    // pos_t vars
    float odom_x;
    float odom_y;
    double odom_theta;  // IN RADIANS
    
    // covar_t vars
    // matrix form: 
    //  [   var_x   a       ]
    //  [   a       var_y   ]
    //  vector form: [ var_x, a, var_y ]
    float odom_var_x;
    float odom_var_y;
    float odom_var_a;
    
    
    // CONSTRUCTOR
    public odometry() throws IOException {
        // retrieve position from sensors/gyro
        // assign to class variables
	this.lcm = new LCM();
	this.lcm.subscribe("10_MOTOR_FEEDBACK", this);
	this.lcm.subscribe("10_PIMU", this);

	odom_encoders = new int[nmotors];
	odom_current = new double[nmotors];
	odom_applied_voltage = new double[nmotors];

	odom_integrator = new int[8];
	odom_accel = new int[3];
	odom_mag = new int[3];
	odom_alttemp = new int[2];
        
    }
    
    // LCM CLASS METHODS
    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins) {

	//System.out.println("Received message on channel " + channel);

        try {
	    if (channel.equals("10_MOTOR_FEEDBACK")) {
		motor_feedback_t msg = new motor_feedback_t(ins);
                odom_timestamp = msg.utime;
		for (int i = 0; i < msg.nmotors; i++) {
                	odom_encoders[i] = msg.encoders[i];
		}
		for (int i = 0; i < msg.nmotors; i++) {
                	odom_current[i] = msg.current[i];
		}
		for (int i = 0; i < msg.nmotors; i++) {
                	odom_applied_voltage[i] = msg.applied_voltage[i];
		}	
	    }
	    else if (channel.equals("10_PIMU")) {
		pimu_t msg = new pimu_t(ins);
		//System.out.println("  timestamp    = " + msg.utime); WHICH TIMESTAMP TO USE?
		odom_timestamp_pimu = msg.utime_pimu;
		for (int i = 0; i < 8; i++) {
			odom_integrator[i] = msg.integrator[i];
	    	}
		for (int i = 0; i < 3; i++) {
			odom_accel[i] = msg.accel[i];
	    	}
		for (int i = 0; i < 3; i++) {
			odom_mag[i] = msg.mag[i];
	    	}
		for (int i = 0; i < 2; i++) {
			odom_alttemp[i] = msg.alttemp[i];
	    	}
	    }

        } catch (IOException ex) {
            System.err.println("Exception: " + ex);
        }
	
    }

    // PRIVATE CLASS METHODS
    private void calculatePose() {
	

	
    }
    
    // PUBLIC ACTION METHODS
    public void refreshPose() {
        // get pose from LCM
        calculatePose();
    }
    
    public void sendPose() {
        // refresh pose
        calculatePose();
        
        LCM lcm = LCM.getSingleton();
        long concurrent_timestamp = System.nanoTime();
        
        // instantiate a new lcm position message
        lcmtypes.pos_t pos_msg = new lcmtypes.pos_t();
        pos_msg.timestamp = concurrent_timestamp;
        pos_msg.x_pos = odom_x;
        pos_msg.y_pos = odom_y;
        pos_msg.theta = odom_theta;
        
        // instantiate a new lcm covariance matrix message
        lcmtypes.covar_t covar_msg = new lcmtypes.covar_t();
        covar_msg.timestamp = concurrent_timestamp;
        covar_msg.var_x = odom_var_x;
	covar_msg.var_y = odom_var_y;
	covar_msg.a = odom_var_a;
        
        //publish
        lcm.publish("10_POSE", pos_msg);
        lcm.publish("10_COVAR", covar_msg);

    }
}
