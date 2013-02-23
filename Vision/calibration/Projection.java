package Vision.calibration;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.image.*;
import java.awt.event.*;
import java.awt.Point;
import javax.swing.*;

import april.jcam.*;
import april.util.*;
import april.jmat.*;

public class Projection extends JFrame {


    // click points for calibration
    private Point MM;
    private Point TL;
    private Point TM;
    private Point TR;
    private Point ML;
    private Point MR;
    private Point BL;
    private Point BM;
    private Point BR;

    private double f;
    private double c_x;
    private double c_y;
    private double imageLength;
    private double objectDistance = -1;
    private double objectHeight;

    private int count;		// used to keep track of which points we have clicked




    private ImageSource imageSource;
    private String cameraURL;
    private Thread projThread;
    private JFrame jf = new JFrame ("Projection");
    private JImage jim = new JImage();

    // constructor
    public Projection(String args[]) {

        jf.setLayout (new BorderLayout());
        jf.add (jim, BorderLayout.CENTER);
        jf.setSize (1024, 768);
        jf.setVisible (true);
        jf.setDefaultCloseOperation (JFrame.EXIT_ON_CLOSE);

        jim.addMouseListener (new MouseAdapter() {
            public void mouseClicked (MouseEvent me) {
                mouseClickHandler (me);
            }
        });

        // set points to -1 initially
        this.MM = new Point (-1, -1);
        this.TL = new Point (-1, -1);
        this.TM = new Point (-1, -1);
        this.TR = new Point (-1, -1);
        this.ML = new Point (-1, -1);
        this.MR = new Point (-1, -1);
        this.BL = new Point (-1, -1);
        this.BM = new Point (-1, -1);
        this.BR = new Point (-1, -1);

        // set object distance and height for calculations
        this.objectDistance = Double.parseDouble (args[0]);	// distance of object from camera
        this.objectHeight = Double.parseDouble (args[1]);	// height of the camera from the ground

	// initialize count
	count = 0;

        getCamera();
        startCamera();


    }

    public void getCamera() {
        ArrayList<String> urls = ImageSource.getCameraURLs();
        if (urls.size() == 0) {
            System.out.println ("No cameras found");
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
                            continue;tream = new FileWr

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

    protected void calculateFocalLength() {
        assert (this.objectDistance != -1);

        this.imageLength = averageImageLength();
        this.f = (this.imageLength*this.objectDistance) / (this.objectHeight);

        this.c_x = this.MM.x - 1024/2;
        this.c_y = this.MM.y - 768/2;

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

    protected double[][] returnMatrix() {
        double[][] A = { {f, 0, c_x, 0},
                         {0, f,c_y, 0},
                         {0, 0, 1, 0} };


        return A;

    }


    protected double averageImageLength() {
        int d1 = Math.abs (this.TM.x - this.TL.x);
        int d2 = Math.abs (this.TR.x - this.TM.x);
        int d3 = Math.abs (this.MM.x - this.ML.x);
        int d4 = Math.abs (this.MR.x - this.MM.x);
        int d5 = Math.abs (this.BM.x - this.BL.x);
        int d6 = Math.abs (this.BR.x - this.BM.x);
        int d7 = Math.abs (this.ML.y - this.TL.y);
        int d8 = Math.abs (this.MM.y - this.TM.y);
        int d9 = Math.abs (this.MR.y - this.TR.y);

        double average = (d1 + d2 + d3 + d4 + d5 + d6 + d7 + d8 + d9)/9;
        return average;



    }


    public void mouseClickHandler (MouseEvent me) {
        // first click must be MM, then row from left to right


        Point input = me.getPoint();
	System.out.println("count = " + count);
	if (count < 9) {
        	System.out.println ("Mouse clicked at " + input.x + " " + input.y);
	}

        if (count == 0) {
            this.MM = input;
	    count++;

        }

        else if (count == 1) {
            this.TL = input;
	    count++;
        }

        else if (count == 2) {
            this.TM = input;
	    count++;

        }

        else if (count == 3) {
            this.TR = input;
	    count++;

        }

        else if (count == 4) {
            this.ML = input;
	    count++;

        }

        else if (count == 5) {
            this.MR = input;
	    count++;

        }

        else if (count == 6) {
            this.BL = input;
	    count++;

        }

        else if (count == 7) {
            this.BM = input;
	    count++;

        }

        else if (count == 8) {
            this.BR = input;
	    count++;

        }

        else { // done, go calculate
	   System.out.println("calculating focal length");
           calculateFocalLength();

        }
    }



}
