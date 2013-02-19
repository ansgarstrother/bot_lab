package orc;

import java.io.*;

public class AX12Servo
{
    Orc orc;
    int id;

    public boolean verbose = false;

    static final int INST_PING         = 0x01;
    static final int INST_READ_DATA    = 0x02;
    static final int INST_WRITE_DATA   = 0x03;
    static final int INST_REG_WRITE    = 0x04;
    static final int INST_ACTION       = 0x05;
    static final int INST_RESET_DATA   = 0x06;
    static final int INST_SYNC_WRITE   = 0x83;

    static final int ADDR_ID           = 0x03;

    public static final int ERROR_INSTRUCTION = (1<<6);
    public static final int ERROR_OVERLOAD    = (1<<5);
    public static final int ERROR_CHECKSUM    = (1<<4);
    public static final int ERROR_RANGE       = (1<<3);
    public static final int ERROR_OVERHEAT    = (1<<2);
    public static final int ERROR_ANGLE_LIMIT = (1<<1);
    public static final int ERROR_VOLTAGE     = (1<<0);

    public static final int BROADCAST_ADDRESS = 0xfe;

    double pos0 = 0, val0 = 0, pos1 = 300, val1 = 300;

    public AX12Servo(Orc orc, int id)
    {
        this.orc = orc;
        this.id = id;
    }

    public AX12Servo(Orc orc, int id, double pos0, double val0, double pos1, double val1)
    {
        this(orc, id);
        remap(pos0, val0, pos1, val1);
    }

    /** Remap the angle scale (and possibly enforce clamping.)
     * @param pos0 Actual servo position [0,300] corresponding to an input of val0.
     * @param val0 The value which will be mapped to the servo position pos0.
     * @param pos1 Actual servo position [0, 300] corresponding to an input of val1.
     * @param val1 The value which will be mapped to the servo position pos1.
     *
     * Suppose that you wish to define a joint with a range of 30
     * degrees [0, 30] that will swing between [170, 200] in the
     * servo's native coordinates. You would call (170, 0, 200, 30).
     **/
    public void remap(double pos0, double val0, double pos1, double val1)
    {
        this.pos0 = pos0;
        this.val0 = val0;
        this.pos1 = pos1;
        this.val1 = val1;
    }

    /** Convert user-specified position into servo coordinates. **/
    double map(double val)
    {
        double x = (val - val0) / (val1 - val0);
        return pos0 + x*(pos1 - pos0);
    }

    double unmap(double pos)
    {
        double x = (pos - pos0) / (pos1 - pos0);
        return val0 + x*(val1 - val0);
    }

    static byte[] makeCommand(int id, int instruction,  byte parameters[])
    {
        int parameterlen = (parameters == null) ? 0 : parameters.length;
        byte cmd[] = new byte[6 + parameterlen];

        cmd[0] = (byte) 255;                       // magic
        cmd[1] = (byte) 255;                       // magic
        cmd[2] = (byte) id;                        // servo id
        cmd[3] = (byte) (parameterlen + 2);   // length
        cmd[4] = (byte) instruction;

        if (parameters != null) {
            for (int i = 0; i < parameters.length; i++)
                cmd[5+i] = parameters[i];
        }

        if (true) {
            // compute checksum
            int checksum = 0;
            for (int i = 2; i < cmd.length -1; i++)
                checksum += (cmd[i] & 0xff);

            cmd[5 + parameterlen] = (byte) (checksum^0xff);
        }

        return cmd;
    }

    /**  This method changes the servo ID from oldID to newID.  It is  OK
     *  to use this method on bus.
     @param oldID [0x00, 0xFD]
     @param newID [0x00, 0xFD]
    **/
    public void changeServoID(int oldID, int newID)
    {
        //0XFF 0XFF 0XFE 0X04   0X03 0X03 0X01  0XF6
        //          ID   LENGTH INST PARAMETERS CHECKSUM
        OrcResponse resp = orc.doCommand(0x01007a00, makeCommand((byte)(oldID&0xff),
                                                                 INST_WRITE_DATA,
                                                                 new byte[] {(byte)ADDR_ID, (byte) (newID&0xff)}));

        if (verbose)
            resp.print();
    }

    public boolean ping()
    {
        OrcResponse resp = orc.doCommand(0x01007a00, makeCommand(id,
                                                                 INST_PING, null));
        if (verbose)
            resp.print();

        byte tmp[] = new byte[] { 0x06, (byte) 0x0ff, (byte) 0x0ff, (byte) id, 0x02, 0x00 };

        for (int i = 0; i < tmp.length; i++)
            if (tmp[i] != resp.responseBuffer[resp.responseBufferOffset +i])
                return false;

        return true;
    }


    /** @param deg [0, 300]
        @param speedfrac [0, 1]
    **/
    public void setGoalDegrees(double deg, double speedfrac, double torquefrac)
    {
        deg = map(deg);
        int posv = (int) (0x3ff*deg/300.0);
        int speedv = (int) (0x3ff*speedfrac);
        int torquev = (int) (0x3ff*torquefrac);

        OrcResponse resp = orc.doCommand(0x01007a00, makeCommand(id,
                                                                 INST_WRITE_DATA,
                                                                 new byte[] {0x1e,
                                                                             (byte) (posv&0xff), (byte) (posv>>8),
                                                                             (byte) (speedv&0xff), (byte) (speedv>>8),
                                                                             (byte) (torquev&0xff), (byte) (torquev>>8)}));
        if (verbose)
            resp.print();
    }

    public byte[] getDebugData()
    {
        // read 8 bytes beginning from register 0x24
        OrcResponse resp = orc.doCommand(0x01007d00, new byte[0]);

        try {
            DataInputStream ins = resp.ins;

            int len = ins.available();
            byte d[] = new byte[len];
            ins.read(d);

            return d;
        } catch (IOException ex) {
            System.out.println("ex: "+ex);
            return null;
        }
    }

    public AX12Status getStatus()
    {
        AX12Status status = new AX12Status();

        while (true) {
            // read 8 bytes beginning from register 0x24
            OrcResponse resp = orc.doCommand(0x01007a00, makeCommand(id,
                                                                     INST_READ_DATA, new byte[] { 0x24, 8 }));

            if (verbose)
                resp.print();

            DataInputStream ins = resp.ins;

            try {
                int len = ins.read();
                if (len != 0x0e)
                    continue;

                int ff = ins.read();
                if (ff != 0xff)
                    continue;
                ff = ins.read();
                if (ff != 0xff)
                    continue;

                int id = ins.read();
                if (id != this.id)
                    continue;

                int paramlen = ins.read();
                if (paramlen != 0x0a)
                    continue;

                status.error_flags = ins.read();

                status.positionDegrees = unmap(((ins.read()&0xff) + ((ins.read()&0x3f)<<8)) * 300.0 / 1024.0);

                // caution: speed is unsigned
                // scale to [0,1]
                int ispeed = ((ins.read()&0xff) + ((ins.read()&0xff)<<8));
                status.speed = (ispeed & 0x3ff) / 1024.0;
                if ((ispeed & (1<<10)) != 0)
                    status.speed *= -1;

                int iload = (ins.read()&0xff) + ((ins.read()&0xff)<<8);
                status.load = (iload & 0x3ff) / 1024.0;
                if ((iload & (1<<10)) != 0)
                    status.load *= -1;

                status.utimeOrc = resp.utimeOrc;
                status.utimeHost = resp.utimeHost;
                status.voltage = (ins.read()&0xff)/10.0;
                status.temperature = (ins.read()&0xff);

            } catch (IOException ex) {
                continue;
            }

            return status;
        }
    }

    public int getId()
    {
        return id;
    }

    public static void main(String args[])
    {
        Orc orc = Orc.makeOrc();

        AX12Servo servo = new AX12Servo(orc, 1);

        byte d[] = servo.getDebugData();
        for (int i = 0; i < d.length; i++) {
            System.out.printf("%3d %02x\n", i, d[i] & 0xff);
        }

        for (int iter = 0; true; iter++) {
            System.out.printf("%d\n", iter);

            servo.ping();

            AX12Status st = servo.getStatus();
            st.print();

            System.out.println("**");
            servo.setGoalDegrees(150, 0.1, 0.1);
            try {
                Thread.sleep(500);
            } catch (InterruptedException ex) {
            }

            servo.setGoalDegrees(160, 0.1, 0.1);
            try {
                Thread.sleep(500);
            } catch (InterruptedException ex) {
            }
        }
    }
}
