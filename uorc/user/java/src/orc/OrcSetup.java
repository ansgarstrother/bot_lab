package orc;

import java.awt.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import java.util.*;
import java.io.*;
import java.net.*;

import orc.*;
import orc.spy.*;

/** Utility for discovering uOrcs on the local network and changing their IP and MAC addresses. **/
public class OrcSetup
{
    class OrcDetection implements Comparable<OrcDetection>
    {
        InetAddress addr;
        long        mstime; // time of last response.

        int         bootNonce;

        // parameter block variables, in Java byte order
        int         magic;
        int         version;
        int         length;
        int         ip4addr;
        int         ip4mask;
        int         ip4gw;
        long        macaddr;
        boolean     dhcpd_enable;
        int         magic2;

        public int compareTo(OrcDetection od)
        {
            if (od.ip4addr == ip4addr)
                return od.bootNonce - bootNonce;

            return od.ip4addr - ip4addr;
        }
    }

    InetAddress broadcastInetAddr;

    OrcDetection selectedOrcDetection = null;

    ArrayList<OrcDetection> detections = new ArrayList<OrcDetection>();

    JFrame jf;
    HashMap<String, Param> paramsMap = new HashMap<String, Param>();
    JPanel  paramsPanel;

    JButton revertButton = new JButton("Revert");
    JButton randomMACButton = new JButton("Randomize MAC");
    JButton writeButton = new JButton("Write to uOrc");

    JTextArea log = new JTextArea();

    MyListModel mdl = new MyListModel();
    JList orcList = new JList(mdl);
    ArrayList<String> orcIps = new ArrayList<String>();

    Random rand = new Random();

    FindOrcThread finder;

    DatagramSocket sock;

    static final int FLASH_PARAM_SIGNATURE = 0xed719abf;
    static final int FLASH_PARAM_VERSION   = 0x0002;
    static final int FLASH_PARAM_LENGTH    = 37;

    public static void main(String args[])
    {
        new OrcSetup();
    }

    interface Param
    {
        public String getValue();
        public void setValue(String v);
        public JComponent getComponent();
    }

    class BooleanParam implements Param
    {
        JCheckBox checkBox = new JCheckBox();

        public String getValue()
        {
            return checkBox.isSelected() ? "true" : "false";
        }

        public void setValue(String v)
        {
            checkBox.setSelected(v.equals("true"));
        }

        public JComponent getComponent()
        {
            return checkBox;
        }
    }

    class StringParam implements Param
    {
        JTextField textField = new JTextField();

        public String getValue()
        {
            return textField.getText();
        }

        public void setValue(String v)
        {
            textField.setText(v);
        }

        public JComponent getComponent()
        {
            return textField;
        }
    }

    void addParamIPAddress(String name)
    {
        paramsPanel.add(new JLabel(name));
        Param p = new StringParam();
        paramsPanel.add(p.getComponent());
        paramsMap.put(name, p);
    }

    void addParamMACAddress(String name)
    {
        paramsPanel.add(new JLabel(name));
        Param p = new StringParam();
        paramsPanel.add(p.getComponent());
        paramsMap.put(name, p);
    }

    void addParamBoolean(String name)
    {
        paramsPanel.add(new JLabel(name));
        Param p = new BooleanParam();
        paramsPanel.add(p.getComponent());
        paramsMap.put(name, p);
    }

    ////////////////////////////////////////////////////////////////
    public OrcSetup()
    {
        try {
            broadcastInetAddr = Inet4Address.getByName("192.168.237.255");
        } catch (UnknownHostException ex) {
            System.out.println("ex: "+ex);
            System.exit(-1);
        }

        finder = new FindOrcThread();
        finder.start();

        orcList.addListSelectionListener(new ListSelectionListener() {
            public void valueChanged(ListSelectionEvent e)
            {
                OrcSetup.this.listChanged();
            }
	    });

        jf = new JFrame("OrcSetup");
        jf.setLayout(new BorderLayout());

        paramsPanel = new JPanel();
        WeightedGridLayout wgl = new WeightedGridLayout(new double[] {0.1, 0.9});
        wgl.setDefaultRowWeight(0);
        paramsPanel.setLayout(wgl);

        addParamIPAddress("ipaddr");
        addParamIPAddress("ipmask");
        addParamIPAddress("ipgw");
        addParamMACAddress("macaddr");
        addParamBoolean("dhcpd_enable");

        JPanel buttonPanel = new JPanel();
        buttonPanel.setLayout(new FlowLayout());
        buttonPanel.add(revertButton);
        buttonPanel.add(randomMACButton);
        buttonPanel.add(writeButton);

        JPanel mainPanel = new JPanel();
        mainPanel.setLayout(new BorderLayout());
        mainPanel.add(paramsPanel, BorderLayout.CENTER);
        mainPanel.add(buttonPanel, BorderLayout.SOUTH);

        JPanel orcListPanel = new JPanel();
        orcListPanel.setLayout(new BorderLayout());
        orcListPanel.add(new JLabel("uOrcs found:"), BorderLayout.NORTH);
        orcListPanel.add(new JScrollPane(orcList), BorderLayout.CENTER);
        orcListPanel.add(Box.createHorizontalStrut(10), BorderLayout.EAST);

        mainPanel.add(orcListPanel, BorderLayout.WEST);

        JSplitPane jsp = new JSplitPane(JSplitPane.VERTICAL_SPLIT, mainPanel, new JScrollPane(log));
        jf.add(jsp, BorderLayout.CENTER);
        jsp.setDividerLocation(0.65);
        jsp.setResizeWeight(0.65);

        revertButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                revert();
            }
	    });

        randomMACButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                randomMAC();
            }
	    });

        writeButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                write();
            }
	    });

        jf.setSize(600,500);
        jf.setVisible(true);

        log.setEditable(false);
        log.setFont(new Font("Monospaced", Font.PLAIN, 12));

        logAppend("Started.");

        listChanged();

    }

    void listChanged()
    {
        int idx = orcList.getSelectedIndex();

        if (idx < 0) {
            paramsMap.get("ipaddr").setValue("");
            paramsMap.get("ipmask").setValue("");
            paramsMap.get("ipgw").setValue("");
            paramsMap.get("macaddr").setValue("");

            revertButton.setEnabled(false);
            randomMACButton.setEnabled(false);
            writeButton.setEnabled(false);
            return;
        }

        selectedOrcDetection = detections.get(idx);
        paramsMap.get("ipaddr").setValue(ip2string(selectedOrcDetection.ip4addr));
        paramsMap.get("ipmask").setValue(ip2string(selectedOrcDetection.ip4mask));
        paramsMap.get("ipgw").setValue(ip2string(selectedOrcDetection.ip4gw));
        paramsMap.get("macaddr").setValue(mac2string(selectedOrcDetection.macaddr));
        paramsMap.get("dhcpd_enable").setValue(boolean2string(selectedOrcDetection.dhcpd_enable));
        revertButton.setEnabled(true);
        randomMACButton.setEnabled(true);
        writeButton.setEnabled(true);
    }

    void revert()
    {
        listChanged();
    }

    void randomMAC()
    {
        long r = rand.nextLong();

        Param p = paramsMap.get("macaddr");
        p.setValue(String.format("%02x:%02x:%02x:%02x:%02x:%02x",
                                 2, 0, (r>>24)&0xff, (r>>16)&0xff, (r>>8)&0xff, r&0xff));
    }

    void logAppend(String s)
    {
        log.append(s);
        log.setCaretPosition(log.getText().length() - 1);
    }

    int swap(int v)
    {
        return (((v>>24)&0xff)<<0) +
            (((v>>16)&0xff)<<8) +
            (((v>>8)&0xff)<<16) +
            ((v&0xff)<<24);
    }

    long swap(long v)
    {
        return
            (((v>>56)&0xff)<<0) +
            (((v>>48)&0xff)<<8) +
            (((v>>40)&0xff)<<16) +
            (((v>>32)&0xff)<<24) +
            (((v>>24)&0xff)<<32) +
            (((v>>16)&0xff)<<40) +
            (((v>>8)&0xff)<<48) +
            ((v&0xff)<<56);
    }

    int string2ip(String s)
    {
        String toks[] = s.split("\\.");
        if (toks.length!=4)
            throw new RuntimeException("Invalid IP format");

        return (Integer.parseInt(toks[0])<<24) +
            (Integer.parseInt(toks[1])<<16) +
            (Integer.parseInt(toks[2])<<8) +
            (Integer.parseInt(toks[3]));
    }


    String ip2string(int ip)
    {
        return String.format("%d.%d.%d.%d",
                             (ip>>24)&0xff,
                             (ip>>16)&0xff,
                             (ip>>8)&0xff,
                             ip&0xff);
    }

    long string2mac(String s)
    {
        String toks[] = s.split(":");
        if (toks.length!=6)
            throw new RuntimeException("Invalid MAC address format");

        return (((long) Integer.parseInt(toks[0],16))<<40) +
            (((long) Integer.parseInt(toks[1],16))<<32) +
            (((long) Integer.parseInt(toks[2],16))<<24) +
            (((long) Integer.parseInt(toks[3],16))<<16) +
            (((long) Integer.parseInt(toks[4],16))<<8) +
            (((long) Integer.parseInt(toks[5],16))<<0);
    }

    boolean string2boolean(String s)
    {
        s = s.toLowerCase();
        return (s.equals("true"));
    }

    String boolean2string(boolean b)
    {
        if (b)
            return "true";
        return "false";
    }

    String mac2string(long mac)
    {
        return String.format("%02x:%02x:%02x:%02x:%02x:%02x",
                             (mac>>40)&0xff,
                             (mac>>32)&0xff,
                             (mac>>24)&0xff,
                             (mac>>16)&0xff,
                             (mac>>8)&0xff,
                             mac&0xff);
    }

    void logArray(byte buf[], int offset, int len)
    {
        for (int i = 0; i < len - offset; i++) {
            if (i % 16 == 0)
                logAppend(String.format("%04x : ", i));
            logAppend(String.format("%02x ", buf[offset + i]));
            if (i % 16 == 15)
                logAppend("\n");
        }
        logAppend("\n");
    }

    void write()
    {
        try {
            ByteArrayOutputStream bouts = new ByteArrayOutputStream();
            DataOutputStream outs = new DataOutputStream(bouts);

            OrcDetection od = detections.get(orcList.getSelectedIndex());

            outs.writeInt(0xedaab739); // magic
            outs.writeInt(0x0000ff01); // write flash command
            outs.writeInt(od.bootNonce);
            outs.writeInt(128);        // flash length

            /////// actual flash parameters begin here.
            outs.writeInt(swap(FLASH_PARAM_SIGNATURE));
            outs.writeInt(swap(FLASH_PARAM_VERSION)); // version
            outs.writeInt(swap(FLASH_PARAM_LENGTH)); // length

            outs.writeInt(swap(string2ip(paramsMap.get("ipaddr").getValue())));
            outs.writeInt(swap(string2ip(paramsMap.get("ipmask").getValue())));
            outs.writeInt(swap(string2ip(paramsMap.get("ipgw").getValue())));
            outs.writeLong(swap(string2mac(paramsMap.get("macaddr").getValue())));
            outs.writeByte(string2boolean(paramsMap.get("dhcpd_enable").getValue()) ? 1 : 0);

            outs.writeInt(swap(FLASH_PARAM_SIGNATURE));

            logAppend("Writing parameter block...\n");

            byte p[] = bouts.toByteArray();

            System.out.println(od.addr);

            /** We used to send to the IP address directly, but this
             * didn't work on a multi-homed ethernet device. Anyway, I
             * have more confidence that if two uOrcs have the same
             * MAC/IP, that the broadcast is more likely to reach
             * the one we want.
             **/
            DatagramPacket packet = new DatagramPacket(p, p.length, broadcastInetAddr, 2377);
            sock.send(packet);

            logAppend("...finished. Reset uORC for settings to take effect.\n\n");

        } catch (IOException ex) {
            System.out.println("ex: "+ex);
        }
    }

    class MyListModel implements ListModel
    {
        ArrayList<ListDataListener> listeners = new ArrayList<ListDataListener>();

        public void addListDataListener(ListDataListener listener)
        {
            listeners.add(listener);
        }

        public Object getElementAt(int index)
        {
            OrcDetection od = detections.get(index);
            return String.format("%08x : %s", od.bootNonce, ip2string(od.ip4addr));
        }

        public int getSize()
        {
            return detections.size();
        }

        public void removeListDataListener(ListDataListener listener)
        {
            listeners.remove(listener);
        }

        public void changed()
        {
            for (ListDataListener listener : listeners) {
                listener.contentsChanged(new ListDataEvent(this, ListDataEvent.CONTENTS_CHANGED, 0, getSize()));
            }
        }

    }

    ///////////////////////////////////////////////////////////////////////
    // This class finds all the uORCs on the LAN and puts them in a list.
    class FindOrcThread extends Thread
    {
        HashMap<Integer,OrcDetection> detectionsMap = new HashMap<Integer,OrcDetection>();

        ReaderThread   reader;

        public FindOrcThread()
        {
            setDaemon(true);
        }

        public void run()
        {
            try {
                runEx();
            } catch (IOException ex) {
                System.out.println("Ex: "+ex);
            } catch (InterruptedException ex) {
                System.out.println("Ex: "+ex);
            }
        }

        void runEx() throws IOException, InterruptedException
        {
            sock = new DatagramSocket(2377);

            reader = new ReaderThread();
            reader.setDaemon(true);
            reader.start();

            while (true) {

                ByteArrayOutputStream bouts = new ByteArrayOutputStream();
                DataOutputStream outs = new DataOutputStream(bouts);
                outs.writeInt(0xedaab739); // magic.
                outs.writeInt(0);          // cmd = query.

                byte p[] = bouts.toByteArray();
                DatagramPacket packet = new DatagramPacket(p, p.length, broadcastInetAddr, 2377);
                sock.send(packet);

                Thread.sleep(500);

                ArrayList<OrcDetection> goodDetections = new ArrayList<OrcDetection>();
                synchronized(detectionsMap) {
                    // remove any OrcRecords that we haven't heard from for a while.
                    for (OrcDetection od : detectionsMap.values()) {
                        double age = (System.currentTimeMillis() - od.mstime)/1000.0;
                        if (age < 1.2)
                            goodDetections.add(od);
                    }

                    detectionsMap.clear();
                    for (OrcDetection od : goodDetections)
                        detectionsMap.put(od.bootNonce, od);
                }

                Collections.sort(goodDetections);
                boolean changed = false;
                if (goodDetections.size() != detections.size())
                    changed = true;

                if (!changed) {
                    for (int i = 0; i < goodDetections.size(); i++) {
                        if (goodDetections.get(i) != detections.get(i))
                            changed = true;
                    }
                }

                if (changed) {
                    detections = goodDetections;

                    if (selectedOrcDetection!=null && !detections.contains(selectedOrcDetection)) {
                        orcList.clearSelection();
                        listChanged();
                    }

                    mdl.changed();
                }
            }
        }

        class ReaderThread extends Thread
        {
            public void run()
            {
                try {
                    runEx();
                } catch (IOException ex) {
                    System.out.println("Ex: "+ex);
                }
            }

            void runEx() throws IOException
            {
                while (true)  {
                    byte buf[] = new byte[1600];
                    DatagramPacket packet = new DatagramPacket(buf, buf.length);

                    sock.receive(packet);
                    DataInputStream ins = new DataInputStream(new ByteArrayInputStream(buf, 0, packet.getLength()));
                    if (ins.available()<4)
                        continue;

                    int magic = ins.readInt();

                    if (magic != 0x754f5243)
                        continue;

                    int bootNonce = ins.readInt();

                    OrcDetection od = detectionsMap.get(bootNonce);
                    if (od == null) {
                        od = new OrcDetection();
                        od.bootNonce = bootNonce;
                        detectionsMap.put(bootNonce, od);
                    }

                    od.addr = packet.getAddress();
                    od.mstime = System.currentTimeMillis();

                    od.magic   = swap(ins.readInt());
                    od.version = swap(ins.readInt());
                    od.length  = swap(ins.readInt());

                    od.ip4addr = swap(ins.readInt());
                    od.ip4mask = swap(ins.readInt());
                    od.ip4gw   = swap(ins.readInt());
                    od.macaddr = swap(ins.readLong());
                    od.dhcpd_enable = ins.readByte() != 0;

                    od.magic2  = swap(ins.readInt());
                    //   System.out.printf("Received from uORC with bootnonce: %08x\n",od.bootNonce);
                }
            }
        }
    }
}
