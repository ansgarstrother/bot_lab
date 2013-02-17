package Rover;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.GraphicsConfiguration;
import java.awt.HeadlessException;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JLabel;

import april.util.ParameterGUI;
import april.vis.*;

public class ellipseFrame extends JFrame {
	
	protected VisWorld vw;
	protected VisLayer vl;
	protected VisCanvas vc;
	protected ParameterGUI pg;
	
	protected JButton resetViewButton;

	public ellipseFrame() {
		super("Rover: Error Ellipse");
		
		vw = new VisWorld();
		vl  = new VisLayer(vw);
		vc = new VisCanvas(vl);
		pg = new ParameterGUI();
		
		this.getContentPane().setLayout(new BorderLayout());
		this.getContentPane().add(vc, BorderLayout.CENTER);
		this.getContentPane().add(pg, BorderLayout.SOUTH);	

		// ADD GRID
		VzGrid.addGrid(vw);

		// TOP PANEL GUI
		JPanel topPanel = new JPanel();
		topPanel.setLayout(new FlowLayout());
		this.getContentPane().add(topPanel, BorderLayout.NORTH);
		resetViewButton = new JButton("Reset View");
		topPanel.add(resetViewButton);
	}
	
	
	
}
