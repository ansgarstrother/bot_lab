package Rover;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.GraphicsConfiguration;
import java.awt.HeadlessException;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;

import april.util.ParameterGUI;
import april.vis.VisCanvas;
import april.vis.VisLayer;
import april.vis.VisWorld;

public class RoverFrame extends JFrame {
	
	protected VisWorld vw;
	protected VisLayer vl;
	protected VisCanvas vc;
	protected ParameterGUI pg;
	protected JButton executeButton;
	protected JButton resetViewButton;

	public RoverFrame() {
		super("Rover: Speed Racer");
		
		vw = new VisWorld();
		vl  = new VisLayer(vw);
		vc = new VisCanvas(vl);
		pg = new ParameterGUI();
		
		this.getContentPane().setLayout(new BorderLayout());
		this.getContentPane().add(vc, BorderLayout.CENTER);
		this.getContentPane().add(pg, BorderLayout.SOUTH);
		
		// TOP PANEL GUI
		JPanel topPanel = new JPanel();
		topPanel.setLayout(new FlowLayout());
		this.getContentPane().add(topPanel, BorderLayout.NORTH);
		executeButton = new JButton("Execute");
		resetViewButton = new JButton("Reset View");
		topPanel.add(executeButton);
		topPanel.add(resetViewButton);
	}
	
	
	
}
