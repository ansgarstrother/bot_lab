package Panda.VisionMapping;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;
import java.awt.event.*;
import java.awt.Point;
import java.awt.geom.*;
import javax.swing.*;

import april.jcam.*;
import april.util.*;
import april.jmat.*;

public class Projection extends JFrame {

    private final double cameraHeight = .2032; // height of camera off the ground
    private double z1 = 0.38735; // 3 sets of distances away from camera
    private double z2 = 0.43815;
    private double z3 = 0.48895;
    private double z4 = 0.53975;
    private double z5 = 0.59055;
    private double z6 = 0.64135;
    private double z7 = 0.69215;
    private double z8 = 0.74295;
    private double z9 = 0.79375;


    private double x1 = -0.36195; // 3 sets of distances spanning from left to right
    private double x2 = -0.1397;
    private double x3 = 0.4;

    private double y = -cameraHeight;

    // click points for calibration

    private double lengths[];

    private double f;
    private double c_x;
    private double c_y;
    private double imageLength;
    private double objectDistance;
    private double objectHeight;

    private int count;		// used to keep track of which points we have clicked

    private double[][] testCoords = { {x1, y, z1},
                                        {x1, y, z2},
                                        {x1, y, z3},
                                        {x1, y, z4},
                                        {x1, y, z5},
                                        {x2, y, z6},
                                        {x2, y, z7},
                                        {x2, y, z8},
                                        {x2, y, z9} };


    private Point[] testPixels;



    private ImageSource imageSource;
    private String cameraURL;
    private Thread projThread;
    private JFrame jf = new JFrame ("Projection");
    private JImage jim = new JImage();


    // constructor
    public Projection() {

        jf.setLayout (new BorderLayout());
        jf.add (jim, BorderLayout.CENTER);
        jf.setSize (1024, 768);
        jf.setVisible (true);
        jf.setDefaultCloseOperation (JFrame.EXIT_ON_CLOSE);

        testPixels = new Point[9];
        lengths = new double[3];

        // fill corresponding real world coordinates for points
        // always strat from bottom left, bottom middle, bottom right, middle left, etc.


        for (int i=0; i < testPixels.length; i++) {
            testPixels[i] = new Point(-1, -1);
        }

	    // initialize count
	    count = 0;
        jim.addMouseListener (new MouseAdapter() {
            public void mouseClicked (MouseEvent me) {
                mouseClickHandler (me);
            }
        });


        getCamera();
        startCamera();


    }

    public void getCamera() {
        ArrayList<String> urls = ImageSource.getCameraURLs();
        if (urls.size() == 0) {
            System.out.println("No cameras found");
            return;
        }

        this.cameraURL = urls.get(0);
        System.out.println (this.cameraURL);



    }

    public void startCamera() {
        try {
            this.imageSource = ImageSource.make (this.cameraURL);
        } catch (IOException e) {
            System.out.println (e);
        }

        this.imageSource.start ();
        this.projThread = new Thread (new Runnable() {
            @Override
                public void run () {
                    ImageSourceFormat fmt = Projection.this.imageSource.getCurrentFormat();
                    while (true) {
                        byte[] buf = Projection.this.imageSource.getFrame().data;

                        if (buf == null)
                            continue;

                        BufferedImage im = ImageConvert.convertToImage (
                                                    fmt.format,
                                                    fmt.width,
                                                    fmt.height,
                                                    buf);

			// RECTIFY IMAGE
			// Consistent with Rectification Process Vision/util/
			BufferedImage im2 = new BufferedImage(fmt.width, fmt.height, BufferedImage.TYPE_INT_RGB);
                	double cx = fmt.width / 2.0;
                	double cy = fmt.height / 2.0;

                	double B = -0.000910;
                	double A = 1.527995;

                	for (int y = 0; y < fmt.height; y++) {
                    		for (int x = 0; x < fmt.width; x++) {

                        		double dy = y - cy;
                        		double dx = x - cx;

                        		double theta = Math.atan2(dy, dx);
                        		double r = Math.sqrt(dy*dy+dx*dx);

                        		double rp = A*r + B*r*r;

                        		int nx = (int) Math.round(cx + rp*Math.cos(theta));
                        		int ny = (int) Math.round(cy + rp*Math.sin(theta));

                        		if (nx >= 0 && nx < fmt.width && ny >= 0 && ny < fmt.height) {
                            			im2.setRGB(x, y, im.getRGB((int) nx, (int) ny));
                        		}

                    		}
                	}
                        jim.setImage (im2);

                    }
                }
        });
        Projection.this.projThread.start();


    }

    // need to perform least squares regression
    // outputs the predicted pixels, given the real world coordinates and estimated inputs
    // inputs of the form [f, c_x, cy]

    protected void calculateLengths() {



        double [][] A = new double[2*testCoords.length][3];
        double [][] pixelVec = new double[2*testCoords.length][1];
        int j = 0;
        for (int i=0; i < A.length; i+= 2) {
            A[i][0] = testCoords[j][0] / testCoords[j][2];
            A[i][1] = 1;
            A[i][2] = 0;

            pixelVec[i][0] = testPixels[j].getX();
            A[i+1][0] = (testCoords[j][1] / testCoords[j][2]);
            A[i+1][1] = 0;
            A[i+1][2] = 1;

            pixelVec[i+1][0] = testPixels[j].getY();
            j++;

        }



        Matrix Amat = new Matrix (A);
        Matrix pixelVecMat = new Matrix (pixelVec);


        Matrix AmatSq = Amat.transpose().times (Amat);
        Matrix result1 = AmatSq.inverse().times(Amat.transpose());
        Matrix result = result1.times(pixelVecMat);

        this.f = result.get(0,0);
        this.c_x = result.get(1,0);
        this.c_y = result.get(2,0);

        System.out.println(this.f);
        System.out.println(this.c_x);
        System.out.println(this.c_y);

        // matrix returned and printed to file
        // matrix will then be imported and used in main controller
        double[][] calibMatrix = returnMatrix();
        try {
            FileWriter fstream = new FileWriter("intrinsic.txt");
            BufferedWriter out = new BufferedWriter(fstream);
            for (int i = 0; i < calibMatrix.length; i++) {
                out.write(calibMatrix[i][0] + " " +
                        calibMatrix[i][1] + " " +
                        calibMatrix[i][2] + " " +
                        calibMatrix[i][3] + "\n"
                        );
            }
        }
        catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
        }

    }


    public double[][] returnMatrix() {
        double[][] A = { {f, 0, c_x, 0},
                         {0, f,c_y, 0},
                         {0, 0, 1, 0} };


        return A;

    }


    public void mouseClickHandler (MouseEvent me) {

        // must click from left to right, closest to furthest
        // first click must be MM, then row from left to right

        if (count < 9) {

            Point input = me.getPoint();
            System.out.println ("Clicked at " + input.x + " " + input.y);
	        System.out.println("count = " + count);
            testPixels[count] = input;

            count++;
        }

        else { // done, go calculate
	   System.out.println("calculating focal length");
           calculateLengths();

        }

    }



}
