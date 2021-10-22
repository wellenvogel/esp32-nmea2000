print("running extra...")
import gzip
import shutil
import os
FILES=['web/index.html']

def compressFile(inFile):
    outfile=inFile+".gz"
    if os.path.exists(outfile):
        otime=os.path.getmtime(outfile)
        itime=os.path.getmtime(inFile)
        if (otime >= itime):
            print("%s is newer then %s, no need to recreate"%(outfile,inFile))
            return
    with open(inFile, 'rb') as f_in:
        with gzip.open(outfile, 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)

for f in FILES:
    print("compressing %s"%f)
    compressFile(f)