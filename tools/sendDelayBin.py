#! /usr/bin/env python3
import sys
import os
import time
import getopt

def usage():
    print(f"usage: {sys.argv[0]} file delay")
    sys.exit(1)

try:
    opts,args=getopt.getopt(sys.argv[1:],"d:")
except getopt.GetoptError as e:
    print(f"Error: {e}",file=sys.stderr)
    sys.exit(1)
delim=0x10    
for o,a in opts:
    if o == 'd':
        delim=int(a)
if len(args) < 2:
    usage()
delim=delim.to_bytes()
delay=float(args[1])
fn=args[0]
if fn == "-":
    fn = "/dev/stdin"
with open (fn,"rb") as fh:
    while True:
        r=fh.read(1)
        if len(r) < 1:
            break
        sys.stdout.buffer.write(r)
        if r == delim:
            time.sleep(delay)
            sys.stdout.flush()


