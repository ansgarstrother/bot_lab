package Vision.TargetDetection;

import javax.swing.*;


public class TargetDetectionMain {

    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                // create app frame
                TargetDetectionFrame frame = new TargetDetectionFrame();
                // build controller
                TargetDetectionController appController = new TargetDetectionController(frame);

            }
        });
    }
}
