package Panda.sensors;

import java.io.*;
import lcm.lcm.*;
import lcmtypes.*;



public class PIMUSubscriber implements LCMSubscriber
{
    LCM lcm;
   	pimu_t pimu;
	pimu_t prev_msg;

	double[] pimu_derivs;


    public PIMUSubscriber() throws IOException
    {
        this.lcm = new LCM();
        this.lcm.subscribe("10_PIMU", this);
		pimu_derivs = new double[2];
    }

    public void messageReceived(LCM lcm, String channel, LCMDataInputStream ins)
    {
		try {
			if (channel.equals ("10_PIMU")) {
			
				pimu_t msg = new pimu_t();
				System.out.println(" timestamp = " + msg.utime);
				System.out.println(" timestamp = " + msg.utime_pimu);

				for (int i = 0; i < 8; i++) {
					System.out.println(" integrator = [ " + msg.integrator[i] + " ]");
				}
				for (int i = 0; i < 3; i++) {
					System.out.println(" accel = [ " + msg.accel[i] + " ]");
				}
				for (int i = 0; i < 3; i++) {
					System.out.println(" mag = [ " + msg.mag[i] + " ]");
				}
				for (int i = 0; i < 2; i++) {
					System.out.println(" alttemp = [ " + msg.alttemp[i] + " ]");
				}

				System.out.println (msg.integrator[4] + " " + msg.integrator[5]);

				double diff1 = (msg.integrator[4] - prev_msg.integrator[4]) / 
							(msg.utime_pimu - prev_msg.utime_pimu);
				double diff2 = (msg.integrator[5] - prev_msg.integrator[5]) / 
							(msg.utime_pimu - prev_msg.utime_pimu);

				System.out.println ("derivative " + diff1 + " " + diff2);
				

				prev_msg = msg;
				this.pimu = msg;
				this.pimu_derivs[0] = diff1;
				this.pimu_derivs[1] = diff2;
			}
		} catch (Exception e) {
			System.out.println (e);
		}
	}

	public pimu_t getPIMU () {
		return this.pimu;

	}


	public double[] getPIMUDerivs () {
		return this.pimu_derivs;

	}


}
