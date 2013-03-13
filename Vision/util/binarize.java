package Vision.util;

import java.awt.image.BufferedImage;
import java.awt.Color;

public class binarize {

	// const
	private static final float MIN_BLUE_HUE = 0.5f;
	private static final float MAX_BLUE_HUE = 0.83333f;

	// args
	BufferedImage binarized;
	BufferedImage color;
	double thresh;

	int r, newPixel;

	public binarize(BufferedImage col, double thresh) {
		this.color = col;
		this.thresh = thresh;
		binarized = col;
		binarizeImage();
	}

	protected void binarizeImage() {
		for (int i = 0; i < color.getWidth(); i++) {
			for (int j = 0; j < (color.getHeight()/2); j++) {
				int x = i;
				int y = color.getHeight() - 1 - j;
				//int r = in.getRGB(x,y) & 0xff;
				int cur_pixel = color.getRGB(x,y);
				float[] hsv = Color.RGBtoHSB((cur_pixel >> 16) & 0xff, (cur_pixel >> 8) & 0xff, (cur_pixel) & 0xff, null);
				if (r > thresh) {
					//int newPixel = 16777215;	// 255 255 255
					//edited.setRGB(i,j, newPixel);
				}
				if (hsv[0] >= MIN_BLUE_HUE && hsv[0] <= MAX_BLUE_HUE) {
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
