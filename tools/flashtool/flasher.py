try:
    import esptool
except:
    import flashtool.esptool as esptool
import os
VERSION="2.1"

class Flasher():
    def getVersion(self):
        return ("Version %s, esptool %s"%(VERSION,str(esptool.__version__)))

    UPDATE_ADDR = 0x10000
    HDROFFSET = 288
    VERSIONOFFSET = 16
    NAMEOFFSET = 48
    IDOFFSET=12 #2 byte chipid
    MINSIZE = HDROFFSET + NAMEOFFSET + 32
    CHECKBYTES = {
        288: 0x32,  # app header magic
        289: 0x54,
        290: 0xcd,
        291: 0xab
    }
    #flash addresses for full images based on chip id
    FLASH_ADDR={
        0: 0x1000,
        9: 0
    }
    def getString(self,buffer, offset, len):
        return buffer[offset:offset + len].rstrip(b'\0').decode('utf-8')
    def getFirmwareInfo(self,filename,isFull):
        with open(filename,"rb") as ih:
            buffer = ih.read(self.MINSIZE)
            if len(buffer) != self.MINSIZE:
                return self.setErr("invalid image file %s, to short"%filename)
            if buffer[0] != 0xe9:
                return self.setErr("invalid magic in file, expected 0xe9 got 0x%02x"%buffer[0])
            chipid= buffer[self.IDOFFSET]+256*buffer[self.IDOFFSET+1]
            flashoffset=self.FLASH_ADDR.get(chipid)
            if flashoffset is None:
                return self.setErr("unknown chip id in image %d",chipid);
            if isFull:
                offset=self.UPDATE_ADDR-flashoffset;
                offset-=self.MINSIZE
                ih.seek(offset,os.SEEK_CUR)
                buffer=ih.read(self.MINSIZE)
                if len(buffer) != self.MINSIZE:
                    return self.setErr("invalid image file %s, to short"%filename)
                if buffer[0] != 0xe9:
                    return self.setErr("invalid magic in file, expected 0xe9 got 0x%02x"%buffer[0])
            for k, v in self.CHECKBYTES.items():
                if buffer[k] != v:
                    return self.setErr("invalid magic at %d, expected %d got %d"
                                % (k+offset, v, buffer[k]))
            name = self.getString(buffer, self.HDROFFSET + self.NAMEOFFSET, 32)
            version = self.getString(buffer, self.HDROFFSET + self.VERSIONOFFSET, 32)
            chipid= buffer[self.IDOFFSET]+256*buffer[self.IDOFFSET+1]
            flashoffset=flashoffset if isFull else self.UPDATE_ADDR
            return {
                'error':False,
                'info':"%s:%s"%(name,version),
                'chipid':chipid,
                'flashbase':flashoffset
            }
    def setErr(self,err):
        return {'error':True,'info':err}
    def checkImageFile(self,filename,isFull):
        if not os.path.exists(filename):
            return self.setErr("file %s not found"%filename)
        return self.getFirmwareInfo(filename,isFull)
    
    def checkSettings(self,port,fileName,isFull):
        if port is None:
            print("ERROR: no com port selected")
            return
        if fileName is None or fileName == '':
            print("ERROR: no filename selected")
            return
        info = self.checkImageFile(fileName, isFull)
        if info['error']:
            print("ERROR: %s" % info['info'])
            return
        return {'fileName': fileName,'port':port,'isFull':isFull,'info':info}
    def runEspTool(self,command):
        print("run esptool: %s" % " ".join(command))
        try:
            esptool.main(command)
            print("esptool done")
            return True
        except Exception as e:
            print("Exception in esptool %s" % e)
    def verifyChip(self,param):
        if not param:
            print("check failed")
            return
        imageChipId=param['info']['chipid']
        try:
            chip=esptool.ESPLoader.detect_chip(param['port'],trace_enabled=True)
            print("Detected chip %s, id=%d"%(chip.CHIP_NAME,chip.IMAGE_CHIP_ID))
            if (chip.IMAGE_CHIP_ID != imageChipId):
                print("##Error: chip id in image %d does not match detected chip"%imageChipId)
                return
            print("Checks OK")
            param['chipname']=chip.CHIP_NAME
        except Exception as e:
            print("ERROR: ",str(e))
        return param
    def runCheck(self,port,fileName,isFull):
        param = self.checkSettings(port,fileName,isFull)
        if not param:
            return
        print("Settings OK")
        param=self.verifyChip(param)
        if not param:
            print("Check Failed")
            return
        print("flashbase=0x%x"%param['info']['flashbase'])
        return param
    def runFlash(self,param):
        if not param:
            return
        if param['isFull']:
            command=['--chip',param['chipname'],'--port',param['port'],'write_flash',str(param['info']['flashbase']),param['fileName']]
            self.runEspTool(command)
        else:
            command=['--chip',param['chipname'],'--port',param['port'],'erase_region','0xe000','0x2000']
            self.runEspTool(command)
            command = ['--chip', param['chipname'], '--port', param['port'], 'write_flash', str(param['info']['flashbase']), param['fileName']]
            self.runEspTool(command)