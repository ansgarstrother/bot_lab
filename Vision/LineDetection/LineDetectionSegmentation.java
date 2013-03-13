package Vision.LineDetection;

import java.awt.image.BufferedImage;
import java.io.*;
import java.util.ArrayList;



public class LineDetectionSegmentation {
    
    // const
    private final static int MIN_LENGTH = 10;
    private final static int ERROR_THRESHOLD = 3;
    
    // typedef
    public enum Direction {
        NONE, SAME
    }
    
    // args
    private int[] boundaryMap;
    private Direction dir;
    protected ArrayList<int[][]> segment_list;
    
    public LineDetectionSegmentation(int[] bm) {
        // init
        boundaryMap = bm;
        dir = Direction.NONE;
        segment_list = new ArrayList<int[][]>();
        
        // traverse and find segments
        traverse();
        //System.out.println(segment_list.size());
        
    }
    
    protected void traverse() {
        // segment all points in boundary map
        // find break points, fit data to lines, repeat
        // enforce that pixels must be within 2 pixel between each other to continue a segment
        // enfore a segment must be at least 40 pixels long
        int n = 0;
        int x; int slope;
        x = slope = 0;  // iteration vars
        int[] init_point = {-1,-1};
        int[] final_point = {-1,-1};
        
        while (x < boundaryMap.length) {
            int y = boundaryMap[x];
            //System.out.println(x + ", " + dir);
            switch (dir) {
                case NONE:  // initialize direction
                    if (x+slope < boundaryMap.length) {
                        // retrieve new slope
                        int next_y = boundaryMap[x+1];
                        int c = 0;
                        while (next_y == y && x+c+2 < boundaryMap.length) {
                            c++;
                            next_y = boundaryMap[x+c+1];
                        }
                        // update directional state
                        dir = Direction.SAME;
                        // update slope, x, init_point, and n
                        init_point[0] = x; init_point[1] = y;
                        slope = c;
                        x = x + slope;
                        n++;
                    }
                    break;
                    
                case SAME:  // same direction i.e. similar slope
                    if (x+slope+1 < boundaryMap.length) {
                        int next_y = boundaryMap[x+1];
                        int c = 0;
                        while (next_y == y && x+c+2 < boundaryMap.length) {
                            c++;
                            next_y = boundaryMap[x+c+1];
                        }
                        // same line
                        if (c < slope + ERROR_THRESHOLD && c > slope - ERROR_THRESHOLD) {
                            n++;
                        }
                        // different line
                        else {
                            if (n > MIN_LENGTH) {
                                final_point[0] = x; final_point[1] = y;
                                System.out.println(init_point[0] + " " + init_point[1]);
                                System.out.println(final_point[0] + " " + final_point[1]);
            					int[][] object = new int[2][2];
								object[0][0] = init_point[0]; object[0][1] = init_point[1];
								object[1][0] = final_point[0]; object[1][1] = final_point[1];
            					segment_list.add(object);
                            }
                            // reset all vars
                            dir = Direction.NONE;
                            n = 0;
                            slope = 0;
                        }
                    }
                    break;
                    
                    
                default:
                    System.err.println("Error. Invalid Direction during Line Segmentation");
                    x = boundaryMap.length;
                    break;
            }
            x++;    // increment to continue
        }

        if (n > MIN_LENGTH) {
            int y = boundaryMap[boundaryMap.length-1];
            final_point[0] = boundaryMap.length-1; final_point[1] = y;
            System.out.println(init_point[0] + " " + init_point[1]);
            System.out.println(final_point[0] + " " + final_point[1]);
            int[][] object = new int[2][2];
			object[0][0] = init_point[0]; object[0][1] = init_point[1];
			object[1][0] = final_point[0]; object[1][1] = final_point[1];
            segment_list.add(object);
        }

    }
    
    public ArrayList<int[][]> getSegments() {
        return segment_list;
    }
    
}



