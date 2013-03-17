
import java.util.Vector;

import Panda.VisionMapping.*;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;

import april.jcam.*;
import april.util.*;
import april.jmat.*;


// CLASS DEFINITION:
//	- Path Planning Test Main
public class PathPlanMain {

	// args
	final static int s = 1000;

	public static void main(String[] args) {
			
		// retrieve robot orientation from mapping
		// values must be static cast to int for planning
		int bot_x = 0;
		int bot_y = 0;
		int bot_t = 0;

		// init a new map
		int[][] map = new int[s][s];
		for (int i = 0; i < s; i++) {
			for (int j = 0; j < s; j++) {
				map[i][j] = 0;
			}
		}

		// path planning
		PathPlan planning = new PathPlan();
		planning.simplePlan(map, bot_x, bot_y, bot_t);

		System.out.println("Planning Results: ");
		System.out.println("new angle: " + planning.getPathAngle() + " delta_x: " + planning.getPathDistance());
	
		return;
	}


}
