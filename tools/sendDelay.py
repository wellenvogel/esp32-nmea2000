#! /usr/bin/env python3
import sys
import os
import time

def usage():
    print(f"usage: {sys.argv[0]} file delay")
    sys.exit(1)

if len(sys.argv) < 3:
    usage()

delay=float(sys.argv[2])
fn=sys.argv[1]
with open (fn,"r") as fh:
    for line in fh:
        print(line,end="",flush=True)
        time.sleep(delay)

