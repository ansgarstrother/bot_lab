package orc;

import java.io.*;

/** The state of the uOrcBoard, including sensor states. Most users won't need to access this directly. **/
public class OrcStatus
{
    public Orc orc; // the Orc from which this status was obtained

    public long utimeOrc;
    public long utimeHost;

    public int statusFlags;
    public int debugCharsWaiting;
    public int analogInput[] = new int[13];
    public int analogInputFiltered[] = new int[13];
    public int analogInputFilterAlpha[] = new int[13];
    public int simpleDigitalValues;
    public int simpleDigitalDirections;

    public boolean motorEnable[] = new boolean[3];
    public int motorPWMactual[] = new int[3];
    public int motorPWMgoal[] = new int[3];
    public int motorSlewRaw[] = new int[3]; // (PWM ticks per ms) * 256
    public double motorSlewSeconds[] = new double[3];

    public int qeiPosition[] = new int[2];
    public int qeiVelocity[] = new int[2];
    public int fastDigitalMode[] = new int[8];
    public int fastDigitalConfig[] = new int[8];

    public long gyroIntegrator[] = new long[3];
    public int  gyroIntegratorCount[] = new int[3];

    static final int readU16(DataInputStream ins) throws IOException
    {
        return ins.readShort() & 0xffff;
    }

    static final int readS16(DataInputStream ins) throws IOException
    {
        return ins.readShort();
    }

    static final int readU8(DataInputStream ins) throws IOException
    {
        return ins.readByte() & 0xff;
    }

    /** Is the estop switch activated? **/
    public boolean getEstopState()
    {
        return (statusFlags & 1) > 0 ? true : false;
    }

    public boolean getMotorWatchdogState()
    {
        return (statusFlags & 2) > 0 ? true : false;
    }

    public double getBatteryVoltage()
    {
        double voltage = analogInputFiltered[11] / 65535.0 * 3.0;
        double batvoltage = voltage * 10.1;
        return batvoltage;
    }

    /** assume that response type has already been consumed. Length is next. **/
    public OrcStatus(Orc orc, OrcResponse response) throws IOException
    {
        this.orc = orc;
        DataInputStream ins = response.ins;

        utimeOrc = response.utimeOrc;
        utimeHost = response.utimeHost;

        statusFlags = ins.readInt();
        debugCharsWaiting = readU16(ins);

        for (int i = 0; i < 13; i++) {
            analogInput[i] = readU16(ins);
            analogInputFiltered[i] = readU16(ins);
            analogInputFilterAlpha[i] = readU16(ins);
        }

        simpleDigitalValues = ins.readInt();
        simpleDigitalDirections = ins.readInt();

        for (int i = 0; i < 3; i++) {
            motorEnable[i] = readU8(ins) != 0;
            motorPWMactual[i] = readS16(ins);
            motorPWMgoal[i] = readS16(ins);
            motorSlewRaw[i] = readU16(ins);
            motorSlewSeconds[i] = 510.0 / motorSlewRaw[i] / 1000.0 * 128.0;
        }

        for (int i = 0; i < 2; i++) {
            qeiPosition[i] = ins.readInt();
            qeiVelocity[i] = ins.readInt();
        }

        for (int i = 0; i < 8; i++) {
            fastDigitalMode[i] = readU8(ins);
            fastDigitalConfig[i] = ins.readInt();
        }

        for (int i = 0; i < 3; i++) {
            gyroIntegrator[i] = ins.readLong();
            gyroIntegratorCount[i] = ins.readInt();
        }
    }
}
