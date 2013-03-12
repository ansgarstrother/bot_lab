package Vision.LineDetection;

import javax.swing.*;
import java.util.ArrayList;
import java.lang.*;
import april.jmat.*;


public class LineDetectionMain {

	private final static double f = 422.51110623757427;
	private final static double c_x = 654.9986200773297;
	private final static double c_y = 949.9890978166349;
    private static double[][] calibrationMatrix =
		{ 	{f, 0, c_x}, {0, f, c_y}, {0, 0, 1}	};

	private ArrayList<Matrix> history;

    public static void main(String[] args) {

			this.history = new ArrayList<Matrix>();

        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                // create app frame
                LineDetectionFrame frame = new LineDetectionFrame();
                // build controller
                LineDetectionController appController = new LineDetectionController(frame, calibrationMatrix, history);

            }
        });
    }
}
