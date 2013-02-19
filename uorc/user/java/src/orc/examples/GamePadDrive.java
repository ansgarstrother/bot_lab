package orc.examples;

import orc.*;
import orc.util.*;

/** Demonstrates how to control a differentially-driven robot with a standard USB joystick. **/
public class GamePadDrive
{
    public static void main(String args[])
    {
        GamePad gp = new GamePad();
        Orc orc = Orc.makeOrc();

        boolean flipLeft = args.length > 2 ? Boolean.parseBoolean(args[1]) : false;
        boolean flipRight = args.length > 2 ? Boolean.parseBoolean(args[2]) : true;
        boolean flipMotors = args.length > 3 ? Boolean.parseBoolean(args[3]) : false;

        Motor leftMotor = new Motor(orc, flipMotors ? 1: 0, flipLeft);
        Motor rightMotor = new Motor(orc, flipMotors ? 0: 1, flipRight);

        System.out.println("flipLeft: "+flipLeft+", flipRight: "+flipRight+", flipMotors: "+flipMotors);

        System.out.println("Hit any gamepad button to begin.");
        gp.waitForAnyButtonPress();

        System.out.printf("%15s %15s %15s %15s\n", "left", "right", "left current", "right current");

        while (true) {

            double left = 0, right = 0;

            if (false) {
                // independently control left & right wheels mode. "Tank mode"
                left = gp.getAxis(1);
                right = gp.getAxis(3);

            } else {
                double fwd = -gp.getAxis(3); // +1 = forward, -1 = back
                double lr  = gp.getAxis(2);   // +1 = left, -1 = right

                left = fwd - lr;
                right = fwd + lr;

                double max = Math.max(Math.abs(left), Math.abs(right));
                if (max > 1) {
                    left /= max;
                    right /= max;
                }
            }
            System.out.printf("%15f %15f %15f %15f\r", left, right, leftMotor.getCurrent(), rightMotor.getCurrent());
            leftMotor.setPWM(left);
            rightMotor.setPWM(right);

            try {
                Thread.sleep(30);
            } catch (InterruptedException ex) {
            }
        }
    }
}
