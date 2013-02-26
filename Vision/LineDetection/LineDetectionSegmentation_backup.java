package Vision.LineDetection;

import java.awt.image.BufferedImage;
import java.io.*;
import java.util.ArrayList;



public class LineDetectionSegmentation {

    // const
    private final static int MIN_LENGTH = 40;
    private final static int ERROR_THRESHOLD = 2;
    private final static int STEP = 1;
    
    // typedef
    public enum Direction {
        NONE, SAME
    }
    
    // args
    private int[] boundaryMap;
    private Direction dir;
    private ArrayList<segments> segment_list;
    
    public LineDetectionSegmentation(int[] bm) {
        // init
        boundaryMap = bm;
        dir = Direction.NONE;
        segment_list = new ArrayList<segments>();
        
        // traverse and find segments
        traverse();
        System.out.println(segment_list.size());
        /*
        for (int i = 0; i < segment_list.size(); i++) {
            fitData(segment_list.get(i));
        }
        */
        
    }
    
    
    protected void fitData(segments sm) {
        // fit points in the array to a line
        ArrayList<int[]> points = sm.segment_points;
        int sum_x = sm.sum_x;
        int sum_y = sm.sum_y;
        int n = sm.n;
        double x_mu = sum_x / n;
        double y_mu = sum_y / n;
        double x_var = 0;
        double y_var = 0;
        double covar = 0;
        
        // variance
        for (int i = 0; i < points.size(); i++) {
            x_var += (points.get(i)[0] - x_mu) * (points.get(i)[0] - x_mu);
            y_var += (points.get(i)[1] - y_mu) * (points.get(i)[1] - y_mu);
            covar += (points.get(i)[0] - x_mu) * (points.get(i)[1] - y_mu);
        }
        
        // y = mx + b
        double m = covar / x_var;
        double b = y_mu - m * x_mu;
        int[] init_point = {points.get(0)[0], points.get(0)[1]};
        int[] final_point = {points.get(points.size())[0], points.get(points.size())[1]};
        
        System.out.println("y = " + m + "x + " + b);
        System.out.println("init point: (" + init_point[0] + "," + init_point[1] + ")");
        System.out.println("final point: (" + final_point[0] + "," + final_point[1] + ")");
        System.out.println("");
        
    }

    protected void traverse() {
        // segment all points in boundary map
        // find break points, fit data to lines, repeat
        // enforce that pixels must be within 2 pixel between each other to continue a segment
        // enfore a segment must be at least 40 pixels long
        int n; int sum_x; int sum_y; int cur_x; int cur_y;
        int x; int prev_y; int slope;
        n = sum_x = sum_y = cur_x = cur_y = 0;  // all relate to current segment
        x = prev_y = slope = 0;  // iteration vars
        ArrayList<int[]> segment = new ArrayList<int[]>();
        
        while (x < boundaryMap.length) {
            int y = boundaryMap[x];
            System.out.println(x + ", " + dir);
            switch (dir) {
                case NONE:  // initialize direction
                    if (x+STEP < boundaryMap.length) {
                        // retrieve new slope
                        int next_y = boundaryMap[x+STEP-1];
                        slope = next_y - y;
                        // update directional state
                        dir = Direction.SAME;
                        // add five points to line
                        for (int i = 0; i < STEP; i++) {
                            int[] point = {x+i,boundaryMap[x+i]};
                            segment.add(point);
                            sum_x = sum_x + x;
                            sum_y = sum_y + boundaryMap[x+i];
                            n = n + 1;
                        }
                        x = x + STEP-1;
                    }
                    else {
                        x = x + STEP-1;
                        break;  //eof
                    }
                    break;
                    
                case SAME:  // same direction i.e. similar slope
                    if (x+STEP < boundaryMap.length) {
                        // retrieve new slope
                        int next_y = boundaryMap[x+STEP-1];
                        int test_slope = next_y - y;
                        // similar slope, add to segment
                        if (test_slope < slope + ERROR_THRESHOLD &&
                            test_slope > slope - ERROR_THRESHOLD) {
                            dir = Direction.SAME;
                            // add five points to line
                            for (int i = 0; i < STEP; i++) {
                                int[] point = {x+i,boundaryMap[x+i]};
                                segment.add(point);
                                sum_x = sum_x + x;
                                sum_y = sum_y + boundaryMap[x+i];
                                n = n + 1;
                            }
                            x = x + STEP-1;
                            //slope = test_slope;
                        }
                        // different slope, test segment length, refresh
                        else {
                            // run secondary test
                            if (x+STEP+STEP < boundaryMap.length) {
                                int last_y = boundaryMap[x+STEP+STEP-2];
                                int last_slope = last_y - y;
                                // similar slope, add to segment
                                if (last_slope < slope/2 + ERROR_THRESHOLD &&
                                    last_slope > slope/2 - ERROR_THRESHOLD) {
                                    dir = Direction.SAME;
                                    // add five points to line
                                    for (int i = 0; i < 2*STEP; i++) {
                                        int[] point = {x+i,boundaryMap[x+i]};
                                        segment.add(point);
                                        sum_x = sum_x + x;
                                        sum_y = sum_y + boundaryMap[x+i];
                                        n = n + 1;
                                    }
                                    x = x + STEP+STEP-2;
                                    //slope = last_slope/2;
                                }
                                else {
                                    if (n > MIN_LENGTH) {
                                        segments object = new segments(segment, sum_x, sum_y, n);
                                        segment_list.add(object);
                                    }
                                    n = sum_x = sum_y = 0;
                                    dir = Direction.NONE;
                                    segment = new ArrayList<int[]>();
                                }
                            }
                        }
                    }
                    else {
                        x = x + STEP-1;
                        break;  //eof
                    }
                    break;

                    
                default:
                    System.err.println("Error. Invalid Direction during Line Segmentation");
                    x = boundaryMap.length;
                    break;
            }
            
            x++;    // increment to continue
        }
        
    }

}





class segments {
    public ArrayList<int[]> segment_points;
    public int sum_x;
    public int sum_y;
    public int n;
    
    public segments(ArrayList<int[]> sp, int sx, int sy, int n) {
        segment_points = sp;
        sum_x = sx;
        sum_y = sy;
        this.n = n;
        
    }
    
    public ArrayList<int[]> ga() {
        return segment_points;
    }
}



