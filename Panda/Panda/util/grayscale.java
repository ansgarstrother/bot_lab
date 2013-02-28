package Panda.util;

import java.awt.image.BufferedImage;
import java.awt.Color;
import java.awt.Graphics;

public class grayscale {
	// args
	BufferedImage in;
	BufferedImage gray;


	public grayscale(BufferedImage im) {
		in = im;
		gray = new BufferedImage(in.getWidth(), in.getHeight(), BufferedImage.TYPE_BYTE_GRAY);
		
		Graphics g = gray.getGraphics();
		g.drawImage(in, 0, 0, null);
		g.dispose();
	}

	// PUBLIC METHOD
	public BufferedImage getGrayScale() {
		return gray;
	}

}
