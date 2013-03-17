package Panda.VisionMapping;

import java.util.*;
import java.awt.*;

import april.jmat.*;
import april.util.*;

public class MapTest {


    public static void main (String args[]) {

        MapMgr map = new MapMgr();

        double[][] globalPos = { {0.5}, {0.5}, {1} };
        double globalTheta = Math.PI/2;
        Matrix globalPosMat = new Matrix (globalPos);

        map.updateMap (globalPosMat, globalTheta);



    }




}
