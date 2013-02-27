import java.io.*;

import lcm.lcm.*;
import lcmtypes.*;
import Rover.RoverDrive;

public class DriveTest 
{
    public static void main(String args[])
    {
        RoverDrive roverDrive = new RoverDrive();
        while(true){
            roverDrive.driveForward();
            Thread.sleep(100);
        }

    }
}
