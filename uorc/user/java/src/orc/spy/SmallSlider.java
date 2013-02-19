package orc.spy;

import java.awt.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import java.io.*;
import java.util.*;

/** A slider that has a "current" value, and a "goal" value. The user
    controls the goal value, but has only indirect control over the
    current value. **/

public class SmallSlider extends JComponent
{
    int barheight=8;

    int minvalue=0;
    int maxvalue=100;

    int goalvalue, actualvalue;

    int goalknobsize=6;
    int actualknobsize=10;

    int totalheight=actualknobsize+4;

    int marginx=6;

    ArrayList<Listener> gsls=new ArrayList<Listener>();

    boolean copyactual=false;

    boolean showactual=true;

    public String formatString = "%.0f";
    public double formatScale = 1.0;

    public interface Listener
    {
        public void goalValueChanged(SmallSlider ss, int goalvalue);
    }

    /** Don't specify actual values; both the goal and actual will be copied
        from the first actual notification. **/
    public SmallSlider(int min, int max, boolean showactual)
    {
        this.minvalue=min;
        this.maxvalue=max;
        this.goalvalue=minvalue;
        this.actualvalue=minvalue;
        this.showactual=showactual;

        addMouseMotionListener(new SmallSliderMouseMotionListener());
        addMouseListener(new SmallSliderMouseMotionListener());

        copyactual=true;
    }

    public SmallSlider(int min, int max, int goalvalue, int actualvalue, boolean showactual)
    {
        this.minvalue=min;
        this.maxvalue=max;

        this.goalvalue=goalvalue;
        this.actualvalue=actualvalue;
        this.showactual = showactual;

        addMouseMotionListener(new SmallSliderMouseMotionListener());
        addMouseListener(new SmallSliderMouseMotionListener());
    }

    public void addListener(Listener gsl)
    {
        gsls.add(gsl);
    }

    public void setMaximum(int i)
    {
        maxvalue=i;
        repaint();
    }

    public void setMinimum(int i)
    {
        minvalue=i;
        repaint();
    }

    public synchronized void setActualValue(int i)
    {
        if (copyactual)
	    {
            goalvalue=i;
            copyactual=false;
	    }

        actualvalue=i;
        repaint();
    }

    /** Only call this during initialization; it should be under user control only! **/
    public void setGoalValue(int i)
    {
        goalvalue=i;
        repaint();
    }

    public int getGoalValue()
    {
        return goalvalue;
    }

    public int getActualValue()
    {
        return actualvalue;
    }

    public Dimension getMinimumSize()
    {
        return new Dimension(40, totalheight);
    }

    public Dimension getPreferredSize()
    {
        return getMinimumSize();
    }

    public synchronized void paint(Graphics gin)
    {
        Graphics2D g=(Graphics2D) gin;
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        int height=getHeight();
        int width=getWidth()-2*marginx;
        int cy=height/2;
        int cx=width/2;
        int x;

        g.translate(marginx, 0);

        g.setColor(getParent().getBackground());
        g.fillRect(0,0,width,height);

        /////// the bar
        RoundRectangle2D.Double barr=new RoundRectangle2D.Double(0, cy-barheight/2,
                                                                 width, barheight,
                                                                 barheight, barheight);

        g.setColor(Color.white);
        g.fill(barr);
        g.setColor(Color.black);
        g.draw(barr);

        ////// goal knob
        x=width*(goalvalue-minvalue)/(maxvalue-minvalue);
        Ellipse2D.Double goalknob=new Ellipse2D.Double(x - goalknobsize/2,
                                                       cy - goalknobsize/2,
                                                       goalknobsize, goalknobsize);
        g.setColor(Color.green);
        g.fill(goalknob);
        g.setStroke(new BasicStroke(1.0f));
        g.setColor(Color.black);
        g.draw(goalknob);
        g.setFont(new Font("Monospaced", Font.PLAIN, 11));

        if (showactual)
	    {
            /////// actual knob
            x=width*(actualvalue-minvalue)/(maxvalue-minvalue);
            g.setColor(Color.black);
            g.setStroke(new BasicStroke(1.0f));
            Ellipse2D.Double actualknob=new Ellipse2D.Double(x - actualknobsize/2,
                                                             cy - actualknobsize/2,
                                                             actualknobsize, actualknobsize);

            g.draw(actualknob);
	    }

        if (true)
	    {
            g.setColor(Color.black);
            String s = String.format(formatString, formatScale*goalvalue);
            g.drawString(s, width - s.length()*8, cy + 16);
	    }
    }

    void handleClick(int x)
    {
        goalvalue=minvalue+(maxvalue-minvalue)*(x-marginx)/(getWidth()-2*marginx);
        if (goalvalue<minvalue)
            goalvalue=minvalue;
        if (goalvalue>maxvalue)
            goalvalue=maxvalue;

        for (Listener gsl: gsls)
            gsl.goalValueChanged(this, goalvalue);

        repaint();
    }

    class SmallSliderMouseMotionListener implements MouseMotionListener, MouseListener
    {
        public void mouseDragged(MouseEvent e)
        {
            handleClick(e.getX());
        }

        public void mouseMoved(MouseEvent e)
        {
        }

        public void mouseClicked(MouseEvent e)
        {
            handleClick(e.getX());
        }

        public void mouseEntered(MouseEvent e)
        {
        }

        public void mouseExited(MouseEvent e)
        {
        }

        public void mousePressed(MouseEvent e)
        {
            handleClick(e.getX());
        }

        public void mouseReleased(MouseEvent e)
        {
        }

    }

}
