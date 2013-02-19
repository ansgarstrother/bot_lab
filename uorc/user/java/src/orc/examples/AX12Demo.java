package orc.examples;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.*;
import javax.swing.event.*;

import orc.*;

public class AX12Demo
{
    JFrame jf = new JFrame("AX12Demo");
    ArrayList<AX12Servo> servos;

    public static void main(String args[])
    {
        Orc orc = Orc.makeOrc();
        orc.verbose = true;

        System.out.printf("Checking communication with Orc board...");
        System.out.flush();
        OrcStatus os = orc.getStatus();
        System.out.println("good!");

        System.out.println("Scanning for AX12 servos...");
        ArrayList<AX12Servo> servos = new ArrayList<AX12Servo>();

        for (int id = 0; id < 254; id++) {
            System.out.printf("%4d\r", id);
            System.out.flush();
            AX12Servo servo = new AX12Servo(orc, id);
            if (servo.ping()) {
                System.out.printf("%d  found!\n", id);
                servos.add(servo);
            }
        }

        if (servos.size() == 0) {
            System.out.println("No AX12 servos found.");
            System.exit(0);
        }

        new AX12Demo(servos);
    }


    public AX12Demo(ArrayList<AX12Servo> servos)
    {
        this.servos = servos;

        jf.setLayout(new GridLayout(servos.size(), 1));

        for (AX12Servo s : servos) {
            jf.add(new AX12Widget(s));
        }

        jf.setSize(800,400);
        jf.setVisible(true);
    }

    class AX12Widget extends JPanel implements ChangeListener
    {
        AX12Servo servo;
        JSlider js;
        JLabel  positionLabel = new JLabel("");
        JLabel  speedLabel = new JLabel("");
        JLabel  loadLabel = new JLabel("");
        JLabel  voltageLabel = new JLabel("");
        JLabel  tempLabel = new JLabel("");
        JLabel  errorsLabel = new JLabel("");

        JSlider positionSlider = new JSlider(0, 300, 150);

        public AX12Widget(AX12Servo servo)
        {
            this.servo = servo;

            setBorder(BorderFactory.createTitledBorder("AX12 id "+servo.getId()));

            setLayout(new BorderLayout());

            JPanel jp = new JPanel();
            jp.setLayout(new GridLayout(6,2));
            jp.add(new JLabel("Position"));
            jp.add(positionLabel);
            jp.add(new JLabel("Speed"));
            jp.add(speedLabel);
            jp.add(new JLabel("Load"));
            jp.add(loadLabel);
            jp.add(new JLabel("Voltage"));
            jp.add(voltageLabel);
            jp.add(new JLabel("Temp. (C)  "));
            jp.add(tempLabel);
            jp.add(new JLabel("Errors"));
            jp.add(errorsLabel);

            add(jp, BorderLayout.EAST);
            add(positionSlider, BorderLayout.CENTER);
            positionSlider.addChangeListener(this);

            new RunThread().start();
        }

        public void stateChanged(ChangeEvent e)
        {
            if (e.getSource() == positionSlider) {
                servo.setGoalDegrees(positionSlider.getValue(), 0.3, 0.3);
            }
        }

        class RunThread extends Thread
        {
            public void run()
            {
                while (true) {
                    AX12Status status = servo.getStatus();
                    positionLabel.setText(String.format("%.3f", status.positionDegrees));
                    speedLabel.setText(String.format("%.3f", status.speed));
                    loadLabel.setText(String.format("%.3f", status.load));
                    voltageLabel.setText(String.format("%.3f", status.voltage));
                    errorsLabel.setText(String.format("%02x", status.error_flags));
                    tempLabel.setText(String.format("%.1f", status.temperature));

                    try {
                        Thread.sleep(50);
                    } catch (InterruptedException ex) {
                    }
                }
            }
        }
    }
}
