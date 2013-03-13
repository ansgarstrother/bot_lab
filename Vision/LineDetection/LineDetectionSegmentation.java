package Vision.LineDetection;

import java.awt.image.BufferedImage;
import java.io.*;
import java.util.ArrayList;
import java.util.Random;



public class LineDetectionSegmentation {
    
    // const
    private static final int MIN_SIZE = 48;	// data fit to model (d)
	private static final double MIN_ERROR = 3;	// min error for RANSAC (t)
	private static final int k = 400;		// num iterations for RANSAC (k)
	private static final int n = 2;			// num randomly selected points from data (n)

    
    // args
	private int width;
	private int height;

	private BufferedImage im;
    private int[] boundaryMap;

	// RANSAC input
	private ArrayList<int[]> data;
    protected ArrayList<int[][]> segment_list;
    

    public LineDetectionSegmentation(int[] bm, BufferedImage im) {
        // init
        boundaryMap = bm;
        segment_list = new ArrayList<int[][]>();
		this.width = im.getWidth();
		this.height = im.getHeight();
		this.im = im;

		// init data array
		this.data = new ArrayList<int[]>();
		for (int i = 0; i < width; i++) {
			if (boundaryMap[i] != 0) {
				int[] point = new int[2];
				point[0] = i; point[1] = boundaryMap[i];
				data.add(point);
			}
		}
        
        // traverse and find segments
        traverse();
        System.out.println(segment_list.size());
        
    }
    
    protected void traverse() {
        // RANSAC ALGORITHM
		//	1. pick two random points from data array
		//	2. fit a line between two points
		//	3. run through all points in the array and test fit to line
		//		- if there is a high fit, add to hypothesis inliers
		//		- if there is a high count of inliers, remove these points from data and add line to segment array
		//	4. Repeat until we try many iterations
		int iterations = 0;
		Random randomizer = new Random();
		while (iterations < k) {
			int index_a = randomizer.nextInt(boundaryMap.length);
			int index_b = randomizer.nextInt(boundaryMap.length);
			int[] point_a = new int[2]; point_a[0] = index_a; point_a[1] = boundaryMap[index_a];
			int[] point_b = new int[2]; point_b[0] = index_b; point_b[1] = boundaryMap[index_b];

			while (point_a[1] == 0 || point_b[1] == 0) {
				index_a = randomizer.nextInt(boundaryMap.length);
				index_b = randomizer.nextInt(boundaryMap.length);
				point_a = new int[2]; point_a[0] = index_a; point_a[1] = boundaryMap[index_a];
				point_b = new int[2]; point_b[0] = index_b; point_b[1] = boundaryMap[index_b];
			}
			if (point_a[1] == 0 || point_b[1] == 0 || point_a[0] == point_b[0]) {
				// skip
			}
			else {
				double[] model = new double[2];
				model = getLinearModel(point_a, point_b);

			
				// find consensus points
				ArrayList<int[]> consensusSet = new ArrayList<int[]>();
				consensusSet.add(point_a); consensusSet.add(point_b);
				for (int i = 0; i < data.size(); i++) {
					if (i == index_a || i == index_b) {
						// skip
					}
					else {
						int[] test_point = {i, boundaryMap[i]};
						if (test_point[1] == 0) {
							// skip
						}
						else {
							double line_y = model[0] * test_point[0] + model[1];
							double error = Math.abs(line_y - test_point[1]);
							if (error < MIN_ERROR) {
								// insert into consensusSet
								int[] in = new int[2]; in[0] = test_point[0]; in[1] = test_point[1];
								consensusSet.add(in);
							}
						}
					}
				}
			
				// test consensus set
				if (consensusSet.size() > MIN_SIZE) {
					// add line segment to segments list
					// remove points from data list
					int[] init_point = consensusSet.get(0);
					int[] final_point = consensusSet.get(consensusSet.size()-1);
					int[][] segment = new int[2][2];
					segment[0][0] = init_point[0]; segment[0][1] = init_point[1];
					segment[1][0] = final_point[0]; segment[1][0] = final_point[1];
					segment_list.add(segment);
	
					for (int i = 0; i < consensusSet.size(); i++) {
						int[] remove_point = consensusSet.get(i);
						boundaryMap[remove_point[0]] = 0;	// remove point from boundary map
						for (int g = -2; g < 2; g++) {
							if (remove_point[0] + g < width && remove_point[0] + g > 0 &&
									remove_point[1] + g < height && remove_point[1] + g > 0) {
								im.setRGB(remove_point[0] + g, remove_point[1] + g, 0xff0000ff);
							}
						}
					}
					
				}
			}
			
			// increment iterations
			iterations++;
		}
			
		




/*
		int mark = 0;
		while (mark + MIN_SIZE < width) {
			// RETRIEVE DATA CLUSTER
			double[] cluster_x = new double[MIN_SIZE];
			double[] cluster_y = new double[MIN_SIZE];
			// read in data, compute mean
			double sumx = 0; double sumy = 0; double sumx2 = 0;
			for (int i = 0; i < MIN_SIZE; i++) {
				cluster_x[i] = mark + i; cluster_y[i] = boundaryMap[mark + i];
				sumx = sumx + cluster_x[i];
				sumx2 = sumx2 + (cluster_x[i] * cluster_x[i]);
				sumy = sumy + cluster_y[i];
			}
			// COMPUTE SUMMARY STATISTICS
			double xbar = sumx / MIN_SIZE;
			double ybar = sumy / MIN_SIZE;
			double xxbar = 0; double yybar= 0; double xybar = 0;
			for (int i = 0; i < MIN_SIZE; i++) {
				xxbar = xxbar + (cluster_x[i] - xbar) * (cluster_x[i] - xbar);
				yybar = yybar + (cluster_y[i] - ybar) * (cluster_y[i] - ybar);
				xybar = xybar + (cluster_x[i] - xbar) * (cluster_y[i] - ybar);
			}
			double m = xybar / xxbar;		// linear slope
			double b = ybar - m * xbar;	//linear y offset
			int df = MIN_SIZE - 2;
			double rss = 0;	// residual error
			double ssr = 0;	// sum of squared error
			for (int i = 0; i < MIN_SIZE; i++) {
				double fit = m*cluster_x[i] + b;
				rss = rss + (fit - cluster_y[i]) * (fit - cluster_y[i]);
				ssr = ssr + (fit - ybar) * (fit - ybar);
			}
			double R2 = ssr / yybar;
			System.out.println("R^2 = " + R2);
        	System.out.println("SSE  = " + rss);
        	System.out.println("SSR  = " + ssr);

			// TEST ERROR RATES WITH THRESHOLD
			if (R2 > MIN_R2_ERROR) {
				// add to segments array
				double[][] segment = new double[2][2];
				segment[0][0] = cluster_x[0]; segment[0][1] = m * cluster_x[0] + b;
				segment[1][0] = cluster_x[MIN_SIZE - 1]; segment[1][1] = m * cluster_x[MIN_SIZE - 1] + b;
				segment_list.add(segment);

				// print mark
				System.out.println("Mark: " + mark);
				for (int i = 0; i < MIN_SIZE; i++) {
					im.setRGB((int)(cluster_x[i]), (int)(cluster_y[i]), 0xff0000ff);
				}

				// increment by MIN_SIZE
				mark = mark + MIN_SIZE;
			}
			else {
				// increment in half
				mark = mark + MIN_SIZE / 2;
			}
		}
*/			

    }

	private double[] getLinearModel(int[] a, int[] b) {
		// retrieves slope and y offset of a line between two points
		double m = (a[1] - b[1]) / (a[0] - b[0]);
		double beta = (-m * a[0] + a[1]);
		double[] ret = new double[2];
		ret[0] = m; ret[1] = beta;
		return ret;
	}

    
    public ArrayList<int[][]> getSegments() {
        return segment_list;
    }
	public BufferedImage getImage() {
		return im;
	}
    
}



