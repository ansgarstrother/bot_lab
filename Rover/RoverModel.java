package Rover;


import java.awt.Color;
import java.util.*;

import april.vis.*;
import april.jmat.*;

public class RoverModel
{

	public RoverModel()
	{
	}

	// creates a VisChain that wraps a point (sphere) whose
	// origin is at the vertical (z-axis) bottom, horizontal (x,y axes) center
	protected static VisChain createPoint(
		final double xCoord,
		final double yCoord,
		final double zCoord)
	{
		VisChain chain = new VisChain();
		VzSphere point = new VzSphere(0.5, new VzMesh.Style(Color.yellow));
		chain.add(LinAlg.translate(xCoord,yCoord, zCoord / 2), point);
		return chain;
	}

	// armAngles MUST be of length 4
	public VisChain getRoverChain()
	{
		VisChain rover = createPoint(0.0,0.0,0.0);
		return rover;
	}

}
