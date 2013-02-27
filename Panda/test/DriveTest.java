import java.io.*;

import lcm.lcm.*;
import lcmtypes.*;
import Panda.*;

public class DriveTest
{
    public static void main(String args[])
    {
        PandaDrive pandaDrive = new PandaDrive();
        while(true){
            try{
                pandaDrive.driveForward();
                Thread.sleep(100);
            }catch(InterruptedException ex) {
                //Do stuff crazy fun things with excesption
            }
         }
    }
}
