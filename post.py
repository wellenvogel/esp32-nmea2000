Import("env", "projenv")
import os

print("##post script running")

def post(source,target,env):
    #print(env.Dump())
    esptool=env.get('UPLOADER')
    uploaderflags=env.subst("${UPLOADERFLAGS}")
    base=env.subst("$PIOENV")
    appoffset=env.subst("$ESP32_APP_OFFSET")
    firmware=env.subst("$BUILD_DIR/${PROGNAME}.bin")
    python=env.subst("$PYTHONEXE")
    print("base=%s,esptool=%s,appoffset=%s,uploaderflags=%s"%(base,esptool,appoffset,uploaderflags))
    uploadparts=uploaderflags.split(" ")
    #currently hardcoded last 8 parameters...
    if len(uploadparts) < 6:
        print("uploaderflags does not have enough parameter")
        return
    uploadfiles=uploadparts[-6:]
    for i in range(1,len(uploadfiles),2):
        if not os.path.isfile(uploadfiles[i]):
            print("file %s for combine not found"%uploadfiles[i])
            return
    offset=uploadfiles[0]
    outfile=os.path.join(env.subst("$BUILD_DIR"),base+"-all.bin")
    cmd=[python,esptool,"--chip","esp32","merge_bin","--target-offset",offset,"-o",outfile]
    cmd+=uploadfiles
    cmd+=[appoffset,firmware]        
    print("running %s"%" ".join(cmd))
    env.Execute(" ".join(cmd),"#testpost")
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.bin",
    post
)