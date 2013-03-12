import java.io.*;

import lcm.lcm.*;
import lcmtypes.*;

public class MySubscriber implements LCMSubscriber
{
    LCM lcm;
    
    public MySubscriber()
    throws IOException
    {
        this.lcm = new LCM();
        this.lcm.subscribe("10_POSE", this);
        this.lcm.subscribe("10_MOTOR_FEEDBACK", this);
        this.lcm.subscribe("10_PIMU", this);
    }
    
    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins)
    {
        //System.out.println("Received message on channel " + channel);
        
        try {
            if (channel.equals("10_POSE")) {
                pos_t msg = new pos_t(ins);
                /*
                System.out.println("  timestamp    = " + msg.timestamp);
                System.out.println("  x coord     = [ " + msg.delta_x + " ]");
                System.out.println("  theta  = [ " + msg.theta + " ]");
				*/
            }
            else if (channel.equals("10_MOTOR_FEEDBACK")) {
                motor_feedback_t msg = new motor_feedback_t(ins);
				/*
                System.out.println("  timestamp    = " + msg.utime);
                for (int i = 0; i < msg.nmotors; i++) {
                	System.out.println("  encoders     = [ " + msg.encoders[i] + " ]");
                }
                for (int i = 0; i < msg.nmotors; i++) {
                	System.out.println("  current     = [ " + msg.current[i] + " ]");
                }
                for (int i = 0; i < msg.nmotors; i++) {
                	System.out.println("  applied voltage     = [ " + msg.applied_voltage[i] + " ]");
                }
				*/
				LCM sent = new LCM("udpm://239.255.76.10:7667?ttl=1");
				sent.publish("10_MOTOR_FEEDBACK", msg);
				

            }
            else if (channel.equals("10_PIMU")) {
                pimu_t msg = new pimu_t(ins);
				/*
                System.out.println("  timestamp    = " + msg.utime);
                System.out.println("  timestamp    = " + msg.utime_pimu);
                for (int i = 0; i < 8; i++) {
                    System.out.println("  integrator     = [ " + msg.integrator[i] + " ]");
                }
                for (int i = 0; i < 3; i++) {
                    System.out.println("  accel     = [ " + msg.accel[i] + " ]");
                }
                for (int i = 0; i < 3; i++) {
                    System.out.println("  mag     = [ " + msg.mag[i] + " ]");
                }
                for (int i = 0; i < 2; i++) {
                    System.out.println("  alttemp     = [ " + msg.alttemp[i] + " ]");
                }
				*/
				LCM sent = new LCM("udpm://239.255.76.10:7667?ttl=1");
				sent.publish("10_PIMU", msg);
            }
            
        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        }
    }
    
    public static void main(String args[])
    {
        try {
            MySubscriber m = new MySubscriber();
            while(true) {
                Thread.sleep(1000);
            }
        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        } catch (InterruptedException ex) { }
    }
}
