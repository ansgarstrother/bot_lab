package orc.spy;

import java.awt.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;

import orc.*;

/** A GUI for visualizing and interacting with the uOrc. **/
public class Spy
{
    JFrame jf;

    JDesktopPane jdp = new JDesktopPane();

    TextPanelWidget basicDisplay, analogDisplay, qeiDisplay, digDisplay;
    MotorPanel motorPanels[];
    ServoPanel servoPanels[];

    Orc orc;

    public static void main(String args[])
    {
        new Spy(args);
    }

    public Spy(String args[])
    {
        if (args.length > 0)
            orc = Orc.makeOrc(args[0]);
        else
            orc = Orc.makeOrc();

        jf = new JFrame("OrcSpy: "+orc.getAddress());
        jf.setLayout(new BorderLayout());
        jf.add(jdp, BorderLayout.CENTER);

        basicDisplay = new TextPanelWidget(new String[] {"Parameter", "Value"},
                                           new String[][] {{"uorc time", "0"}},
                                           new double[] {.4, .6});

        analogDisplay = new TextPanelWidget(new String[] {"Port",
                                                          "Raw value",
                                                          "Value",
                                                          "LPF Value"},
            new String[][] { {"0", "0", "0", "0"},
                             {"1", "0", "0", "0"},
                             {"2", "0", "0", "0"},
                             {"3", "0", "0", "0"},
                             {"4", "0", "0", "0"},
                             {"5", "0", "0", "0"},
                             {"6", "0", "0", "0"},
                             {"7", "0", "0", "0"},
                             {" 8 (mot0)", "0", "0", "0"},
                             {" 9 (mot1)", "0", "0", "0"},
                             {"10 (mot2)", "0", "0", "0"},
                             {"11 (vbat)", "0", "0", "0"},
                             {"12 (temp)", "0", "0", "0"}},
            new double[] {.3, .5, .5, .5});

        digDisplay = new TextPanelWidget(new String[] {"Port",
                                                       "Mode",
                                                       "Value"},
            new String[][] { { "0", "--", "0"},
                             { "1", "--", "0"},
                             { "2", "--", "0"},
                             { "3", "--", "0"},
                             { "4", "--", "0"},
                             { "5", "--", "0"},
                             { "6", "--", "0"},
                             { "7", "--", "0"},
                             { "8  (mot0 fail)", "--", "0"},
                             { "9  (mot0 en)  ", "--", "0"},
                             { "10 (mot1 fail)", "--", "0"},
                             { "11 (mot1 en)  ", "--", "0"},
                             { "12 (mot2 fail)", "--", "0"},
                             { "13 (mot2 en)  ", "--", "0"},
                             { "14 (estop)    ", "--", "0"},
                             { "15 (button0)  ",    "--", "0"}},
            new double[] {.3, .5, .5});

        qeiDisplay = new TextPanelWidget(new String[] {"Port", "Position", "Velocity"},
                                         new String[][] {{"0", "0", "0"},
                                                         {"1", "0", "0"}},
                                         new double[] {.3, .5, .5});


        JPanel motDisplay = new JPanel();
        WeightedGridLayout motorWGL = new WeightedGridLayout(new double[] {0, 0, 0.8, 0.2});
        motorWGL.setDefaultRowWeight(1);
        motorWGL.setRowWeight(0, 0);
        motDisplay.setLayout(motorWGL);

        motDisplay.add(new JLabel("# "));
        motDisplay.add(new JLabel("En "));
        motDisplay.add(new JLabel("PWM "));
        motDisplay.add(new JLabel("Slew"));

        motorPanels = new MotorPanel[3];
        for (int i = 0; i < motorPanels.length; i++) {
            motorPanels[i] = new MotorPanel(motDisplay, i);
        }

        JPanel servoDisplay = new JPanel();
        servoDisplay.setLayout(new GridLayout(8,1));
        servoPanels = new ServoPanel[8];
        for (int i = 0; i < servoPanels.length; i++) {
            servoPanels[i] = new ServoPanel(i);
            JPanel jp = new JPanel();
            jp.setLayout(new BorderLayout());
            jp.add(new JLabel(" "+i), BorderLayout.WEST);
            jp.add(servoPanels[i], BorderLayout.CENTER);
            servoDisplay.add(jp);
        }

        jf.setSize(906,550);
        jf.setVisible(true);
        makeInternalFrame("Basic Properties", basicDisplay, 300, 120);

        makeInternalFrame("Analog Input", analogDisplay, 300, 285);
        makeInternalFrame("Quadrature Decoders", qeiDisplay, 300, 120);
        makeInternalFrame("Motors", motDisplay, 300, 200);
        makeInternalFrame("Servos", servoDisplay, 300, 300);
        makeInternalFrame("DigitalIO", digDisplay, 300, 330);

        new StatusPollThread().start();

        jf.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                System.exit(0);
            }});
    }

    int xpos = 0, ypos = 0;
    void makeInternalFrame(String name, JComponent jc, int width, int height)
    {
        JInternalFrame jif = new JInternalFrame(name, true, true, true, true);
        jif.setLayout(new BorderLayout());
        jif.add(jc, BorderLayout.CENTER);
        if (ypos+height >= jf.getHeight()) {
            ypos = 0;
            xpos += width;
        }
        jif.reshape(xpos, ypos, width, height);
        ypos += height;

        jif.setVisible(true);
        jdp.add(jif);
        jdp.setBackground(Color.blue);
    }

    class StatusPollThread extends Thread
    {
        StatusPollThread()
        {
            setDaemon(true);
        }

        public void run()
        {
            while(true) {
                try {
                    OrcStatus status = orc.getStatus();
                    orcStatus(orc, status);
                    Thread.sleep(100);
                } catch (Exception ex) {
                    System.out.println("Spy.StatusPollThread ex: "+ex);
                }
            }
        }
    }

    boolean firstStatus = true;

    public void orcStatus(Orc orc, OrcStatus status)
    {
        // if uorc resets...
        if (status.utimeOrc < 2000000)
            firstStatus = true;

        for (int i = 0; i < 2; i++) {
            qeiDisplay.values[i][1] = String.format("%6d", status.qeiPosition[i]);
            qeiDisplay.values[i][2] = String.format("%6d", status.qeiVelocity[i]);
        }
        basicDisplay.values[0][1] = String.format("%.6f", status.utimeOrc/1000000.0);

        for (int i = 0; i < 13; i++) {
            analogDisplay.values[i][1] = String.format("%04X", status.analogInput[i]);
            //	    analogDisplay.values[i][1] = String.format("%d", status.analogInput[i]);

            if (i < 8) {
                double voltage = status.analogInput[i] / 65535.0 * 5.0;
                analogDisplay.values[i][2] = String.format("%8.3f V  ", voltage);

                voltage = status.analogInputFiltered[i] / 65535.0 * 5.0;
                analogDisplay.values[i][3] = String.format("%8.3f V  ", voltage);
            } else if (i >=8 && i <= 10) {
                // motor current sense
                double voltage = status.analogInput[i] / 65535.0 * 3.0;
                double current = voltage * 375/200*1000;
                analogDisplay.values[i][2] = String.format("%8.0f mA ", current);

                voltage = status.analogInputFiltered[i] / 65535.0 * 3.0;
                current = voltage * 375/200*1000;
                analogDisplay.values[i][3] = String.format("%8.0f mA ", current);

            } else if (i==11) {
                // battery voltage sense
                double voltage = status.analogInput[i] / 65535.0 * 3.0;
                double batvoltage = voltage * 10.1;
                analogDisplay.values[i][2] = String.format("%8.2f V  ", batvoltage);

                voltage = status.analogInputFiltered[i] / 65535.0 * 3.0;
                batvoltage = voltage * 10.1;
                analogDisplay.values[i][3] = String.format("%8.2f V  ", batvoltage);

            } else {
                // MCU on-die temperature
                double voltage = status.analogInput[i] / 65535.0 * 3.0;
                double temp = -(voltage - 2.7) * 75 - 55; // from datasheet
                analogDisplay.values[i][2] = String.format("%8.2f degC", temp);

                voltage = status.analogInputFiltered[i] / 65535.0 * 3.0;
                temp = -(voltage - 2.7) * 75 - 55; // from datasheet
                analogDisplay.values[i][3] = String.format("%8.2f degC", temp);
            }
        }

        for (int i = 0; i < 16; i++) {
            int val = (status.simpleDigitalValues>>i)&1;
            digDisplay.values[i][2] = ""+val;
        }

        for (int i = 0; i < 3; i++) {
            motorPanels[i].pwmslider.setActualValue(status.motorPWMactual[i]);
            motorPanels[i].slewslider.setActualValue((int) (status.motorSlewSeconds[i] / motorPanels[i].slewslider.formatScale));

            //	    motorPanels[i].pwmslider.setBackground(fault==0 ? jf.getBackground() : Color.red);
            motorPanels[i].enabledCheckbox.setSelected(status.motorEnable[i]);

            if (firstStatus) {
                motorPanels[i].pwmslider.setGoalValue(status.motorPWMactual[i]);
                motorPanels[i].slewslider.setGoalValue((int) (status.motorSlewSeconds[i] / motorPanels[i].slewslider.formatScale));
            }
        }

        firstStatus = false;

        digDisplay.repaint();
        qeiDisplay.repaint();
        basicDisplay.repaint();
        analogDisplay.repaint();
    }

    class ServoPanel extends JPanel implements SmallSlider.Listener
    {
        SmallSlider slider = new SmallSlider(250, 3750, 1500, 1500, true);
        Servo servo;

        ServoPanel(int port)
        {
            slider.formatScale = 1;
            slider.formatString = "%.0f us";

            // pos/usecs don't matter, we specify pulse widths, not positions
            servo = new Servo(orc, port, 0, 0, 0, 0);

            setLayout(new BorderLayout());
            add(slider, BorderLayout.CENTER);

            slider.addListener(this);
        }

        public void goalValueChanged(SmallSlider slider, int goal)
        {
            servo.setPulseWidth(goal);
        }
    }

    class MotorPanel implements SmallSlider.Listener
    {
        SmallSlider pwmslider = new SmallSlider(-255, 255, 0, 0, true);
        SmallSlider slewslider = new SmallSlider(0, 65535, 0, 0, true);
        JCheckBox   enabledCheckbox = new JCheckBox();

        int port;
        Motor motor;

        MotorPanel(JPanel jp, int port)
        {
            this.motor = new Motor(orc, port, false);

            pwmslider.formatScale = 100.0 / 255.0;
            pwmslider.formatString = "%.0f %%";

            slewslider.formatString = "%.3fs";
            slewslider.formatScale = 3.0 / 65535.0;

            jp.add(new JLabel(" "+port));
            jp.add(enabledCheckbox);
            jp.add(pwmslider);
            jp.add(slewslider);

            pwmslider.addListener(this);
            slewslider.addListener(this);

            enabledCheckbox.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    enabledClicked();
                }
            });
        }

        public void goalValueChanged(SmallSlider slider, int goal)
        {
            if (slider == pwmslider)
                motor.setPWM(goal/255.0);
            if (slider == slewslider)
                motor.setSlewSeconds(goal*slewslider.formatScale);
        }

        void enabledClicked()
        {
            if (enabledCheckbox.isSelected()) {
                motor.setPWM(pwmslider.getGoalValue()/255.0);
            } else {
                motor.idle();
            }
        }
    }

    class TextPanelWidget extends JPanel
    {
        String panelName;
        String columnNames[];
        String values[][];
        double columnWidthWeight[];

        public TextPanelWidget(String columnNames[], String values[][],
                               double columnWidthWeight[])
        {
            this.columnNames = columnNames;
            this.columnWidthWeight = columnWidthWeight;
            this.values = values;
        }

        int getStringWidth(Graphics g, String s)
        {
            FontMetrics fm = g.getFontMetrics(g.getFont());
            return fm.stringWidth(s);
        }

        int getStringHeight(Graphics g, String s)
        {
            FontMetrics fm = g.getFontMetrics(g.getFont());
            return fm.getMaxAscent();
        }

        public Dimension getPreferredSize()
        {
            return new Dimension(100,100);
        }

        public Dimension getMinimumSize()
        {
            return getPreferredSize();
        }

        public Dimension getMaximumSize()
        {
            return new Dimension(1000,1000);
        }

        public void paint(Graphics _g)
        {
            Graphics2D g = (Graphics2D) _g;
            int width = getWidth(), height = getHeight();

            Font fb = new Font("Monospaced", Font.BOLD, 12);
            Font fp = new Font("Monospaced", Font.PLAIN, 12);
            g.setFont(fb);
            int textHeight = getStringHeight(g, "");

            ////////////////////////////////////////////
            // Draw backgrounds/outlines

            g.setColor(getBackground());
            g.fillRect(0, 0, width, height);

            g.setColor(Color.black);
            g.drawRect(0, 0, width-1, height-1);

            double totalColumnWidthWeight = 0;
            for (int i = 0; i < columnWidthWeight.length; i++)
                totalColumnWidthWeight += columnWidthWeight[i];

            int columnPos[] = new int[columnWidthWeight.length];
            int columnWidth[] = new int[columnWidthWeight.length];
            for (int i = 0; i < columnWidth.length; i++)
                columnWidth[i] = (int) (width * columnWidthWeight[i]/totalColumnWidthWeight);

            for (int i = 1; i < columnWidth.length; i++) {
                columnPos[i] = columnPos[i-1] + columnWidth[i-1];
            }

            ////////////////////////////////////////////
            // draw column headings
            g.setColor(Color.black);
            for (int i = 0; i < columnNames.length; i++) {
                String s = columnNames[i];
                g.drawString(s, columnPos[i]+columnWidth[i]/2 - getStringWidth(g, s)/2, textHeight*3/2);
            }

            ////////////////////////////////////////////
            // draw cells
            g.setFont(fp);
            for (int i = 0; i < values.length; i++) {
                for (int j = 0; j < columnNames.length; j++) {
                    String s = values[i][j];
                    if (s == null)
                        s = "";

                    g.drawString(s, columnPos[j]+columnWidth[j]/2 - getStringWidth(g, s)/2, textHeight*(3+i));
                }
            }

        }
    }
}
