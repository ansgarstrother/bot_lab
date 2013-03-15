package Vision.LineDetection;

import javax.swing.*;
import java.util.ArrayList;
import java.lang.*;
import april.jmat.*;


public class LineDetectionMain {

	private final static double f = 640.1483;
	private final static double c_x = 676.0408;
	private final static double c_y = 480.3221;
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
