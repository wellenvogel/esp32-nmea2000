#! /usr/bin/env python3
import tkinter as tk
from tkinter import ttk
import tkinter.font as tkFont

import os
import serial.tools.list_ports
from tkinter import filedialog as FileDialog

import builtins

VERSION="Version 1.1, esptool 3.2"

oldprint=builtins.print
def print(*args, **kwargs):
    app.addText(*args,**kwargs)


builtins.print=print
import esptool    

class App:
    def __init__(self, root):
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
        tk.Label(frame, text=VERSION).grid(row=row,column=0,columnspan=2,sticky="ew",pady=10)
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
        self.flashInfo.set("Address 0x1000")
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
            self.flashInfo.set("Address 0x1000")
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
            info=self.checkImageFile(fn,self.mode.get() == 1)
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

    FULLOFFSET=61440
    HDROFFSET = 288
    VERSIONOFFSET = 16
    NAMEOFFSET = 48
    MINSIZE = HDROFFSET + NAMEOFFSET + 32
    CHECKBYTES = {
        0: 0xe9,  # image magic
        288: 0x32,  # app header magic
        289: 0x54,
        290: 0xcd,
        291: 0xab
    }

    def getString(self,buffer, offset, len):
        return buffer[offset:offset + len].rstrip(b'\0').decode('utf-8')

    def getFirmwareInfo(self,ih,imageFile,offset):
        buffer = ih.read(self.MINSIZE)
        if len(buffer) != self.MINSIZE:
            return self.setErr("invalid image file %s, to short"%imageFile)
        for k, v in self.CHECKBYTES.items():
            if buffer[k] != v:
                return self.setErr("invalid magic at %d, expected %d got %d"
                                % (k+offset, v, buffer[k]))
        name = self.getString(buffer, self.HDROFFSET + self.NAMEOFFSET, 32)
        version = self.getString(buffer, self.HDROFFSET + self.VERSIONOFFSET, 32)
        return {'error':False,'info':"%s:%s"%(name,version)}

    def setErr(self,err):
        return {'error':True,'info':err}
    def checkImageFile(self,filename,isFull):
        if not os.path.exists(filename):
            return self.setErr("file %s not found"%filename)
        with open(filename,"rb") as fh:
            offset=0
            if isFull:
                b=fh.read(1)
                if len(b) != 1:
                    return self.setErr("unable to read header")
                if b[0] != 0xe9:
                    return self.setErr("invalid magic in file, expected 0xe9 got 0x%02x"%b[0])
                st=fh.seek(self.FULLOFFSET)
                offset=self.FULLOFFSET
            return self.getFirmwareInfo(fh,filename,offset)

    def runCheck(self):
        self.text_widget.delete("1.0", "end")
        idx = self.port.current()
        isFull = self.mode.get() == 1
        if idx < 0:
            self.addText("ERROR: no com port selected")
            return
        port = self.serialDevices[idx]
        fn = self.filename.get()
        if fn is None or fn == '':
            self.addText("ERROR: no filename selected")
            return
        info = self.checkImageFile(fn, isFull)
        if info['error']:
            print("ERROR: %s" % info['info'])
            return
        return {'port':port,'isFull':isFull}

    def runEspTool(self,command):
        for b in self.actionButtons:
            b.configure(state=tk.DISABLED)
        self.cancelButton.configure(state=tk.NORMAL)
        print("run esptool: %s" % " ".join(command))
        root.update()
        root.update_idletasks()
        try:
            esptool.main(command)
            print("esptool done")
        except Exception as e:
            print("Exception in esptool %s" % e)
        for b in self.actionButtons:
            b.configure(state=tk.NORMAL)
        self.cancelButton.configure(state=tk.DISABLED)
    def buttonCheck(self):
        param = self.runCheck()
        if not param:
            return
        print("Settings OK")
        command = ['--chip', 'ESP32', '--port', param['port'], 'chip_id']
        self.runEspTool(command)

    def buttonFlash(self):
        param=self.runCheck()
        if not param:
            return
        if param['isFull']:
            command=['--chip','ESP32','--port',param['port'],'write_flash','0x1000',self.filename.get()]
            self.runEspTool(command)
        else:
            command=['--chip','ESP32','--port',param['port'],'erase_region','0xe000','0x2000']
            self.runEspTool(command)
            command = ['--chip', 'ESP32', '--port', param['port'], 'write_flash', '0x10000', self.filename.get()]
            self.runEspTool(command)


    def buttonCancel(self):
        self.interrupt=True


if __name__ == "__main__":
    root = tk.Tk()
    app = App(root)
    root.mainloop()
