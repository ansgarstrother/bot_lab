package Rover;

import Mapping.*;
import lcmtypes.*;

import lcm.lcm.*;



public class RoverMapController implements Runnable {

	// const
	final protected RoverControllerDelegate delegate;
	protected MapSubscriber ms;
	protected RoverSubscriber rs;
	protected RoverPositioning roverPositioning;

	// args
	protected boolean isExecuting;
	protected volatile map_t prev_msg;
	protected volatile map_t new_msg;
	protected volatile pos_t prev_pos;
	protected volatile pos_t new_pos;
	protected volatile double[] new_pos_array;
	protected volatile double[] prev_pos_array;
	
	protected MapMgr map;
	protected PathPlan path;

	protected volatile boolean finished;


	// CONSTRUCTOR METHOD
	public RoverMapController(RoverControllerDelegate delegate) {
		this.delegate = delegate;
		try { this.ms = new MapSubscriber(); this.rs = new RoverSubscriber();}
		catch (Exception e) { System.err.println("Error initializing Subscriber: " + e.getMessage()); }

		this.roverPositioning = new RoverPositioning();

		this.isExecuting = false;
		this.prev_msg = new map_t();
		this.new_msg = new map_t();
		this.prev_pos = new pos_t();
		this.new_pos = new pos_t();
		this.new_pos_array = new double[3];
		this.prev_pos_array = new double[3];

		this.map = new MapMgr();
		this.path = new PathPlan();

		this.finished = false;

	}

	// CLASS METHODS
	// NULL


	// EXECUTION METHOD
	public void execute() {
		synchronized (this) {
			if (this.isExecuting) return;
			this.isExecuting = true;

		}

		while (!finished) {
			// retrieve message from map type
			try {
				this.prev_msg = new_msg;
				this.new_msg = ms.getMapMessage();
				this.prev_pos = new_pos;
				this.new_pos = rs.getPose();

				if (prev_pos != new_pos) {
					// retrieve position
					this.new_pos_array = roverPositioning.getNewPosition(new_pos);
					this.prev_pos_array = roverPositioning.getPrevPosition();
				}

				if (prev_msg != new_msg && prev_pos != new_pos) {
					// new barriers and/or triangles present
                	update();
				}
			} catch (Exception e) {
				System.out.println("Error retrieving map message: " + e.getMessage());
			}
		}
		// finished
        synchronized (this) {
            this.isExecuting = false;
        }
	}

	public void update() {
		// retrieve msg data
		int numTriangles = new_msg.numTriangles;
		int numBarriers = new_msg.numBarriers;
		double[][] barriers = new_msg.barriers;	// [ init_x, init_y, fin_x, fin_y ]
		double[][] triangles = new_msg.triangles; // [ center_x, center_y ]

        // set barriers, update known positions
		// set triangles, update known positions
		// retrieves current bot pos, segments of barriers, and points for triangle detection
        map.updateMap (new_pos_array, barriers, triangles);
	
		// plan path to send
		// new_pos_array = [x y t]
		path.advancedPlan(map.getMap(), (int)new_pos_array[0], (int)new_pos_array[1], new_pos_array[2]);

		// retrieve new path values
		double new_angle = path.getPathAngle();
		double new_dist = path.getPathDistance();
		finished = path.getFinishedTest();

		// publish commands to robot
		try {
			LCM new_plan_lcm = new LCM("udpm://239.255.76.10:7667?ttl=1");
			plan_t new_plan_msg = new plan_t();
    		new_plan_msg.timestamp = System.nanoTime();
			new_plan_msg.finished = finished;
			new_plan_msg.delta_x = (float)new_dist;
			new_plan_msg.theta = new_angle;
			new_plan_lcm.publish("10_PLAN", new_plan_msg);
			System.out.println("sending new plan:");
			System.out.println("finished: " + new_plan_msg.finished + " delta_x: " +  new_plan_msg.delta_x + " theta: " + new_plan_msg.theta);
		} catch (Exception e) {
			System.err.println("Error publishing next plan message: " + e.getMessage());
		}
		
	}



	// Runnable Method
	@Override
	public void run() {
		System.out.println("Mapping has been engaged!");
		execute();
	}


}
