package Vision.util;

import java.awt.image.BufferedImage;
import java.awt.Color;

public class binarize {
	// args
	BufferedImage in;
	BufferedImage binarized;
	double thresh;

	int r, newPixel;

	public binarize(BufferedImage im, double thresh) {
		in = im;
		this.thresh = thresh;
		//binarized = in;
		binarized = new BufferedImage(im.getWidth(), im.getHeight(), im.getType());
		binarizeImage();
	}

	protected void binarizeImage() {
		for (int i = 0; i < in.getWidth(); i++) {
			for (int j = 0; j < in.getHeight(); j++) {
				r = new Color(in.getRGB(i,j)).getRed();
				int alpha = new Color(in.getRGB(i,j)).getAlpha();
				if (r > thresh) {
					newPixel = 16777215;	// 255 255 255
				}
				else {
					newPixel = 0;	// 0 0 0
					//System.out.println("hi");
				}

				binarized.setRGB(i,j, newPixel);
			}
		}
	}

	// PUBLIC METHOD
	public BufferedImage getBinarizedImage() {
		return binarized;
	}

}