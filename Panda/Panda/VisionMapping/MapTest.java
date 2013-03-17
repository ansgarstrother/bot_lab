package Panda.VisionMapping;

import java.util.*;
import java.awt.*;

import april.jmat.*;
import april.util.*;

public class MapTest {


    public static void main (String args[]) {

        MapMgr map = new MapMgr();

        double[][] globalPos = { {0}, {0}, {1} };
        double globalTheta = 0;
        Matrix globalPosMat = new Matrix (globalPos);

        map.updateMap (globalPosMat, globalTheta);



    }




}
