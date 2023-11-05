#! /usr/bin/env python3
import subprocess
import sys

try:
    import serial
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", 'pyserial'])
finally:
    import serial

import tkinter as tk
from tkinter import ttk
import tkinter.font as tkFont

import os
import serial.tools.list_ports
from tkinter import filedialog as FileDialog

import builtins
import esptool

VERSION="Version 2.0"
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
        chip=esptool.ESPLoader.detect_chip(param['port'])
        print("Detected chip %s, id=%d"%(chip.CHIP_NAME,chip.IMAGE_CHIP_ID))
        if (chip.IMAGE_CHIP_ID != imageChipId):
            print("##Error: chip id in image %d does not match detected chip"%imageChipId)
            return
        print("Checks OK")
        param['chipname']=chip.CHIP_NAME
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

def main():

    oldprint=builtins.print
    def print(*args, **kwargs):
        app.addText(*args,**kwargs)


    builtins.print=print
    import esptool

    class App:
        def __init__(self, root):
            self.flasher=Flasher()
            root.title("ESP32 NMEA2000 Flash Tool")
            root.geometry("800x600")
            root.resizable(width=True, height=True)
            root.configure(background='lightgrey')
            root.columnconfigure(0, weight=1)
            root.rowconfigure(0, weight=1)
            frame=tk.Frame(root)
            row=0
            frame.grid(row=0,column=0,sticky='news',padx=10,pady=5)
            DUMMY = "prevent to handled as virus"
            #frame.configure(background='lightblue')
            frame.columnconfigure(0,weight=1)
            frame.columnconfigure(1, weight=3)
            tk.Label(frame,text="ESP32 NMEA2000 Flash Tool").grid(row=row,column=0,columnspan=2,sticky='ew')
            row+=1
            tk.Label(frame, text=self.flasher.getVersion()).grid(row=row,column=0,columnspan=2,sticky="ew",pady=10)
            row+=1
            self.mode=tk.IntVar()
            self.mode.set(1)
            rdFrame=tk.Frame(frame)
            rdFrame.grid(row=row,column=1,sticky="ew",pady=20)
            tk.Radiobutton(rdFrame,text="Initial Flash",value=1,variable=self.mode,command=self.changeMode).grid(row=0,column=0)
            tk.Radiobutton(rdFrame, text="Update Flash", value=2, variable=self.mode,command=self.changeMode).grid(row=0,column=1)
            row+=1
            tk.Label(frame, text="Com Port").grid(row=row,column=0,sticky='ew')
            ttk.Style().configure("TCombobox",padding=8,arrowsize=28)
            ttk.Style().configure("TEntry", padding=8)
            self.port=ttk.Combobox(frame)
            self.port.grid(row=row,column=1,sticky="ew",pady=5)
            row+=1
            tk.Label(frame,text="Select Firmware").grid(row=row,column=0,sticky='ew')
            self.filename=tk.StringVar()
            fn=ttk.Entry(frame,textvariable=self.filename)
            fn.grid(row=row,column=1,sticky='ew',pady=5)
            fn.bind("<1>",self.fileDialog)
            row+=1
            self.fileInfo=tk.StringVar()
            tk.Label(frame,textvariable=self.fileInfo).grid(row=row,column=0,columnspan=2,sticky="ew")
            row+=1
            self.flashInfo=tk.StringVar()
            self.flashInfo.set("Full Flash")
            tk.Label(frame,textvariable=self.flashInfo).grid(row=row,column=0,columnspan=2,sticky='ew',pady=10)
            row+=1
            btFrame=tk.Frame(frame)
            btFrame.grid(row=row,column=0,columnspan=2,pady=15)
            self.actionButtons=[]
            bt=tk.Button(btFrame,text="Check",command=self.buttonCheck)
            bt.grid(row=0,column=0)
            self.actionButtons.append(bt)
            bt=tk.Button(btFrame, text="Flash", command=self.buttonFlash)
            bt.grid(row=0, column=1)
            self.actionButtons.append(bt)
            self.cancelButton=tk.Button(btFrame,text="Cancel",state=tk.DISABLED,command=self.buttonCancel)
            self.cancelButton.grid(row=0,column=2)
            row+=1
            self.text_widget = tk.Text(frame)
            frame.rowconfigure(row,weight=1)
            self.text_widget.grid(row=row,column=0,columnspan=2,sticky='news')
            self.readDevices()
            self.interrupt=False

        def updateFlashInfo(self):
            if self.mode.get() == 1:
                #full
                self.flashInfo.set("Full Flash")
            else:
                self.flashInfo.set("Erase(otadata): 0xe000...0xffff, Address 0x10000")
        def changeMode(self):
            m=self.mode.get()
            self.updateFlashInfo()
            self.filename.set('')
            self.fileInfo.set('')
        def fileDialog(self,ev):
            fn=FileDialog.askopenfilename()
            if fn:
                self.filename.set(fn)
                info=self.flasher.checkImageFile(fn,self.mode.get() == 1)
                if info['error']:
                    self.fileInfo.set("***ERROR: %s"%info['info'])
                else:
                    self.fileInfo.set(info['info'])
        def readDevices(self):
            self.serialDevices=[]
            names=[]
            for dev in serial.tools.list_ports.comports(False):
                self.serialDevices.append(dev.device)
                if dev.description != 'n/a':
                    label=dev.description+"("+dev.device+")"
                else:
                    label=dev.name+"("+dev.device+")"
                names.append(label)
            self.port.configure(values=names)
        def addText(self,*args,**kwargs):
            first=True
            for k in args:
                self.text_widget.insert(tk.END,k)
                if not first:
                    self.text_widget.insert(tk.END, ',')
                first=False
            if kwargs.get('end') is None:
                self.text_widget.insert(tk.END,"\n")
            else:
                self.text_widget.insert(tk.END,kwargs.get('end'))
            self.text_widget.see('end')
            root.update()
            if self.interrupt:
                self.interrupt=False
                raise Exception("User cancel")

        def runCheck(self):
            self.text_widget.delete("1.0", "end")
            idx = self.port.current()
            isFull = self.mode.get() == 1
            if idx < 0:
                self.addText("ERROR: no com port selected")
                return
            port = self.serialDevices[idx]
            fn = self.filename.get()
            param = self.flasher.runCheck(port,fn,isFull)
            return param

        def runFlash(self,param):
            for b in self.actionButtons:
                b.configure(state=tk.DISABLED)
            self.cancelButton.configure(state=tk.NORMAL)
            root.update()
            root.update_idletasks()
            self.flasher.runFlash(param)
            for b in self.actionButtons:
                b.configure(state=tk.NORMAL)
            self.cancelButton.configure(state=tk.DISABLED)
        
        def buttonCheck(self):
            param = self.runCheck()
            
            

        def buttonFlash(self):
            param=self.runCheck()
            if not param:
                return
            self.runFlash(param)
            
        def buttonCancel(self):
            self.interrupt=True

    root = tk.Tk()
    app = App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
