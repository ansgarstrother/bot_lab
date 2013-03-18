package Mapping;


import java.io.*;
import lcm.lcm.*;
import java.lang.Math;
import lcmtypes.*;

public class MapSubscriber implements LCMSubscriber
{
    LCM lcm;

    // pos_t variables
    map_t msg;


    
    public MapSubscriber()
    throws IOException
    {
        this.lcm = new LCM("udpm://239.255.76.10:7667?ttl=1");
        this.lcm.subscribe("10_MAP", this);
		this.msg = new map_t();
    }
    
    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins)
    {   
	System.out.println("Received message on channel " + channel);

        try {
            if (channel.equals("10_MAP")) {
                this.msg = new map_t(ins);
            }
            
        } catch (IOException ex) {
            System.out.println("Exception: " + ex);
        }
    }

    public map_t getMapMessage() {
    	return msg;
    }
}


