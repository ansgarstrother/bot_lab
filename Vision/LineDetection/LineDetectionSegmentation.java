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
	private static final int q = 10;		// num iterations for finding best lines (max = 10)
	private static final int n = 2;			// num randomly selected points from data (n)

	private static final double inf = Double.POSITIVE_INFINITY;

    
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
		int trials = 0;

		while (trials < q) {
			// init
			double[] best_model = new double[5];
			// reset best results
			for (int i = 0; i < best_model.length - 1; i++) {
				best_model[i] = inf;
			}
			best_model[4] = 0;
			ArrayList<int[]> best_consensusSet = new ArrayList<int[]>();
			Random randomizer = new Random();
			int iterations = 0;

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
						// create a new model reflecting points in consensus set
						// model_stats = [m b rss ssr R2]
						double[] model_stats = linearRegression(consensusSet);
						System.out.println("Model Stats:");
						System.out.println(model_stats[2] + " " + model_stats[3] + " " + model_stats[4]);
						System.out.println("Best Stats:");
						System.out.println(best_model[2] + " " + best_model[3] + " " + best_model[4]);

						// evaluate model stats error
						if ((model_stats[2] + model_stats[3] < best_model[2] + best_model[3]) && model_stats[4] > best_model[4]) {
							best_model = model_stats.clone();
							best_consensusSet.clear();
							best_consensusSet.addAll(consensusSet);
						}
					}
				}
			
				// increment iterations
				iterations++;
			}
			// retrieve best linear fit, remove points, add to segments list
			// add line segment to segments list
			// remove points from data list
			if (best_consensusSet.size() != 0) {
				int[] init_point = best_consensusSet.get(0);
				int[] final_point = best_consensusSet.get(best_consensusSet.size()-1);
				int[][] segment = new int[2][2];
				segment[0][0] = init_point[0]; segment[0][1] = init_point[1];
				segment[1][0] = final_point[0]; segment[1][0] = final_point[1];
				segment_list.add(segment);
	
				for (int i = 0; i < best_consensusSet.size(); i++) {
					int[] remove_point = best_consensusSet.get(i);
					boundaryMap[remove_point[0]] = 0;	// remove point from boundary map
					for (int g = -2; g < 2; g++) {
						if (remove_point[0] + g < width && remove_point[0] + g > 0 &&
								remove_point[1] + g < height && remove_point[1] + g > 0) {
							im.setRGB(remove_point[0] + g, remove_point[1] + g, 0xff0000ff);
						}
					}
				}
			}
			// increment trials
			trials++;
		}	

    }

	private double[] getLinearModel(int[] a, int[] b) {
		// retrieves slope and y offset of a line between two points
		double m = (a[1] - b[1]) / (a[0] - b[0]);
		double beta = (-m * a[0] + a[1]);
		double[] ret = new double[2];
		ret[0] = m; ret[1] = beta;
		return ret;
	}

	private double[] linearRegression(ArrayList<int[]> set) {
		// perform linear regression on all points in set
		// return vector: [m b rss ssr R2]
		// read in data, compute mean
		double sumx = 0; double sumy = 0; double sumx2 = 0;
		for (int i = 0; i < set.size(); i++) {
			int[] point = set.get(i);
			sumx = sumx + point[0];
			sumx2 = sumx2 + (point[0] * point[0]);
			sumy = sumy + point[1];
		}
		// COMPUTE SUMMARY STATISTICS
		double xbar = sumx / set.size();
		double ybar = sumy / set.size();
		double xxbar = 0; double yybar= 0; double xybar = 0;
		for (int i = 0; i < set.size(); i++) {
			int[] point = set.get(i);
			xxbar = xxbar + (point[0] - xbar) * (point[0] - xbar);
			yybar = yybar + (point[1] - ybar) * (point[1] - ybar);
			xybar = xybar + (point[0] - xbar) * (point[1] - ybar);
		}
		double m = xybar / xxbar;		// linear slope
		double b = ybar - m * xbar;	//linear y offset
		int df = MIN_SIZE - 2;
		double rss = 0;	// residual error
		double ssr = 0;	// sum of squared error
		for (int i = 0; i < MIN_SIZE; i++) {
			int[] point = set.get(i);
			double fit = m*point[0] + b;
			rss = rss + (fit - point[1]) * (fit - point[1]);
			ssr = ssr + (fit - ybar) * (fit - ybar);
		}
		double R2 = ssr / yybar;

		double[] stats = new double[5];
		stats[0] = m; stats[1] = b; stats[2] = rss; stats[3] = ssr; stats[4] = R2;
		return stats;
	}

    
    public ArrayList<int[][]> getSegments() {
        return segment_list;
    }
	public BufferedImage getImage() {
		return im;
	}
    
}



