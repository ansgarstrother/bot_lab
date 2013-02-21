package Vision.util;

import java.awt.image.BufferedImage;

public class colored_binarize {
	// args
	BufferedImage in;
	BufferedImage binarized;
	double thresh;
    double lowpass;
    
	int b, newPixel;
    
	public binarize(BufferedImage im, double thresh, double lwthresh) {
		in = im;
		this.thresh = thresh;   // blue threshold
        lowpass = lwthresh;     // red and green threshold (lower bounds)
		binarized = in;
		binarizeImage();
	}
    
	protected void binarizeImage() {
		for (int i = 0; i < in.getWidth(); i++) {
			for (int j = (in.getHeight()/2 - 100); j < in.getHeight(); j++) {
                int r = (in.getRGB(i,j) >> 16) & 0xff;
                int g = (in.getRGB(i,j) >> 8) & 0xff;
				int b = in.getRGB(i,j) & 0xff;
				if (b > thresh && g < lowpass && r < lowpass) {
					binarized.setRGB(i,j, 0xff0000ff); //blue
				}
				else {
					//int newPixel = 0;
					//binarized.setRGB(i,j,newPixel);
				}
				
			}
		}
	}
    
	// PUBLIC METHOD
	public BufferedImage getBinarizedImage() {
		return binarized;
	}
    
}