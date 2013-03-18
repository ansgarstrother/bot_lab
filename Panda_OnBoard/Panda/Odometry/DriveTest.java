package Panda.Odometry;

import java.io.*;

import lcm.lcm.*;

import java.util.*;
import Panda.*;

public class DriveTest
{
    public DriveTest(){

    }


    public static void main(String args[])
    {
       PandaDrive pandaDrive = new PandaDrive();
       int i = 0;
		while (true) {
			Scanner in = new Scanner(System.in);
			float a, b, c;
			try{
				a = in.nextFloat();
				b = in.nextFloat();
				c = in.nextFloat();

				if(a == 0)	
					pandaDrive.driveForward(b);
			
				if(a == 1)
					pandaDrive.turn(b);

				Thread.sleep(100);

            } catch(InterruptedException ex) {
                //Do stuff crazy fun things with excesption
            }
        }
    }
}
