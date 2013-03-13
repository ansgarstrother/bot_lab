package Panda.util;

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
			for (int j = 0; j < (in.getHeight()/2 + 100); j++) {
				int x = i;
				int y = in.getHeight() - 1 - j;
				int r = in.getRGB(x,y) & 0xff;
				if (r > thresh) {
					//int newPixel = 16777215;	// 255 255 255
					//edited.setRGB(i,j, newPixel);
				}
				else {
					int newPixel = 0;
					binarized.setRGB(x,y,newPixel);
					binarized.setRGB(x,y-1,newPixel);	
					binarized.setRGB(x,y-2,newPixel);
					binarized.setRGB(x,y-3,newPixel);
					binarized.setRGB(x,y-4,newPixel);
					binarized.setRGB(x,y-5,newPixel);	
					binarized.setRGB(x,y-6,newPixel);
					break;			
				}
				
			}
		}
	}

	// PUBLIC METHOD
	public BufferedImage getBinarizedImage() {
		return binarized;
	}

}
