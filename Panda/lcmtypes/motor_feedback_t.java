/* LCM type definition class file
 * This file was automatically generated by lcm-gen
 * DO NOT MODIFY BY HAND!!!!
 */

package lcmtypes;
 
import java.io.*;
import java.util.*;
import lcm.lcm.*;
 
public final class motor_feedback_t implements lcm.lcm.LCMEncodable
{
    public long utime;
    public boolean estop;
    public short nmotors;
    public int encoders[];
    public float current[];
    public float applied_voltage[];
 
    public motor_feedback_t()
    {
    }
 
    public static final long LCM_FINGERPRINT;
    public static final long LCM_FINGERPRINT_BASE = 0x1a793f72a7a09d71L;
 
    static {
        LCM_FINGERPRINT = _hashRecursive(new ArrayList<Class<?>>());
    }
 
    public static long _hashRecursive(ArrayList<Class<?>> classes)
    {
        if (classes.contains(lcmtypes.motor_feedback_t.class))
            return 0L;
 
        classes.add(lcmtypes.motor_feedback_t.class);
        long hash = LCM_FINGERPRINT_BASE
            ;
        classes.remove(classes.size() - 1);
        return (hash<<1) + ((hash>>63)&1);
    }
 
    public void encode(DataOutput outs) throws IOException
    {
        outs.writeLong(LCM_FINGERPRINT);
        _encodeRecursive(outs);
    }
 
    public void _encodeRecursive(DataOutput outs) throws IOException
    {
        outs.writeLong(this.utime); 
 
        outs.writeByte( this.estop ? 1 : 0); 
 
        outs.writeShort(this.nmotors); 
 
        for (int a = 0; a < this.nmotors; a++) {
            outs.writeInt(this.encoders[a]); 
        }
 
        for (int a = 0; a < this.nmotors; a++) {
            outs.writeFloat(this.current[a]); 
        }
 
        for (int a = 0; a < this.nmotors; a++) {
            outs.writeFloat(this.applied_voltage[a]); 
        }
 
    }
 
    public motor_feedback_t(byte[] data) throws IOException
    {
        this(new LCMDataInputStream(data));
    }
 
    public motor_feedback_t(DataInput ins) throws IOException
    {
        if (ins.readLong() != LCM_FINGERPRINT)
            throw new IOException("LCM Decode error: bad fingerprint");
 
        _decodeRecursive(ins);
    }
 
    public static lcmtypes.motor_feedback_t _decodeRecursiveFactory(DataInput ins) throws IOException
    {
        lcmtypes.motor_feedback_t o = new lcmtypes.motor_feedback_t();
        o._decodeRecursive(ins);
        return o;
    }
 
    public void _decodeRecursive(DataInput ins) throws IOException
    {
        this.utime = ins.readLong();
 
        this.estop = ins.readByte()!=0;
 
        this.nmotors = ins.readShort();
 
        this.encoders = new int[(int) nmotors];
        for (int a = 0; a < this.nmotors; a++) {
            this.encoders[a] = ins.readInt();
        }
 
        this.current = new float[(int) nmotors];
        for (int a = 0; a < this.nmotors; a++) {
            this.current[a] = ins.readFloat();
        }
 
        this.applied_voltage = new float[(int) nmotors];
        for (int a = 0; a < this.nmotors; a++) {
            this.applied_voltage[a] = ins.readFloat();
        }
 
    }
 
    public lcmtypes.motor_feedback_t copy()
    {
        lcmtypes.motor_feedback_t outobj = new lcmtypes.motor_feedback_t();
        outobj.utime = this.utime;
 
        outobj.estop = this.estop;
 
        outobj.nmotors = this.nmotors;
 
        outobj.encoders = new int[(int) nmotors];
        if (this.nmotors > 0)
            System.arraycopy(this.encoders, 0, outobj.encoders, 0, this.nmotors); 
        outobj.current = new float[(int) nmotors];
        if (this.nmotors > 0)
            System.arraycopy(this.current, 0, outobj.current, 0, this.nmotors); 
        outobj.applied_voltage = new float[(int) nmotors];
        if (this.nmotors > 0)
            System.arraycopy(this.applied_voltage, 0, outobj.applied_voltage, 0, this.nmotors); 
        return outobj;
    }
 
}

