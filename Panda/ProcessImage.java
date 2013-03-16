import Panda.Odometry.*;
import Panda.VisionMapping.*;
import Panda.Targeting.*;

import java.util.*;
import java.io.*;
import java.awt.*;

// ProcessImage abstracts away ALL of image processing

public class ProcessImage {

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
