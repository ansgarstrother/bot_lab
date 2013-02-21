package Rover;
import java.io.*;
import lcm.lcm.*;
import java.lang.Math;
import lcmtypes.*;

public class PositionCalulator {

	static int OdCal;
	static int base;

	static class Location{
		float locX;
		float errorX;
    	float locY;
		float errorY;
		float errorTheta; 
   		double theta;  // IN RADIANS
	}

	static Vector<Location> prevLocations = new Vector<Location>;

	public PostisionCalulator(int OdCalibration, int roverBase){
		OdCal = OdCalibration;
		base = roverBase;
	}


	static float moved (int disL, int disR){
		return (disL + disR)/2;
	}

	static double turned ( int disL, int disR){
		return (disR-disL)/base;
	}
		
	public Location updatelocation(int ticksL, int ticksR){
		
		int disL = ticksL * OdCal;
		int disR = ticksR * OdCal;
		
		
	}



}
