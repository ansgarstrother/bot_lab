/* LCM type definition class file
 * This file was automatically generated by lcm-gen
 * DO NOT MODIFY BY HAND!!!!
 */

package lcmtypes;
 
import java.io.*;
import java.util.*;
import lcm.lcm.*;
 
public final class tri_loc_t implements lcm.lcm.LCMEncodable
{
    public boolean found;
    public float x_pos;
    public float y_pos;
    public double theta;
 
    public tri_loc_t()
    {
    }
 
    public static final long LCM_FINGERPRINT;
    public static final long LCM_FINGERPRINT_BASE = 0x58a5be8ecc8baaceL;
 
    static {
        LCM_FINGERPRINT = _hashRecursive(new ArrayList<Class<?>>());
    }
 
    public static long _hashRecursive(ArrayList<Class<?>> classes)
    {
        if (classes.contains(lcmtypes.tri_loc_t.class))
            return 0L;
 
        classes.add(lcmtypes.tri_loc_t.class);
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
        outs.writeByte( this.found ? 1 : 0); 
 
        outs.writeFloat(this.x_pos); 
 
        outs.writeFloat(this.y_pos); 
 
        outs.writeDouble(this.theta); 
 
    }
 
    public tri_loc_t(byte[] data) throws IOException
    {
        this(new LCMDataInputStream(data));
    }
 
    public tri_loc_t(DataInput ins) throws IOException
    {
        if (ins.readLong() != LCM_FINGERPRINT)
            throw new IOException("LCM Decode error: bad fingerprint");
 
        _decodeRecursive(ins);
    }
 
    public static lcmtypes.tri_loc_t _decodeRecursiveFactory(DataInput ins) throws IOException
    {
        lcmtypes.tri_loc_t o = new lcmtypes.tri_loc_t();
        o._decodeRecursive(ins);
        return o;
    }
 
    public void _decodeRecursive(DataInput ins) throws IOException
    {
        this.found = ins.readByte()!=0;
 
        this.x_pos = ins.readFloat();
 
        this.y_pos = ins.readFloat();
 
        this.theta = ins.readDouble();
 
    }
 
    public lcmtypes.tri_loc_t copy()
    {
        lcmtypes.tri_loc_t outobj = new lcmtypes.tri_loc_t();
        outobj.found = this.found;
 
        outobj.x_pos = this.x_pos;
 
        outobj.y_pos = this.y_pos;
 
        outobj.theta = this.theta;
 
        return outobj;
    }
 
}

