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
        this.objectDistance = Double.parseDouble (args[0]);
        this.objectHeight = Double.parseDouble (args[1]);


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
                            continue;

                        BufferedImage im = ImageConvert.convertToImage (
                                                    fmt.format,
                                                    fmt.width,
                                                    fmt.height,
                                                    buf);

                        jim.setImage (im);

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
        // first click must be MM, then clockwise from top left


        Point input = me.getPoint();
        System.out.println ("Mouse clicked at " + input.x + " " + input.y);

        if (this.MM.x == -1 || this.MM.y == -1) {
            this.MM = input;

        }

        else if (this.TL.x == -1 || this.TL.y == -1) {
            this.TL = input;
        }

        else if (this.TM.x == -1 || this.TM.y == -1) {
            this.TM = input;

        }

        else if (this.TR.x == -1 || this.TR.y == -1) {
            this.TR = input;

        }

        else if (this.ML.x == -1 || this.ML.y == -1) {
            this.ML = input;

        }

        else if (this.MR.x == -1 || this.MR.y == -1) {
            this.MR = input;

        }

        else if (this.BL.x == -1 || this.BL.y == -1) {
            this.BL = input;

        }

        else if (this.BM.x == -1 || this.BM.y == -1) {
            this.BM = input;

        }

        else if (this.BR.x == -1 || this.BR.y == -1) {
            this.BR = input;

        }

        else { // done, go calculate
           calculateFocalLength();

        }
    }



}
