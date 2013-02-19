package orc.util;

import java.io.*;
import java.nio.*;

/** A simple driver for Linux joysticks. **/
public class GamePad
{
    String devicePath;
    int    axes[] = new int[16];
    int    buttons[] = new int[16];

    ///////////////////////////////////////////////////////
    public GamePad()
    {
        String paths[] = { "/dev/js0",
                           "/dev/input/js0"
        };

        for (int i = 0; i < paths.length; i++) {
            String path = paths[i];
            File f = new File(path);
            if (f.exists()) {
                this.devicePath = path;
                break;
            }
        }

        if (devicePath == null) {
            System.out.println("Couldn't find a joystick.");
            System.exit(-1);
        }

        new ReaderThread().start();
    }

    public GamePad(String path)
    {
        this.devicePath = path;
        new ReaderThread().start();
    }

    // returns [-1, 1]
    public double getAxis(int axis)
    {
        if (axis >= axes.length)
            return 0;

        return axes[axis] / 32767.0;
    }

    public boolean getButton(int button)
    {
        if (button >= buttons.length)
            return false;

        return buttons[button] > 0;
    }

    /** Returns once any button has been pressed, returning the button
     * id. This is useful with cordless game pads as a way of ensuring
     * that there's actually a device connected.
     **/
    public int waitForAnyButtonPress()
    {
        boolean buttonState[] = new boolean[16];
        for (int i = 0; i < buttonState.length; i++)
            buttonState[i] = getButton(i);

        while (true) {

            for (int i = 0; i < buttonState.length; i++)
                if (getButton(i) != buttonState[i])
                    return i;

            try {
                Thread.sleep(10);
            } catch (InterruptedException ex) {
            }
        }
    }

    class ReaderThread extends Thread
    {
        ReaderThread()
        {
            setDaemon(true);
        }

        public void run()
        {
            try {
                runEx();
            } catch (IOException ex) {
                System.out.println("GamePad ex: "+ex);
            }
        }

        public void runEx() throws IOException
        {
            FileInputStream fins = new  FileInputStream(new File(devicePath));
            byte buf[] = new byte[8];

            while (true) {
                fins.read(buf);

                int mstime = (buf[0]&0xff) | ((buf[1]&0xff)<<8) | ((buf[2]&0xff)<<16) | ((buf[3]&0xff)<<24);
                int value  = (buf[4]&0xff) | ((buf[5]&0xff)<<8);

                if ((value & 0x8000)>0) // sign extend
                    value |= 0xffff0000;

                int type   = buf[6]&0xff;
                int number = buf[7]&0xff;

                if ((type&0x3)== 1) {
                    if (number < buttons.length)
                        buttons[number] = value;
                    else
                        System.out.println("GamePad: "+number+" buttons!");
                }

                if ((type&0x3) == 2) {
                    if (number < axes.length)
                        axes[number] = value;
                    else
                        System.out.println("GamePad: "+number+" axes!");
                }
            }
        }
    }

    ///////////////////////////////////////////////////////
    public static void main(String args[])
    {
        GamePad gp = new GamePad();

        int nAxes = 6;
        int nButtons = 16;

        for (int i = 0; i < nAxes; i++) {
            System.out.printf("%10s ", "Axis "+i);
        }

        for (int i = 0; i < nButtons; i++) {
            int v = i & 0xf;
            System.out.printf("%c", v>=10 ? 'a'+(v-10) : '0'+v);
        }
        System.out.printf("\n");

        while (true) {
            String s = "";
            for (int i = 0; i < nButtons; i++)
                s=s+(gp.getButton(i) ? 1 : 0);

            System.out.printf("\r");
            for (int i = 0; i < nAxes; i++)
                System.out.printf("%10f ", gp.getAxis(i));
            System.out.printf("%s", s);

            try {
                Thread.sleep(10);
            } catch (InterruptedException ex) {
            }
        }
    }
}
