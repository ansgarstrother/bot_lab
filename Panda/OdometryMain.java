import lcm.lcm.*;
import java.util.Vector;


import Panda.VisionMapping.*;
import Panda.Odometry.*;
import Panda.util.*;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;

import april.jcam.*;
import april.util.*;
import april.jmat.*;


// CLASS DEFINITION:
//	- Implementation of Task #1
//	- lcm communication and interpretation of lcm types sent from panda_bot
//
public class OdometryMain {

	// args

	public static void main(String[] args) {
		// init sensors
		MotorSubscriber ms = new MotorSubscriber();
		PIMUSubscriber ps = new PIMUSubscriber();
		PandaOdometry po = new PandaOdometry(ms, ps);		


		return;
	}


}
