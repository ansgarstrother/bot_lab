package Vision.LineDetection;

import javax.swing.*;


public class LineDetectionMain {

    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                // create app frame
                LineDetectionFrame frame = new LineDetectionFrame();
                // build controller
                LineDetectionController appController = new LineDetectionController(frame);

            }
        });
    }
}