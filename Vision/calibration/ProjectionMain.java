package Vision.calibration;

import javax.swing.*;
import javax.swing.SwingUtilities;


public class ProjectionMain {

    public static void main (final String[] args) {
        SwingUtilities.invokeLater (new Runnable() {
            public void run() {

                if (args.length != 2) {
                    System.out.println ("Specify object distance and height");
                    return;
                }
                Projection projection = new Projection(args);
            }

        });
    }


}
