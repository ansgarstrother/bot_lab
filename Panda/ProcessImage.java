import Panda.*;

import java.util.*;
import java.io.*;
import java.util.*;

// ProcessImage abstracts away ALL of image processing

public class ProcessImage () {

    LineDetectionDetector lineDetector;
    TargetDetector targetDetector;

    public ProcessImage (BufferedImage im) {

        lineDetector = new LineDetectionDetector ();
        targetDetector = new TargetDetector ();
    }



    // returns line segments in panda's coordinate frame
    // passes to PandaPositioning (or whoever) to get global coordinates
    public double[][] getLineSegments () {



    }


    public bool foundTarget () {



    }






}
