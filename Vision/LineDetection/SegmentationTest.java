package Vision.LineDetection;

import java.awt.image.BufferedImage;
import java.io.*;
import java.util.ArrayList;



public class SegmentationTest {

    public static void main(String[] args) {
        int[] boundaryMap = new int[1296];
        try {
            FileInputStream fstream = new FileInputStream(args[0]);
            DataInputStream in = new DataInputStream(fstream);
            BufferedReader br = new BufferedReader(new InputStreamReader(in));
            String strLine;
            
            int i = 0;
            while ((strLine = br.readLine()) != null && i < 1296) {
                String[] digit = strLine.split(", ");
                boundaryMap[i] = Integer.parseInt(digit[1]);
                i++;
            }
            in.close();
            //LineDetectionSegmentation lds = new LineDetectionSegmentation(boundaryMap, );
        }
        catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
        }
        
    }
}
