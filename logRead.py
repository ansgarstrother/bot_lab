import struct
import sys
import lcm

f = file(sys.argv[1])
l = lcm.LCM()

while True:
  t = f.read(28)
  if len(t) < 28: break
  sync, evno, tstamp, clen, dlen = struct.unpack('>LqqLL',t)
  chan = f.read(clen)
  data = f.read(dlen)
  l.publish(chan, data)
f.close()