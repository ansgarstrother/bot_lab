package Vision.LineDetection;

import javax.swing.*;


public class LineDetectionMain {

	private final static double f = -269.30751168;
	private final static double c_x = 560.081029;
	private final static double c_y = 280.264223576;
    private static double[][] calibrationMatrix =
		{ 	{f, 0, c_x}, {0, f, c_y}, {0, 0, 1}	};

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
