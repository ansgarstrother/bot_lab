package Vision.LineDetection;

import java.io.*;

import javax.imageio.ImageIO;
import javax.swing.*;
import javax.swing.filechooser.FileNameExtensionFilter;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseAdapter;
import java.awt.image.BufferedImage;
import java.awt.Graphics2D;
import java.awt.Point;
import java.util.ArrayList;
import java.util.List;
import java.io.IOException;
import java.awt.geom.*;

import april.jcam.ImageSource;
import april.jcam.ImageConvert;
import april.jcam.ImageSourceFormat;
import april.jcam.ImageSourceFile;



public class LineDetectionController {

    // args
    private ImageSource		    selectedImageSource;
    private LineDetectionFrame      frame;
    private String		    selectedCameraURL;
    private Thread		    imageThread;
    private BufferedImage 	    selectedImage;
    private LineDetectionDetector   detector;

    final static double binaryThresh = 155;
    
    
    // CONSTRUCTOR
    public LineDetectionController(LineDetectionFrame frame) {
        this.frame = frame;
        frame.setSize(1024, 768);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setVisible(true);

        // add action event listeners
        frame.getChooseCameraSourceButton().addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				LineDetectionController.this.chooseCameraSourceAction();
			}
		});
         
        
        frame.getChooseImageButton().addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				JFileChooser chooser = new JFileChooser();
			    FileNameExtensionFilter filter = new FileNameExtensionFilter(
                                                                             "Images", "jpg", "gif", "png");
			    chooser.setFileFilter(filter);
			    int returnVal = chooser.showOpenDialog(LineDetectionController.this.frame);
			    if(returnVal == JFileChooser.APPROVE_OPTION) {
			    	selectedImage = imageFromFile(chooser.getSelectedFile());
			    	selectedImageSource = null;
			    	if ( selectedImage != null ) {
			    		startImage();
			    	}
			    }
			}
		});
        
        
        // toggle line of best fit visibility by clicking image
        frame.getCenterImage().addMouseListener(new MouseAdapter() {
			public void mouseClicked(MouseEvent me) {
				LineDetectionController.this.didClickMouse(me);
			}
		});
        
        
    }
    
    // PROTECTED CLASS METHODS
    protected static BufferedImage imageFromFile(File file) {
		BufferedImage in;
		try {
			in = ImageIO.read(file);
		} catch (IOException e) {
			e.printStackTrace();
			System.err.println("Error Reading from File");
			return null;
		}
		BufferedImage newImage = new BufferedImage(in.getWidth(), in.getHeight(), BufferedImage.TYPE_INT_ARGB);
		Graphics2D g = newImage.createGraphics();
		g.drawImage(in, 0, 0, null);
		g.dispose();
		return newImage;
	}
    
    protected void didClickMouse(MouseEvent me) {
        // toggle visibility of best fit lines

	}
    
	protected void chooseCameraSourceAction() {
		// retrieve camera URLS
		List<String> URLS = ImageSource.getCameraURLs();
        
		// test for no cameras availablImageSourceFormate
		if (URLS.size() == 0) {
			JOptionPane.showMessageDialog(
                                          this.getFrame(),
                                          "There are no camera urls available",
                                          "Error encountered",
                                          JOptionPane.OK_OPTION
                                          );
			return;
		}
        
		final String initial = (this.selectedCameraURL == null) ?
        URLS.get(0) : this.selectedCameraURL;
        
		final String option = (String)JOptionPane.showInputDialog(
                                                                  this.getFrame(),
                                                                  "Select a camera source from the available URLs below:",
                                                                  "Select Source",
                                                                  JOptionPane.PLAIN_MESSAGE,
                                                                  null,
                                                                  URLS.toArray(),
                                                                  initial
                                                                  );
        
		if ( option != null ) {
			// the selected URL
			this.selectedCameraURL = option;
			this.startCamera();
		}
	}
    
    protected void startImage() {
		if ( this.imageThread != null ) {
			System.err.println("Warning, camera already running");
			return;
		}
        
		this.imageThread = new Thread(new Runnable() {
			@Override
			public void run() {
				SwingUtilities.invokeLater(new Runnable() {
					@Override
					public void run() {
						BufferedImage out = LineDetectionController.this.processImage(selectedImage);
                            			LineDetectionController.this.getFrame().getCenterImage().setImage(out);
					}
				});
			}
		});
		this.imageThread.start();
	}
    
    protected void startCamera() {
		
		if ( this.imageThread != null ) {
			System.err.println("Warning, camera already running");
			return;
		}
        
		try {
			this.selectedImageSource = ImageSource.make(this.selectedCameraURL);
		}
		catch(IOException e) {
			// do nothing
			System.err.println(e);
			this.selectedImageSource = null;
			return;
		}
        
		// BUILD NEW THREAD
		this.selectedImageSource.start();
		this.imageThread = new Thread(new Runnable() {
			@Override
            public void run() {
                ImageSourceFormat fmt = LineDetectionController.this.selectedImageSource.getCurrentFormat();
                while (true) {
                    // get buffer with image data from next frame
                    byte buf[] = LineDetectionController.this.selectedImageSource.getFrame().data;
                    
                    // if next frame is not ready, buffer will be null
                    // continue and keep trying
                    if (buf == null) {
                        System.err.println("Buffer is null");
                        continue;
                    }
                    
                    // created buffered image from image data
                    final BufferedImage im = ImageConvert.convertToImage(
                                                                         fmt.format,
                                                                         fmt.width,
                                                                         fmt.height,
                                                                         buf
                                                                         );
                    
                    // set image on main window
                    SwingUtilities.invokeLater(new Runnable() {
                        @Override
                        public void run() {
                            BufferedImage out = LineDetectionController.this.processImage(im);
                            LineDetectionController.this.getFrame().getCenterImage().setImage(out);
                        }
                    });
                }
            }
        });
        this.imageThread.start();
    }

    // Image Processing
    protected BufferedImage processImage(BufferedImage image) {
	    	detector = new LineDetectionDetector(image, binaryThresh);
		return detector.getProcessedImage();
    }

    
    // PUBLIC CLASS METHODS
    public LineDetectionFrame getFrame() {
        return frame;
    }

    
}
