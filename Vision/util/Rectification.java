package Vision.util;

import java.awt.image.BufferedImage;
import java.lang.Math;


public class Rectification {

    // args
    int[] inputMap;
    double[] radialMap;
    static int width;
    static int height;
    
    // constants and parameters
    final static double a = -0.000910;
    final static double b = 1.527995;
    final static int CENTER_X = width/2;  // needs to be found
    final static int CENTER_Y = height/2;  // needs to be found
    
    public Rectification(int[] input, int w, int h) {
        radialMap = new double[width];
        inputMap = input;
        width = w;
        height = h;
        buildRadialMap();
    }


    protected void rectify() {
        // for all pixel correspondences in the input map
        // find the radius from the center
        // use lookup table to get rectified radius
        // recompute (x,y) and store in new data structure
        
    }
    
    // HELPER FUNCTIONS
    protected void buildRadialMap() {
        for (int i = 0; i < CENTER_X; i++) {
            radialMap[i] = a*i*i + b*i;
        }
    }
    
    protected double getRadius(int x, int y) {
        return Math.sqrt(x*x + y*y);
    }
}