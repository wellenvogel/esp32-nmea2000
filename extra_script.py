print("running extra...")
import gzip
import shutil
import os
import sys
import inspect
import json
import glob
from datetime import datetime
import re
import pprint
from platformio.project.config import ProjectConfig


Import("env")
#print(env.Dump())
OWN_FILE="extra_script.py"
GEN_DIR='lib/generated'
CFG_FILE='web/config.json'
XDR_FILE='web/xdrconfig.json'
CFG_INCLUDE='GwConfigDefinitions.h'
CFG_INCLUDE_IMPL='GwConfigDefImpl.h'
XDR_INCLUDE='GwXdrTypeMappings.h'
TASK_INCLUDE='GwUserTasks.h'
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
            own=os.path.join(basePath(),OWN_FILE)
            if os.path.exists(own):
                owntime=os.path.getmtime(own)
                if owntime > otime:
                    return False
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
                return False
    print("#generating %s"%fileName)
    with open(fileName,"w") as oh:
        oh.write(data)
    return True    

def mergeConfig(base,other):
    for bdir in other:
        cname=os.path.join(bdir,"config.json")
        if os.path.exists(cname):
            print("merge config %s"%cname)
            with open(cname,'rb') as ah:
                merge=json.load(ah)
                base=base+merge
    return base

def generateMergedConfig(inFile,outFile,addDirs=[]):
    if not os.path.exists(inFile):
        raise Exception("unable to read cfg file %s"%inFile)
    data=""
    with open(inFile,'rb') as ch:    
        config=json.load(ch)
        config=mergeConfig(config,addDirs)
        data=json.dumps(config,indent=2)
        writeFileIfChanged(outFile,data)

def generateCfg(inFile,outFile,impl):
    if not os.path.exists(inFile):
        raise Exception("unable to read cfg file %s"%inFile)
    data=""
    with open(inFile,'rb') as ch:    
        config=json.load(ch)     
        data+="//generated from %s\n"%inFile
        l=len(config)
        idx=0
        if not impl:
            data+='#include "GwConfigItem.h"\n'
            data+='class GwConfigDefinitions{\n'
            data+='  public:\n'
            data+='  int getNumConfig() const{return %d;}\n'%(l)
            for item in config:
                n=item.get('name')
                if n is None:
                    continue
                if len(n) > 15:
                    raise Exception("%s: config names must be max 15 caracters"%n)
                data+='  static constexpr const char* %s="%s";\n'%(n,n)
            data+="};\n"
        else:
            data+='void GwConfigHandler::populateConfigs(GwConfigInterface **config){\n'
            for item in config:
                name=item.get('name')
                if name is None:
                    continue
                data+='  configs[%d]=\n'%(idx)
                idx+=1
                secret="false";
                if item.get('type') == 'password':
                    secret="true"
                data+="    #undef __CFGMODE\n"
                data+="    #ifdef CFGMODE_%s\n"%(name)
                data+="      #define __CFGMODE CFGMODE_%s\n"%(name)
                data+="    #else\n"
                data+="      #define __CFGMODE GwConfigInterface::NORMAL\n"
                data+="    #endif\n"    
                data+="    #ifdef CFGDEFAULT_%s\n"%(name)
                data+="     new GwConfigInterface(%s,CFGDEFAULT_%s,%s,__CFGMODE)\n"%(name,name,secret)
                data+="    #else\n"
                data+="     new GwConfigInterface(%s,\"%s\",%s,__CFGMODE)\n"%(name,item.get('default'),secret)
                data+="    #endif\n"
                data+=";\n"
            data+='}\n'  
            for item in config:
                name=item.get('name')
                if name is None:
                    continue
                data+="#ifdef CFGMODE_%s\n"%(name)
                data+=" __MSG(\"CFGMODE_%s=\" __XSTR(CFGMODE_%s))\n"%(name,name)
                data+="#endif\n"
                data+="#ifdef CFGDEFAULT_%s\n"%(name)
                data+=" __MSG(\"CFGDEFAULT_%s=\" CFGDEFAULT_%s)\n"%(name,name)
                data+="#endif\n"
    writeFileIfChanged(outFile,data)    
                    
def labelFilter(label):
    return re.sub("[^a-zA-Z0-9]","",re.sub("\([0-9]*\)","",label))    
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
            oh.write("   new GwXDRTypeMapping(%d,0,%d) /*%s*/"%(cid,tc,cat))
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
            oh.write("   new GwXDRTypeMapping(%d,%d,%d) /*%s:%s*/"%(cid,id,tc,cat,l))
    oh.write("\n")
    oh.write("};\n")
    for cat in jdoc:
        item=jdoc[cat]
        cid=item.get('id')
        if cid is None:
            continue
        selectors=item.get('selector')
        if selectors is not None:
            for selector in selectors:
                label=selector.get('l')
                value=selector.get('v')
                if label is not None and value is not None:
                    label=labelFilter(label)
                    define=("GWXDRSEL_%s_%s"%(cat,label)).upper()
                    oh.write("    #define %s %s\n"%(define,value))
        fields=item.get('fields')
        if fields is not None:
            idx=0
            for field in fields:
                v=field.get('v')
                if v is None:
                    v=idx
                else:
                    v=int(v)
                label=field.get('l')
                if v is not None and label is not None:
                    define=("GWXDRFIELD_%s_%s"%(cat,labelFilter(label))).upper();
                    oh.write("    #define %s %s\n"%(define,str(v)))
                idx+=1


userTaskDirs=[]

def getUserTaskDirs():
    rt=[]
    taskdirs=glob.glob(os.path.join( basePath(),'lib','*task*'))
    for task in taskdirs:
        rt.append(task)
    return rt

def checkAndAdd(file,names,ilist):
    if not file.endswith('.h'):
        return
    match=False
    for cmp in names:
        #print("##check %s<->%s"%(f.lower(),cmp))
        if file.lower() == cmp:
            match=True
    if not match:
        return
    ilist.append(file) 
def genereateUserTasks(outfile):
    includes=[]
    for task in userTaskDirs:
        #print("##taskdir=%s"%task)
        base=os.path.basename(task)
        includeNames=[base.lower()+".h",'gw'+base.lower()+'.h']
        for f in os.listdir(task):
            checkAndAdd(f,includeNames,includes)
    includeData=""
    for i in includes:
        print("#task include %s"%i)
        includeData+="#include <%s>\n"%i
    writeFileIfChanged(outfile,includeData)            

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
    if (fn.endswith('css')):    
        return "text/css"
    return "application/octet-stream"


def getLibs():
    base=os.path.join(basePath(),"lib")
    rt=[]
    for sd in os.listdir(base):
        if sd == '..':
            continue
        if sd == '.':
            continue
        fn=os.path.join(base,sd)
        if os.path.isdir(fn):
            rt.append(sd)
    EXTRAS=['generated']
    for e in EXTRAS:
        if not e in rt:
            rt.append(e)
    return rt

OWNLIBS=getLibs()+["FS","WiFi"]
GLOBAL_INCLUDES=[]

def handleDeps(env):
    #overwrite the GetProjectConfig
    #to inject all our libs
    oldGetProjectConfig=env.GetProjectConfig    
    def GetProjectConfigX(env):        
        rt=oldGetProjectConfig()
        cenv="env:"+env['PIOENV']
        libs=[]
        for section,options in rt.as_tuple():
            if section == cenv:
                for key,values in options:
                    if key == 'lib_deps':
                        libs=values
    
        mustUpdate=False
        for lib in OWNLIBS:
            if not lib in libs:
                libs.append(lib)
                mustUpdate=True
        if mustUpdate:
            update=[(cenv,[('lib_deps',libs)])]
            rt.update(update)
        return rt
    env.AddMethod(GetProjectConfigX,"GetProjectConfig")
    #store the list of all includes after we resolved
    #the dependencies for our main project
    #we will use them for all compilations afterwards
    oldLibBuilder=env.ConfigureProjectLibBuilder
    def ConfigureProjectLibBuilderX(env):
        global GLOBAL_INCLUDES
        project=oldLibBuilder()
        #print("##ConfigureProjectLibBuilderX")
        #pprint.pprint(project)
        if project.depbuilders:
            #print("##depbuilders %s"%",".join(map(lambda x: x.path,project.depbuilders)))
            for db in project.depbuilders:
                idirs=db.get_include_dirs()
                for id in idirs:
                    if not id in GLOBAL_INCLUDES:
                        GLOBAL_INCLUDES.append(id)
        return project
    env.AddMethod(ConfigureProjectLibBuilderX,"ConfigureProjectLibBuilder")
    def injectIncludes(env,node):
        return env.Object(
            node,
            CPPPATH=env["CPPPATH"]+GLOBAL_INCLUDES
        )
    env.AddBuildMiddleware(injectIncludes)


def prebuild(env):
    global userTaskDirs
    print("#prebuild running")
    if not checkDir():
        sys.exit(1)
    ldf_mode=env.GetProjectOption("lib_ldf_mode")
    if ldf_mode == 'off':
        print("##ldf off - own dependency handling")
        handleDeps(env)
    userTaskDirs=getUserTaskDirs()
    mergedConfig=os.path.join(outPath(),os.path.basename(CFG_FILE))
    generateMergedConfig(os.path.join(basePath(),CFG_FILE),mergedConfig,userTaskDirs)
    compressFile(mergedConfig,mergedConfig+".gz")
    generateCfg(mergedConfig,os.path.join(outPath(),CFG_INCLUDE),False)
    generateCfg(mergedConfig,os.path.join(outPath(),CFG_INCLUDE_IMPL),True)
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
    genereateUserTasks(os.path.join(outPath(), TASK_INCLUDE))
    generateFile(os.path.join(basePath(),XDR_FILE),os.path.join(outPath(),XDR_INCLUDE),generateXdrMappings)
    version="dev"+datetime.now().strftime("%Y%m%d")
    env.Append(CPPDEFINES=[('GWDEVVERSION',version)])

def cleangenerated(source, target, env):
    od=outPath()
    if os.path.isdir(od):
        print("#cleaning up %s"%od)
        for f in os.listdir(od):
            if f == "." or f == "..":
                continue
            fn=os.path.join(od,f)
            os.unlink(f)


print("#prescript...")
prebuild(env)
board="PLATFORM_BOARD_%s"%env["BOARD"].replace("-","_").upper()
print("Board=#%s#"%board)
print("BuildFlags=%s"%(" ".join(env["BUILD_FLAGS"])))
env.Append(
    LINKFLAGS=[ "-u", "custom_app_desc" ],
    CPPDEFINES=[(board,"1")]
)
#script does not run on clean yet - maybe in the future
env.AddPostAction("clean",cleangenerated)
