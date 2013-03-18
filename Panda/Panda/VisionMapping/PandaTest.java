package Panda.VisionMapping;

import java.util.*;
import java.awt.*;
import java.lang.*;

import april.jmat.*;


public class PandaTest {

    public static void main (String args[]) {
        PandaPositioning pp = new PandaPositioning ();

        double[][] coord = { {1}, {2}, {1}};
        Matrix coordMat = new Matrix(coord);
        Matrix globalCoord;
        globalCoord = pp.translateToGlobal(coordMat);
        System.out.println (globalCoord);


    }




}
