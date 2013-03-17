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
	private double startAngle;

    private long curTime;
    private int curGyroVal;

    private double gyroAngle;

    PIMUSubscriber ps;
	pimu_t pimuData;

    public Gyro()
    {
		pimuData = new pimu_t();
        try{
            ps = new PIMUSubscriber();
        }catch(Exception e){}
        initGyro();
    }

    private void initGyro(){
        System.out.println("Calibrating Gyro..");
        try{ //This wait is needed otherwise the code below will execute before a pimu message comes in 
           Thread.sleep(50);
        }catch(Exception e){}
        pimuData = ps.getMessage();
        startTime = pimuData.utime_pimu;
        startGyroVal = pimuData.integrator[5];
		System.out.println(startGyroVal);
        try{
           Thread.sleep(5000);
        }catch(Exception e){}
        pimuData = ps.getMessage();
        curTime = pimuData.utime_pimu;
        curGyroVal = pimuData.integrator[5];
		System.out.println(curGyroVal);

        gyroCreep = (double)(curGyroVal - startGyroVal) / (double)(curTime - startTime);
		System.out.println("Gyro Creep Val: " + gyroCreep);

        //Note: You could improve this offset calibration and limit
        //drift using this: New offset = (samplesize – 1) * old offset + (1 / samplesize) * gyro
        //every time the robot is stopped
    }

	public double getGyroAngleInDegrees(){
        pimuData = ps.getMessage();
        curTime = pimuData.utime_pimu;
        curGyroVal = pimuData.integrator[5];
		//System.out.println("Current gyro val: " + curGyroVal);

        long timeChange = curTime - startTime;
		//System.out.println("Time Change: " + timeChange);
        double totalCreep = gyroCreep * timeChange;
		//System.out.println("Total Creep: " + totalCreep);
        gyroAngle = (curGyroVal - startGyroVal) - totalCreep;

        return gyroAngle / 999059;

        //Note: Can also integrate the accelerometer and use a Kalman filter to
        //      minimize drift

	}
    public double getGyroAngle(){
        return Math.toRadians(getGyroAngleInDegrees());
    }

	public double getGyroAngle(double gyroVal, long[] time) {
		// gyroVals = [prev cur]
		// time = [prev cur]
		long timeDiff = time[1] - time[0];
		double totalCreep = gyroCreep * timeDiff;
		double gyroAngle = gyroVal - startGyroVal - totalCreep;
		return gyroAngle;
	}

}
