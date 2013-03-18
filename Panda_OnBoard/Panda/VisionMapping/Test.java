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


public class Test extends JFrame {

    private double[] intrinsics = {640.1483, 676.0408, 480.3221};


    private ImageSource imageSource;
    private String cameraURL;
    private Thread projThread;
    private JFrame jf = new JFrame ("Projection");
    private JImage jim = new JImage();

    private PandaPositioning pp;

    // constructor
    public Test() {

        jf.setLayout (new BorderLayout());
        jf.add (jim, BorderLayout.CENTER);
        jf.setSize (1024, 768);
        jf.setVisible (true);
        jf.setDefaultCloseOperation (JFrame.EXIT_ON_CLOSE);

	    pp = new PandaPositioning();
        // initialize count
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
                    ImageSourceFormat fmt = Test.this.imageSource.getCurrentFormat();
                    while (true) {
                        byte[] buf = Test.this.imageSource.getFrame().data;

                        if (buf == null)
                            continue;

                        BufferedImage im = ImageConvert.convertToImage (
                                                    fmt.format,
                                                    fmt.width,
                                                    fmt.height,
                                                    buf);

			/*
			// DRAW CENTER LINE IN BLUE
			int column = fmt.width / 2;
			for (int i = 0; i < fmt.height; i++) {
				im.setRGB(column, i, 0x00ff0000ff);
			}
			*/

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
        Test.this.projThread.start();


    }

    // need to perform least squares regression
    // outputs the predicted pixels, given the real world coordinates and estimated inputs
    // inputs of the form [f, c_x, cy]



    public void mouseClickHandler (MouseEvent me) {

        // must click from left to right, closest to furthest
        // first click must be MM, then row from left to right


		// Retrieve clicked point, convert
		Point input = me.getPoint();

		final Point guiPoint = new Point(input.x, input.y);
		System.out.println("Clicked at " + guiPoint + " w.r.t GUI.");
		AffineTransform imageTransform = null;
		try {
        		 imageTransform = this.jim.getAffine().createInverse();
		} catch ( Exception e ) {
			System.out.println("Fuck.");
			return;
		}
        	Point2D imagePoint = imageTransform.transform(guiPoint, null);
        	System.out.println("Clicked at " + imagePoint + " w.r.t image.");
	        //System.out.println("count = " + count);
            //testPixels[count].setLocation(imagePoint.getX(), imagePoint.getY());

            double[] pixels = {imagePoint.getX(), imagePoint.getY() };


            Matrix mat = pp.getGlobalPoint (intrinsics, pixels, false);
            System.out.println(mat);

    }

    public static void main(String[] args) {
        Test test = new Test();


    }



}
