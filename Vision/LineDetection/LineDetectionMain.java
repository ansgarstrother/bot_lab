package Vision.LineDetection;

import javax.swing.*;
import java.util.ArrayList;
import java.lang.*;
import april.jmat.*;


public class LineDetectionMain {

	private final static double f = 425.226845;
	private final static double c_x = 653.45258;
	private final static double c_y = 447.21663;
    private static double[] calibrationMatrix =
		{ 	f, c_x, c_y	};


    public static void main(String[] args) {

        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                // create app frame
                LineDetectionFrame frame = new LineDetectionFrame();
                // build controller
				PandaPositioning pp = new PandaPositioning();
                LineDetectionController appController = new LineDetectionController(frame, calibrationMatrix, pp);

            }
        });
    }
}
