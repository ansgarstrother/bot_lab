package Panda;

import java.io.*;

import lcm.lcm.*;
//import Panda.*;

public class DriveTest
{
    public DriveTest(){

    }


    public static void main(String args[])
    {
       PandaDrive pandaDrive = new PandaDrive();
       int i = 0;
		while (true) {    
			try{
				pandaDrive.driveForward(1.0F);
				//pandaDrive.turnRight();
				Thread.sleep(100);
				System.out.println (i);
				i++;

            } catch(InterruptedException ex) {
                //Do stuff crazy fun things with excesption
            }
         }
    }
}
