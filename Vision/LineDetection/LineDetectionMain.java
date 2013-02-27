package Vision.LineDetection;

import javax.swing.*;


public class LineDetectionMain {

    private static double[][] calibrationMatrix = { {139.63452, 0, 472.76047, 0},
                         {0, 139.63452,617.7583, 0},
                         {0, 0, 1, 0} };

    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                // create app frame
                LineDetectionFrame frame = new LineDetectionFrame();
                // build controller
                LineDetectionController appController = new LineDetectionController(frame, calibrationMatrix);

            }
        });
    }
}
