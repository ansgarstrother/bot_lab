package Vision.util;

import java.awt.image.BufferedImage;

public class binarize {
	// args
	BufferedImage in;
	BufferedImage binarized;
	double thresh;

	int r, newPixel;

	public binarize(BufferedImage im, double thresh) {
		in = im;
		this.thresh = thresh;
		binarized = in;
		binarizeImage();
	}

	protected void binarizeImage() {
		for (int i = 0; i < in.getWidth(); i++) {
			for (int j = (in.getHeight()/2 - 100); j < in.getHeight(); j++) {
				int r = in.getRGB(i,j) & 0xff;
				if (r > thresh) {
					//int newPixel = 16777215;	// 255 255 255
					//edited.setRGB(i,j, newPixel);
				}
				else {
					int newPixel = 0;
					binarized.setRGB(i,j,newPixel);
				}
				
			}
		}
	}

	// PUBLIC METHOD
	public BufferedImage getBinarizedImage() {
		return binarized;
	}

}
