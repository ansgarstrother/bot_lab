package orc;

import java.io.*;

public class AX12ChangeID
{
    public AX12ChangeID()
    {
    }

    public static void main(String args[])
    {
        int oldID = 1, newID = 2;
        Orc orc = Orc.makeOrc();
        Integer i;

        if (args != null)
        {
            System.out.println("length = " + args.length);
            if (args.length == 1)
            {
                i = new Integer(args[0]);
                newID = i.intValue();
            }
            else if (args.length == 2)
            {
                System.out.println(args[0]);
                i = new Integer(args[0]);
                oldID = i.intValue();
                i = new Integer(args[1]);
                newID = i.intValue();
            }
        }
        System.out.println("oldID="+oldID+"\t\tnewID="+newID);
        AX12Servo servo = new AX12Servo(orc, oldID);
        if (servo.ping())
        {
            System.out.println("GOOD Ping");
            servo.changeServoID(oldID, newID);
        }
        else
            System.out.println("BAD Ping");

    }
}
