#! /usr/bin/env python3
import tkinter as tk
from tkinter import ttk
import tkinter.font as tkFont
import serial.tools.list_ports

import builtins



oldprint=builtins.print
def print(*args, **kwargs):
    app.addText(args[0])


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
        frame.grid(row=0,column=0,sticky='news')
        #frame.configure(background='lightblue')
        frame.columnconfigure(0,weight=1)
        frame.columnconfigure(1, weight=3)
        tk.Label(frame,text="ESP32 NMEA2000 Flash Tool").grid(row=0,column=0,columnspan=2,sticky='ew')
        tk.Label(frame, text="Com Port").grid(row=1,column=0,sticky='ew')
        self.port=ttk.Combobox(frame)
        self.port.grid(row=1,column=1,sticky="ew")
        tk.Label(frame,text="Select Firmware").grid(row=2,column=0,sticky='ew')
        self.filename=tk.StringVar()
        tk.Entry(frame,textvariable=self.filename).grid(row=2,column=1,sticky='ew')
        tk.Label(frame,text="Address 0x10000").grid(row=3,column=0,columnspan=2,sticky='ew',pady=10)
        tk.Button(frame,text="Flash",command=self.buttonAction).grid(row=4,column=0,columnspan=2,pady=10)
        self.text_widget = tk.Text(frame)
        frame.rowconfigure(5,weight=1)
        self.text_widget.grid(row=5,column=0,columnspan=2,sticky='news')
        self.readDevices()

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
    def addText(self,text):
        self.text_widget.insert(tk.END,text+"\n")

    def buttonAction(self):
        self.text_widget.delete("1.0", "end")
        idx=self.port.current()
        if idx < 0:
            self.addText("ERROR: no com port selected")
            return
        port=self.serialDevices[idx]
        command=['--chip','ESP32','--port',port,'chip_id']
        print("run esptool: %s"%" ".join(command))
        try:
            esptool.main(command)
            print("esptool done")
        except Exception as e:
            print("Exception in esptool %s"%e)    
        

if __name__ == "__main__":
    root = tk.Tk()
    app = App(root)
    root.mainloop()
