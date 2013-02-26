package Vision.calibration;

import javax.swing.*;
import javax.swing.SwingUtilities;


public class ProjectionMain {

    public static void main (final String[] args) {
        SwingUtilities.invokeLater (new Runnable() {
            public void run() {

                Projection projection = new Projection();
            }

        });
    }


}
