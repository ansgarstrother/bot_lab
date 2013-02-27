package Rover;
import java.io.*;
import lcm.lcm.*;
import java.lang.Math;
import april.jmat.Matrix.*;
import java.util.Vector;

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

	static Vector<Move> moves = new Vector<Move>();

    static class Move{
		float distance;
   		double theta;  // IN RADIANS
    }


    static Location curLocation = new Location();


	public void PostisionCalulator(int OdCalibration, int roverBase){
		OdCal = OdCalibration;
		base = roverBase;
        curLocation.locX = 0;
        curLocation.locY = 0;
        curLocation.theta = 0;
        curLocation.errorX = 0;
        curLocation.errorY = 0;
        curLocation.errorTheta = 0;

	}


	static float moved (int disL, int disR){
		return (disL + disR)/2;
	}

	static double turned ( int disL, int disR){
		return (disR-disL)/base;
	}

    static void updatePosition(Move move){

        curLocation.theta += move.theta;
        curLocation.locX += move.distance * Math.cos(curLocation.theta);
        curLocation.locY += move.distance * Math.sin(curLocation.theta);

        curLocation.errorTheta += .01 * move.theta;
        curLocation.errorX += .01 * move.distance * Math.cos(curLocation.theta);
        curLocation.errorY += .01 * move.distance * Math.sin(curLocation.theta);
    }

	public void updatelocation(int ticksL, int ticksR){
		int disL = ticksL * OdCal;
		int disR = ticksR * OdCal;


        Move curMove = new Move();

        //calculate the movement
        curMove.distance = moved(disL, disR);
        curMove.theta = turned(disL, disR);

        //Update the postition
        updatePosition(curMove);

        moves.add(curMove);
	}

    public double[] curPostion(){

        double temp[] = {curLocation.locX, curLocation.locY, curLocation.theta};
        return temp;
    }

    public float[] curError(){
        float temp[] = {curLocation.errorX, curLocation.errorY, curLocation.errorTheta};
        return temp;
    }
}
