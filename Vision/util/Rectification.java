package Vision.util;

import java.awt.image.BufferedImage;
import java.lang.Math;
import java.util.ArrayList;


public class Rectification {

    // args
    ArrayList<int[][]> inputMap;
    ArrayList<int[][]> outputMap;    // rectified points
    static int width;
    static int height;
    
    // constants and parameters
    final static double a = -0.000910;
    final static double b = 1.527995;
    final static int CENTER_X = width/2;  // needs to be found
    final static int CENTER_Y = height/2;  // needs to be found
    
    public Rectification(ArrayList<int[][]> input, int w, int h) {
        outputMap = new ArrayList<int[][]>();
        inputMap = input;
        width = w;
        height = h;
        rectify();
    }
    
    public ArrayList<int[][]> getRectifiedPoints() {
        return outputMap;
    }


    protected void rectify() {
        // for all pixel correspondences in the input map
        // find the radius from the center
        // use lookup table to get rectified radius
        // recompute (x,y) and store in new data structure
        for (int i = 0; i < inputMap.size(); i++) {
            int[][] cur_segment = inputMap.get(i);
            int init_x = cur_segment[0][0];
            int init_y = cur_segment[0][1];
            int fin_x = cur_segment[1][0];
            int fin_y = cur_segment[1][1];
            double init_r = getRadius(init_x, init_y);
            double init_t = getAngle(init_x, init_y);
            double rect_init_r = getRectifiedRadius(init_r);
            double fin_r = getRadius(fin_x, fin_y);
            double fin_t = getAngle(fin_x, fin_y);
            double rect_fin_r = getRectifiedRadius(fin_r);
			double[] init_point = new double[2];
            init_point[0] = rect_init_r; init_point[1] = init_t;
			double[] fin_point = new double[2];
            fin_point[0] = rect_fin_r; fin_point[1] = fin_t;
            insertRectifiedPoint(init_point, fin_point);
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
    protected void insertRectifiedPoint(double[] ip, double[] fp) {
        double init_rp = ip[0];
        double init_theta = ip[1];
        double fin_rp = fp[0];
        double fin_theta = fp[1];
        int init_nx = (int) Math.round(CENTER_X + init_rp*Math.cos(init_theta));
        int init_ny = (int) Math.round(CENTER_Y + init_rp*Math.sin(init_theta));
        int fin_nx = (int) Math.round(CENTER_X + fin_rp*Math.cos(fin_theta));
        int fin_ny = (int) Math.round(CENTER_Y + fin_rp*Math.sin(fin_theta));
        if (init_nx >= 0 && init_nx < width && init_ny >= 0 && init_ny < height &&
            fin_nx >= 0 && fin_nx < width && fin_nx >= 0 && fin_ny < height) {
			int[][] input = new int[2][2];
			input[0][0] = init_nx; input[0][1] = init_ny; input[1][0] = fin_nx; input[1][1] = fin_ny;
			//System.out.println(input[0][0] + ", " + input[0][1]);
			//System.out.println(input[1][0] + ", " + input[1][1]);
            outputMap.add(input);
        }
    }
}
