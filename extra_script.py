print("running extra...")
import gzip
import shutil
import os
import sys
import inspect
import json
from datetime import datetime
Import("env")
GEN_DIR='generated'
CFG_FILE='web/config.json'
XDR_FILE='web/xdrconfig.json'
CFG_INCLUDE='GwConfigDefinitions.h'
XDR_INCLUDE='GwXdrTypeMappings.h'
EMBEDDED_INCLUDE="GwEmbeddedFiles.h"

def getEmbeddedFiles(env):
    rt=[]
    efiles=env.GetProjectOption("board_build.embed_files")
    for f in efiles.split("\n"):
        if f == '':
            continue
        rt.append(f)
    return rt  

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
def compressFile(inFile,outfile):
    if isCurrent(inFile,outfile):
        return
    with open(inFile, 'rb') as f_in:
        with gzip.open(outfile, 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)

def generateFile(infile,outfile,callback,inMode='rb',outMode='w'):
    if isCurrent(infile,outfile):
        return
    print("creating %s"%outfile)
    oh=None
    with open(infile,inMode) as ch:
        with open(outfile,'w') as oh:
            try:
                callback(ch,oh,inFile=infile)
                oh.close()
            except Exception as e:
                try:
                    oh.close()
                except:
                    pass
                os.unlink(outfile)
                raise

def writeFileIfChanged(fileName,data):
    if os.path.exists(fileName):
        with open(fileName,"r") as ih:
            old=ih.read()
            ih.close()
            if old == data:
                return
    print("#generating %s"%fileName)
    with open(fileName,"w") as oh:
        oh.write(data)

def generateCfg(ch,oh,inFile=''):
    config=json.load(ch)      
    oh.write("//generated from %s\n"%inFile)
    oh.write('#include "GwConfigItem.h"\n')
    l=len(config)
    oh.write('class GwConfigDefinitions{\n')
    oh.write('  public:\n')
    oh.write('  int getNumConfig() const{return %d;}\n'%(l))
    for item in config:
        n=item.get('name')
        if n is None:
            continue
        if len(n) > 15:
            raise Exception("%s: config names must be max 15 caracters"%n)
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
                    

def generateXdrMappings(fp,oh,inFile=''):
    jdoc=json.load(fp)
    oh.write("static GwXDRTypeMapping* typeMappings[]={\n")
    first=True
    for cat in jdoc:
        item=jdoc[cat]
        cid=item.get('id')
        if cid is None:
            continue
        tc=item.get('type')
        if tc is not None:
            if first:
                first=False
            else:
                oh.write(",\n")
            oh.write("   new GwXDRTypeMapping(%d,%d,0) /*%s*/"%(cid,tc,cat))
        fields=item.get('fields')
        if fields is None:
            continue
        idx=0
        for fe in fields:
            if not isinstance(fe,dict):
                continue
            tc=fe.get('t')
            id=fe.get('v')
            if id is None:
                id=idx
            idx+=1
            l=fe.get('l') or ''
            if tc is None or id is None:
                continue
            if first:
                first=False
            else:
                oh.write(",\n")
            oh.write("   new GwXDRTypeMapping(%d,%d,%d) /*%s:%s*/"%(cid,tc,id,cat,l))
    oh.write("\n")
    oh.write("};\n")

def generateEmbedded(elist,outFile):
    content=""
    for entry in elist:
        content+="EMBED_GZ_FILE(\"%s\",%s,\"%s\");\n"%entry
    writeFileIfChanged(outFile,content)    

def getContentType(fn):
    if (fn.endswith('.gz')):
        fn=fn[0:-3]
    if (fn.endswith('html')):
        return "text/html"
    if (fn.endswith('json')):
        return "application/json"
    if (fn.endswith('js')):
        return "text/javascript"
    return "application/octet-stream"

def prebuild(env):
    print("#prebuild running")
    if not checkDir():
        sys.exit(1)
    embedded=getEmbeddedFiles(env)
    filedefs=[]
    for ef in embedded:
        print("#checking embedded file %s"%ef)
        (dn,fn)=os.path.split(ef)
        pureName=fn
        if pureName.endswith('.gz'):
            pureName=pureName[0:-3]
        ct=getContentType(pureName)
        usname=ef.replace('/','_').replace('.','_')
        filedefs.append((pureName,usname,ct))
        inFile=os.path.join(basePath(),"web",pureName)
        if os.path.exists(inFile):
            print("compressing %s"%inFile)
            compressFile(inFile,ef)
        else:
            print("#WARNING: infile %s for %s not found"%(inFile,ef))
    generateEmbedded(filedefs,os.path.join(outPath(),EMBEDDED_INCLUDE))
    generateFile(os.path.join(basePath(),CFG_FILE),os.path.join(outPath(),CFG_INCLUDE),generateCfg)
    generateFile(os.path.join(basePath(),XDR_FILE),os.path.join(outPath(),XDR_INCLUDE),generateXdrMappings)
    version="dev"+datetime.now().strftime("%Y%m%d")
    env.Append(CPPDEFINES=[('GWDEVVERSION',version)])

prebuild(env)
