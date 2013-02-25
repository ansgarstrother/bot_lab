package Vision.LineDetection;

import java.awt.image.BufferedImage;
import java.io.*;
import java.util.ArrayList;



public class LineDetectionSegmentation {

    // const
    private final static int MIN_LENGTH = 25;
    
    // typedef
    public enum Direction {
        NONE, UPRIGHT, RIGHT, DOWNRIGHT
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
        for (int i = 0; i < segment_list.size(); i++) {
            fitData(segment_list.get(i));
        }
        
        
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
        int x; int prev_y;
        n = sum_x = sum_y = cur_x = cur_y = 0;  // all relate to current segment
        x = prev_y = 0;  // iteration vars
        ArrayList<int[]> segment = new ArrayList<int[]>();
        
        while (x < boundaryMap.length) {
            int y = boundaryMap[x];
            switch (dir) {
                case NONE:  // initialize direction
                    if (x+1 < boundaryMap.length) {
                        int next_y = boundaryMap[x+1];
                        if (next_y == y + 2 || next_y == y +3) {
                            dir = Direction.UPRIGHT;
                        }
                        else if (next_y > y - 1 && next_y < y + 1) {    // 1 pixel boundary
                            dir = Direction.RIGHT;
                        }
                        else if (next_y == y - 2 || next_y == y - 3) {
                            dir = Direction.DOWNRIGHT;
                        }
                        else {
                            break;  // do not add to list
                        }
                        int[] point = {x,y};
                        prev_y = y;
                        segment.add(point);
                        sum_x = sum_x + x;
                        sum_y = sum_y + y;
                        n = n + 1;
                    }
                    break;
                    
                case UPRIGHT:   // up right direction
                    if (prev_y == y - 2 || prev_y == y - 3) {
                        dir = Direction.UPRIGHT;
                        int[] point = {x,y};
                        prev_y = y;
                        segment.add(point);
                        sum_x = sum_x + x;
                        sum_y = sum_y + y;
                        n = n + 1;
                    }
                    else {
                        // check next 3 pixels for direction consistency
                        if (x < boundaryMap.length - 4) {
                            int y2 = boundaryMap[x+1];
                            int y3 = boundaryMap[x+2];
                            int y4 = boundaryMap[x+3];
                            if (prev_y == y2 - 3 || prev_y == y2 - 4 || prev_y == y2 - 5 ||
                                prev_y == y3 - 4 || prev_y == y3 - 5 || prev_y == y2 - 6 ||
                                prev_y == y4 - 5 || prev_y == y4 - 6 || prev_y == y4 - 7) {
                                dir = Direction.UPRIGHT;
                                int[] point = {x,y};
                                int[] point2 = {x+1,y2};
                                int[] point3 = {x+2,y3};
                                int[] point4 = {x+3,y4};
                                prev_y = y4;
                                segment.add(point);
                                segment.add(point2);
                                segment.add(point3);
                                segment.add(point4);
                                sum_x = sum_x + x;
                                sum_y = sum_y + y;
                                sum_x = sum_x + x;
                                sum_y = sum_y + y2;
                                sum_x = sum_x + x;
                                sum_y = sum_y + y3;
                                sum_x = sum_x + x;
                                sum_y = sum_y + y4;
                                n = n + 4;
                                x = x + 4;
                            }
                            else {
                                // break segment, start new
                                if (segment.size() > MIN_LENGTH) {
                                    segments object = new segments(segment, sum_x, sum_y, n);
                                    segment_list.add(object);
                                }
                                // reset
                                sum_x = sum_y = n = 0;
                                segment = new ArrayList<int[]>();
                                dir = Direction.NONE;
                                
                            }
                            break;
                        }
                        else {
                            break;  //end of file
                        }
                    }
                    break;
                    
                case RIGHT: // right direction
                    if (prev_y > y - 1 && prev_y < y + 1) {
                        dir = Direction.RIGHT;
                        int[] point = {x,y};
                        prev_y = y;
                        segment.add(point);
                        sum_x = sum_x + x;
                        sum_y = sum_y + y;
                        n = n + 1;
                    }
                    else {
                        // check next 3 pixels for direction consistency
                        if (x < boundaryMap.length - 4) {
                            int y2 = boundaryMap[x+1];
                            int y3 = boundaryMap[x+2];
                            int y4 = boundaryMap[x+3];
                            if (prev_y > y2 - 1 && prev_y < y2 + 1 ||
                                prev_y > y3 - 1 && prev_y < y3 + 1 ||
                                prev_y > y4 - 1 && prev_y < y4 + 1) {
                                dir = Direction.RIGHT;
                                int[] point = {x,y};
                                int[] point2 = {x+1,y2};
                                int[] point3 = {x+2,y3};
                                int[] point4 = {x+3,y4};
                                prev_y = y4;
                                segment.add(point);
                                segment.add(point2);
                                segment.add(point3);
                                segment.add(point4);
                                sum_x = sum_x + x;
                                sum_y = sum_y + y;
                                sum_x = sum_x + x;
                                sum_y = sum_y + y2;
                                sum_x = sum_x + x;
                                sum_y = sum_y + y3;
                                sum_x = sum_x + x;
                                sum_y = sum_y + y4;
                                n = n + 4;
                                x = x + 4;
                            }
                            else {
                                // break segment, start new
                                if (segment.size() > MIN_LENGTH) {
                                    segments object = new segments(segment, sum_x, sum_y, n);
                                    segment_list.add(object);
                                }
                                // reset
                                sum_x = sum_y = n = 0;
                                segment = new ArrayList<int[]>();
                                dir = Direction.NONE;
                                
                            }
                            break;
                        }
                        else {
                            break;  //end of file
                        }
                    }
                    break;
                    
                case DOWNRIGHT: // down right direction
                    if (prev_y == y + 2 || prev_y == y + 3) {
                        dir = Direction.DOWNRIGHT;
                        int[] point = {x,y};
                        prev_y = y;
                        segment.add(point);
                        sum_x = sum_x + x;
                        sum_y = sum_y + y;
                        n = n + 1;
                    }
                    else {
                        // check next 3 pixels for direction consistency
                        if (x < boundaryMap.length - 4) {
                            int y2 = boundaryMap[x+1];
                            int y3 = boundaryMap[x+2];
                            int y4 = boundaryMap[x+3];
                            if (prev_y == y2 + 3 || prev_y == y2 + 4 || prev_y == y2 + 5 ||
                                prev_y == y3 + 4 || prev_y == y3 + 5 || prev_y == y2 + 6 ||
                                prev_y == y4 + 5 || prev_y == y4 + 6 || prev_y == y4 + 7) {
                                dir = Direction.DOWNRIGHT;
                                int[] point = {x,y};
                                int[] point2 = {x+1,y2};
                                int[] point3 = {x+2,y3};
                                int[] point4 = {x+3,y4};
                                prev_y = y4;
                                segment.add(point);
                                segment.add(point2);
                                segment.add(point3);
                                segment.add(point4);
                                sum_x = sum_x + x;
                                sum_y = sum_y + y;
                                sum_x = sum_x + x;
                                sum_y = sum_y + y2;
                                sum_x = sum_x + x;
                                sum_y = sum_y + y3;
                                sum_x = sum_x + x;
                                sum_y = sum_y + y4;
                                n = n + 4;
                                x = x + 4;
                            }
                            else {
                                // break segment, start new
                                if (segment.size() > MIN_LENGTH) {
                                    segments object = new segments(segment, sum_x, sum_y, n);
                                    segment_list.add(object);
                                }
                                // reset
                                sum_x = sum_y = n = 0;
                                segment = new ArrayList<int[]>();
                                dir = Direction.NONE;
                                
                            }
                            break;
                        }
                        else {
                            break;  //end of file
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



