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

    private double gyroCreep;
    private long startTime;
    private int startGyroVal;

    private long curTime;
    private int curGyroVal;

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
        pimu_t pimuData = ps.getMessage();
        startTime = pimuData.utime_pimu;
        startGyroVal = pimuData.integrator[5];
        try{
           Thread.sleep(2000);
        }catch(Exception e){}
        curTime = pimuData.utime_pimu;
        curGyroVal = pimuData.integrator[5];

        gyroCreep = (curGyroVal - startGyroVal) / (curTime - startTime);

        //Note: You could improve this offset calibration and limit
        //drift using this: New offset = (samplesize – 1) * old offset + (1 / samplesize) * gyro
        //every time the robot is stopped
    }

    public double getGyroAngle(){
        pimu_t pimuData = ps.getMessage();
        curTime = pimuData.integrator[5];
        curGyroVal = pimuData.integrator[5];

        long timeChange = curTime - startTime;
        double totalCreep = gyroCreep * timeChange;
        gyroAngle = (curGyroVal - startGyroVal) - totalCreep;

        return gyroAngle;

        //Note: Can also integrate the accelerometer and use a Kalman filter to
        //      minimize drift
    }

}
