print("running extra...")
import gzip
import shutil
import os
import sys
import inspect
import json
GEN_DIR='generated'
CFG_FILE='web/config.json'
FILES=['web/index.html',CFG_FILE,'web/index.js','web/index.css']
CFG_INCLUDE='GwConfigDefinitions.h'

def basePath():
    #see: https://stackoverflow.com/questions/16771894/python-nameerror-global-name-file-is-not-defined
    return os.path.dirname(inspect.getfile(lambda: None))

def outPath():
    return os.path.join(basePath(),GEN_DIR)
def checkDir():
    dn=outPath()
    if not os.path.exists(dn):
        os.makedirs(dn)
    if not os.path.isdir(dn):
        print("unable to create %s"%dn)
        return False
    return True    

def isCurrent(infile,outfile):
    if os.path.exists(outfile):
        otime=os.path.getmtime(outfile)
        itime=os.path.getmtime(infile)
        if (otime >= itime):
            print("%s is newer then %s, no need to recreate"%(outfile,infile))
            return True
    return False        
def compressFile(inFile):
    outfile=os.path.basename(inFile)+".gz"
    inFile=os.path.join(basePath(),inFile)
    outfile=os.path.join(outPath(),outfile)
    if isCurrent(inFile,outfile):
        return
    with open(inFile, 'rb') as f_in:
        with gzip.open(outfile, 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)


def generateCfg():
    outfile=os.path.join(outPath(),CFG_INCLUDE)
    infile=os.path.join(basePath(),CFG_FILE)
    if isCurrent(infile,outfile):
        return
    print("creating %s"%CFG_INCLUDE)
    with open(CFG_FILE,'rb') as ch:
        config=json.load(ch)
        with open(outfile,'w') as oh:
            oh.write("//generated from %s\n"%CFG_FILE)
            oh.write('#include "GwConfigItem.h"\n')
            l=len(config)
            oh.write('class GwConfigDefinitions{\n')
            oh.write('  public:\n')
            oh.write('  int getNumConfig() const{return %d;}\n'%(l))
            for item in config:
                n=item.get('name')
                if n is None:
                    continue
                oh.write('  const String %s=F("%s");\n'%(n,n))
            oh.write('  protected:\n')    
            oh.write('  GwConfigItem *configs[%d]={\n'%(l))
            first=True
            for item in config:
                if not first:
                    oh.write(',\n')
                first=False    
                oh.write("    new GwConfigItem(%s,\"%s\")"%(item.get('name'),item.get('default')))
            oh.write('};\n')  
            oh.write('};\n')
            oh.close()  
if not checkDir():
    sys.exit(1)
for f in FILES:
    print("compressing %s"%f)
    compressFile(f)
generateCfg()    