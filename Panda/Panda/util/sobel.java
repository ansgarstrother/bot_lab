package Panda.util;


import java.awt.event.*;   
import java.awt.image.BufferedImage;   
import java.io.*;   
   
import javax.imageio.ImageIO;   
import javax.swing.ImageIcon;   
import javax.swing.JFrame;   
import javax.swing.JLabel;


public class sobel {
	
	// args
	int width;
	int height;
	int[] pixels;
	int[][] output;
	double Gx[][], Gy[][], G[][];
	

	public sobel(BufferedImage im) {
		width = im.getWidth();
		height = im.getHeight();
		pixels = new int[width * height];
		output = new int[width][height];
		Gx = new double[width][height];
		Gy = new double[width][height];
		G = new double[width][height];
		im.getRaster().getPixels(0,0,width,height,pixels);
		int ct = 0;
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				output[i][j] = pixels[ct];
				ct++;
			}
		}

	}

	// OPERATION METHOD
	public BufferedImage detectEdges() {
		// Build Operator and Convolve
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				// image boundary
				if (i == 0 || i == width - 1 || j == 0 || j == height - 1) {
					Gx[i][j] = Gy[i][j] = G[i][j] = 0;
				}
				else {
					Gx[i][j] = output[i+1][j-1] + 2*output[i+1][j] + output[i+1][j+1] -   
          				output[i-1][j-1] - 2*output[i-1][j] - output[i-1][j+1];   
          				Gy[i][j] = output[i-1][j+1] + 2*output[i][j+1] + output[i+1][j+1] -   
          				output[i-1][j-1] - 2*output[i][j-1] - output[i+1][j-1];   
          				G[i][j]  = Math.abs(Gx[i][j]) + Math.abs(Gy[i][j]);
				}
			}
		}
		int count = 0;
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				pixels[count] = (int) G[i][j];
				count++;
			}
		}
		
		// Build buffered image
		BufferedImage out = new BufferedImage(width, height, BufferedImage.TYPE_BYTE_GRAY);
		out.getRaster().setPixels(0,0,width,height,pixels);
		return out;
	}
}
		
