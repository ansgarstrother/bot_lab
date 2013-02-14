import java.io.*;
import lcm.lcm.*;
import java.lang.Math;


public class odometry {

    // pos_t vars
    float odom_x;
    float odom_y;
    double odom_theta;  // IN RADIANS
    
    // covar_t vars
    // matrix form: 
    //  [   var_x   a       ]
    //  [   a       var_y   ]
    //  vector form: [ var_x, a, var_y ]
    float [] odom_covar_vec;
    
    
    // CONSTRUCTOR
    public odometry() {
        // retrieve position from sensors/gyro
        // assign to class variables
        
    }
    
    // PRIVATE CLASS METHODS
    private void calculatePose() {
        // derive all class args
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
        concurrent_timestamp = System.nanoTime();
        
        // instantiate a new lcm position message
        lcmtypes.pos_t pos_msg = new lcmtypes.pos_t();
        pos_msg.timestamp = concurrent_timestamp;
        pos_msg.x_pos = odom_x;
        pos_msg.y_pos = odom_y;
        pos_msg.theta = odom_theta;
        
        // instantiate a new lcm covariance matrix message
        lcmtypes.covar_t covar_msg = new lcmtypes.covar_t();
        covar_msg.timestamp = concurrent_timestamp;
        covar_msg.covar_vec = odom_covar_vec;
        
        //publish
        lcm.publish("10_POSE", pos_msg);
        lcm.publish("10_POSE", covar_msg);

    }
}