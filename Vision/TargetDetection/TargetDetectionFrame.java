package Vision.TargetDetection;
import javax.swing.*;
import java.awt.*;


import april.util.JImage;
import april.util.ParameterGUI;

public class TargetDetectionFrame extends JFrame {

    // args
    private JButton         chooseCameraSourceButton;
    private JButton         chooseImageButton;
    private JImage          centerImage;
    protected ParameterGUI  pg;
    
    
    // CONSTRUCTOR METHOD
    public TargetDetectionFrame() {
        super("Green Target Detection");
        this.setLayout(new BorderLayout());
        
        // add center image from JCam
        centerImage = new JImage();
        this.add(centerImage, BorderLayout.CENTER);

	// add parameterGUI
	pg = new ParameterGUI();
	pg.addDoubleSlider("HueLower", "Hue Lower Threshold", 0, 0.5, 0);
	pg.addDoubleSlider("HueUpper", "Hue Upper Threshold", 0.5, 1, 0.5);
	pg.addDoubleSlider("Saturation", "Saturation Threshold", 0, 1, 0);
	pg.addDoubleSlider("Value", "Value Threshold", 0, 1, 0);
	this.getContentPane().add(pg, BorderLayout.SOUTH);
        
        // add camera source button
        // add image source button
        chooseCameraSourceButton = new JButton("Camera Source");
        chooseImageButton = new JButton("Choose Image");
        
        // build GUI
        buildGUI();
        
    }
    
    // BUILD GUI
    public void buildGUI() {
        // build panel
        JPanel northPanel = new JPanel();
        northPanel.setLayout(new FlowLayout());
        this.add(northPanel, BorderLayout.NORTH);
        northPanel.add(chooseCameraSourceButton);
        northPanel.add(chooseImageButton);
        
    }
    
    // ACCESS FUNCTIONS
    public JButton getChooseCameraSourceButton() {
        return chooseCameraSourceButton;
    }
    
    
    public JImage getCenterImage() {
        return centerImage;
    }
     
    public synchronized JButton getChooseImageButton() {
        return chooseImageButton;
    }

    public ParameterGUI getParameterGUI() {
	return pg;
    }
    

}
