print("running extra...")
import gzip
import shutil
import os
import sys
import inspect
GEN_DIR='generated'
FILES=['web/index.html']

def outPath():
    #see: https://stackoverflow.com/questions/16771894/python-nameerror-global-name-file-is-not-defined
    return os.path.join(os.path.dirname(inspect.getfile(lambda: None)),GEN_DIR)

def checkDir():
    dn=outPath()
    if not os.path.exists(dn):
        os.makedirs(dn)
    if not os.path.isdir(dn):
        print("unable to create %s"%dn)
        return False
    return True    

def compressFile(inFile):
    outfile=os.path.basename(inFile)+".gz"
    outfile=os.path.join(outPath(),outfile)
    if os.path.exists(outfile):
        otime=os.path.getmtime(outfile)
        itime=os.path.getmtime(inFile)
        if (otime >= itime):
            print("%s is newer then %s, no need to recreate"%(outfile,inFile))
            return
    with open(inFile, 'rb') as f_in:
        with gzip.open(outfile, 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)

if not checkDir():
    sys.exit(1)
for f in FILES:
    print("compressing %s"%f)
    compressFile(f)