package Vision.util;

import java.awt.image.BufferedImage;
import java.lang.Math;
import java.util.ArrayList;


public class Rectification {

    // args
    int[] inputMap;
    ArrayList<int[]> outputMap;    // rectified points
    static int width;
    static int height;
    
    // constants and parameters
    final static double a = -0.000910;
    final static double b = 1.527995;
    final static int CENTER_X = width/2;  // needs to be found
    final static int CENTER_Y = height/2;  // needs to be found
    
    public Rectification(int[] input, int w, int h) {
        outputMap = new ArrayList<int[]>();
        inputMap = input;
        width = w;
        height = h;
        rectify();
    }
    
    public ArrayList<int[]> getRectifiedPoints() {
        return outputMap;
    }


    protected void rectify() {
        // for all pixel correspondences in the input map
        // find the radius from the center
        // use lookup table to get rectified radius
        // recompute (x,y) and store in new data structure
        for (int i = 0; i < width; i++) {
            int cur_x = i;
            int cur_y = inputMap[i];
            double cur_r = getRadius(cur_x, cur_y);
            double cur_t = getAngle(cur_x, cur_y);
            double rect_r = getRectifiedRadius(cur_r);
            insertRectifiedPoint(rect_r, cur_t);
        }
        
    }
    
    // HELPER FUNCTIONS    
    protected double getRadius(int x, int y) {
        double dy = y - CENTER_Y;
        double dx = x - CENTER_X;
        return Math.sqrt(dx*dx + dy*dy);
    }
    protected double getAngle(int x, int y) {
        double dy = y - CENTER_Y;
        double dx = x - CENTER_X;
        return Math.atan2(dy, dx);
    }
    protected double getRectifiedRadius(double r) {
        return b*r + a*r*r;
    }
    protected void insertRectifiedPoint(double rp, double theta) {
        int nx = (int) Math.round(CENTER_X + rp*Math.cos(theta));
        int ny = (int) Math.round(CENTER_Y + rp*Math.sin(theta));
        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            int[] input = {nx, ny};
            outputMap.add(input);
        }
    }
}