#! /usr/bin/env python3
import sys
import json

def err(txt):
    print(txt,file=sys.stderr)
    sys.exit(1)

HDR='''
PGNM_Fast=0
PGNM_Single=1
PGNM_ISO=2
PGN_MODES={
'''
FOOTER='''
    }
'''
with open(sys.argv[1],"r") as ih:
    data=json.load(ih)
    pgns=data.get('PGNs')
    if pgns is None:
        err("no pgns")
    print(HDR)
    for p in pgns:
        t=p['Type']
        pgn=p['PGN']
        if t and pgn:
            print(f"    {pgn}: PGNM_{t},")
    print(FOOTER)