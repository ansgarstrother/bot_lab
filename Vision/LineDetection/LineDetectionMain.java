package Vision.LineDetection;

import javax.swing.*;


public class LineDetectionMain {

	private final static double f = 431.91903976039714;
	private final static double c_x = 637.0650924397855;
	private final static double c_y = 875.0942250076687;
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
