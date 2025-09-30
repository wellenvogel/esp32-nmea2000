Import("env", "projenv")
import os
import glob
import shutil
import re

print("##post script running")
HDROFFSET=288
VERSIONOFFSET=16
NAMEOFFSET=48
MINSIZE=HDROFFSET+NAMEOFFSET+32
CHECKBYTES={
    0: 0xe9, #image magic
    288: 0x32, #app header magic
    289: 0x54,
    290: 0xcd,
    291: 0xab
}
def getString(buffer,offset,len):
    return buffer[offset:offset+len].rstrip(b'\0').decode('utf-8')

def getFirmwareInfo(imageFile):
    with open(imageFile,"rb") as ih:
        buffer=ih.read(MINSIZE)
        if len(buffer) != MINSIZE:
            raise Exception("invalid image file %s, to short",imageFile)
        for k,v in CHECKBYTES.items():
            if buffer[k] != v:
                raise Exception("invalid magic in %s at %d, expected %d got %d"
                    %(imageFile,k,v,buffer[k]))
        name=getString(buffer,HDROFFSET+NAMEOFFSET,32)      
        version=getString(buffer,HDROFFSET+VERSIONOFFSET,32)
    return (name,version)    

def post(source,target,env):
    #print(env.Dump())
    esptool=env.get('UPLOADER')
    uploaderflags=env.subst("${UPLOADERFLAGS}")
    base=env.subst("$PIOENV")
    appoffset=env.subst("$ESP32_APP_OFFSET")
    firmware=env.subst("$BUILD_DIR/${PROGNAME}.bin")
    (fwname,version)=getFirmwareInfo(firmware)
    fwname=re.sub(r"[^0-9A-Za-z_.-]*","",fwname)
    print("found fwname=%s, fwversion=%s"%(fwname,version))
    python=env.subst("$PYTHONEXE")
    print("base=%s,esptool=%s,appoffset=%s,uploaderflags=%s"%(base,esptool,appoffset,uploaderflags))
    chip="esp32"
    uploadparts=uploaderflags.split(" ")
    #currently hardcoded last 8 parameters...
    if len(uploadparts) < 6:
        print("uploaderflags does not have enough parameter")
        return
    for i in range(0,len(uploadparts)):
        if uploadparts[i]=="--chip":
            if i < (len(uploadparts) -1):
                chip=uploadparts[i+1]
    uploadfiles=uploadparts[-6:]
    for i in range(1,len(uploadfiles),2):
        if not os.path.isfile(uploadfiles[i]):
            print("file %s for combine not found"%uploadfiles[i])
            return
    offset=uploadfiles[0]
    #cleanup old versioned files
    outdir=env.subst("$BUILD_DIR")
    for f in glob.glob(os.path.join(outdir,base+"*.bin")):
        print("removing old file %s"%f)
        os.unlink(f)
    outfile=os.path.join(outdir,"%s-all.bin"%(base))
    cmd=[python,esptool,"--chip",chip,"merge_bin","--target-offset",offset,"-o",outfile]
    cmd+=uploadfiles
    cmd+=[appoffset,firmware]        
    print("running %s"%" ".join(cmd))
    env.Execute(" ".join(cmd),"#testpost")
    ofversion="-"+version
    versionedFile=os.path.join(outdir,"%s%s-update.bin"%(fwname,ofversion))
    shutil.copyfile(firmware,versionedFile)
    print(f"wrote {versionedFile}")
    versioneOutFile=os.path.join(outdir,"%s%s-all.bin"%(fwname,ofversion))
    shutil.copyfile(outfile,versioneOutFile)
    print(f"wrote {versioneOutFile}")
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.bin",
    post
)