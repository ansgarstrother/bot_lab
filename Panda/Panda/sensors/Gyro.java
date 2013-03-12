package Panda.sensors;

import java.io.*;
import java.lang.Math;
import lcmtypes.*;
import Panda.sensors.*;
import java.util.ArrayList;

public class Gyro
{
    //Gyro Constants
    static final float CALIBRATION_TIME = 300; //Approx 3 sec
    static final double SENSITIVITY_CONSTANT = 1.0; //This needs to be calculated

    private long prevTimeInMilli;
    private double gyroOffset;
    private double gyroAngle;

    PIMUSubscriber ps;

    public Gyro()
    {
        try{
            ps = new PIMUSubscriber();
        }catch(Exception e){}
        initGyro();
    }

    private void initGyro(){
        System.out.println("Calibrating Gyro..");
        int counter = 0;
        ArrayList<Double> data = new ArrayList<Double>();

        //This loop will take 100 gyro data points
        while(counter < CALIBRATION_TIME){
            pimu_t pimuData = ps.getMessage();
            //TODO: Add gyro integrator value to data ArrayList here
            double curGyroIntegrator = 0;
            data.add(curGyroIntegrator);
            try{ Thread.sleep(10); }
            catch(Exception e){};
            counter++;
        }

        double temp = 0;
        for(double dataPoint : data){
            temp += dataPoint;
        }

        gyroOffset = temp / (CALIBRATION_TIME / 10);
        System.out.println("Finished Gyro Calibration");

        //Note: You could improve this offset calibration and limit
        //drift using this: New offset = (samplesize – 1) * old offset + (1 / samplesize) * gyro
        //every time the robot is stopped
    }

    public double getGyroAngle(){
        //This should be called continuously in short time intervals to minimize error

        pimu_t pimuData = ps.getMessage();
        //TODO: Get current gyro value
        double curGyroIntegrator = 0;

        //Final units of gyroRate should be degrees per second
        double gyroRate = (curGyroIntegrator - gyroOffset) / SENSITIVITY_CONSTANT;
        gyroAngle += gyroRate * (System.currentTimeMillis() - prevTimeInMilli) / 1000;

        prevTimeInMilli = System.currentTimeMillis();
        return gyroAngle;

        //Note: Can also integrate the accelerometer and use a Kalman filter to
        //      minimize drift
    }

}
