package Rover;

import orc.*;

import java.io.*;
import java.util.*;
import java.lang.*;

public class RoverOrcGetValues {


    Orc orc;
    public double[] getMotorsCurrents () {

        double[] currents = new double[3];

        for (int i=0; i < 3; i++) {
            Motor motor = new Motor (orc, i, false);
            currents[i] = motor.getCurrent();

        }

        return currents;

    }

    public double[] getGyros () {

        double[] gyros = new double[3];

        for (int i=0; i < 3; i++) {
            Gyro gyro = new Gyro (orc, i);
            gyros[i] = gyro.getTheta();

        }

        return gyros;

    }





}
