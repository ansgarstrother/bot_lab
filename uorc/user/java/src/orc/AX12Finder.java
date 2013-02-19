package orc;

import java.util.ArrayList;

// Pings AX12s on all channels from 1 to 253 inclusive
public class AX12Finder
{
    public static void main(String args[])
    {
        Orc orc = Orc.makeOrc();

        ArrayList<Integer> servoIDs = new ArrayList<Integer>();

        System.out.println("Attempting to find servos: ");
        for (int i = 1; i <= 0xfd; i++) {
            AX12Servo servo  = new AX12Servo(orc, i);

            boolean result = servo.ping();

            if (result){
                servoIDs.add(i);
                System.out.print("o");
            } else {
                System.out.print("x");
            }

            if (i % 64 == 0)
                System.out.println();
        }

        System.out.print("\nFound servos: [");
        for (Integer i : servoIDs) {
            System.out.print(i+",");
        }
        System.out.println("]");
    }
}
