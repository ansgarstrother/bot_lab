package Rover;

import java.io.*;
import lcm.lcm.*;
import java.lang.Math;
import lcmtypes.*;

public class RoverOdometry {
    
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
    public RoverOdometry() throws IOException {
        // retrieve position from sensors/gyro
        // assign to class variables
        nmotors = 2;
        
        odom_encoders = new int[nmotors];
        odom_current = new double[nmotors];
        odom_applied_voltage = new double[nmotors];
        
        odom_integrator = new int[8];
        odom_accel = new int[3];
        odom_mag = new int[3];
        odom_alttemp = new int[2];
        
    }
    
    // PRIVATE CLASS METHODS
    private void calculatePose() {
        
        // TALK TO ORC BOARD HERE
        
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
        
        // instantiate a new lcm pimu message
        lcmtypes.pimu_t pimu_msg = new lcmtypes.pimu_t();
        pimu_msg.utime = concurrent_timestamp;
        pimu_msg.utime_pimu = System.nanoTime();
        for (int i = 0; i < odom_integrator.length; i++) {
            pimu_msg.integrator[i] = odom_integrator[i];
        }
        for (int i = 0; i < odom_accel.length; i++) {
            pimu_msg.accel[i] = odom_accel[i];
        }
        for (int i = 0; i < odom_mag.length; i++) {
            pimu_msg.mag[i] = odom_mag[i];
        }
        for (int i = 0; i < odom_alttemp.length; i++) {
            pimu_msg.alttemp[i] = odom_alttemp[i];
        }
        
        // instantiate a new lcm motor feedback message
        lcmtypes.motor_feedback_t mf_msg = new lcmtypes.motor_feedback_t();
        mf_msg.utime = concurrent_timestamp;
        mf_msg.nmotors = nmotors;
        for (int i = 0; i < odom_encoders.length; i++) {
            mf_msg.encoders[i] = odom_encoders[i];
        }
        for (int i = 0; i < odom_current.length; i++) {
            mf_msg.current[i] = odom_current[i];
        }
        for (int i = 0; i < odom_applied_voltage.length; i++) {
            mf_msg.applied_voltage[i] = odom_applied_voltage[i];
        }
            
        
        //publish
        lcm.publish("10_POSE", pos_msg);
        lcm.publish("10_COVAR", covar_msg);
        lcm.publish("10_PIMU", pimu_msg);
        lcm.publish("10_MOTOR_FEEDBACK", mf_msg);
        
    }
}
