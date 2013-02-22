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

    private Point click1;
    private Point click2;
    private double focalLength;
    private double objectDistance;
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
        this.click1 = new Point (-1, -1);
        this.click2 = new Point (-1, -1);

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

                        // if both points have been set
                        if (Projection.this.click1.x != -1
                            && Projection.this.click1.y != -1
                            && Projection.this.click2.x != -1
                            && Projection.this.click2.y != -1) {

                            // go calculate shit
                            Projection.this.calculateFocalLength();

                        }
                    }
                }
        });
        Projection.this.projThread.start();


    }

    Projection.this.calculateFocalLength() {


    }


    public void mouseClickHandler (MouseEvent me) {

        Point input = me.getPoint();
        System.out.println ("Mouse clicked at " + input.x + " " + input.y);

        // if click one has not yet been set
        if (this.click1.x == -1 || this.click1.y == -1) {
            System.out.println ("set click1");
            click1 = input;
        }

        else if (this.click2.x == -1 || this.click2.y == -1) {
            System.out.println ("set click2");
            click2 = input;

        }


    }



}
