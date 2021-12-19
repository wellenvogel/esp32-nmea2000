#! /usr/bin/env python3
import tkinter as tk
import tkinter.font as tkFont

import builtins
import io


oldprint=builtins.print
def print(*args, **kwargs):
    app.addText(args[0])


builtins.print=print
import esptool    

class App:
    def __init__(self, root):
        #setting title
        root.title("ESP32 NMEA2000 Flash Tool")
        root.geometry("800x600")
        root.resizable(width=True, height=True)
        root.configure(background='lightgrey')
        tk.Label(root,text="ESP32 NMEA2000 Flash Tool").grid(row=0,column=0,columnspan=2)
        tk.Label(root, text="Com Port").grid(row=1,column=0)
        self.port=tk.StringVar()
        tk.Entry(root,textvariable=self.port).grid(row=1,column=1)
        tk.Label(root,text="Select Firmware").grid(row=2,column=0)
        self.filename=tk.StringVar()
        tk.Entry(root,textvariable=self.filename).grid(row=2,column=1)
        tk.Label(root,text="Address 0x10000").grid(row=3,column=0,columnspan=2)
        tk.Button(root,text="Flash",command=self.buttonAction).grid(row=4,column=0,columnspan=2)
        self.text_widget = tk.Text(root, height=20, width=60)
        self.text_widget.grid(row=5,rowspan=10,column=0,columnspan=2)
    def addText(self,text):
        self.text_widget.insert(tk.END,text+"\n")

    def buttonAction(self):
        command=['--chip','ESP32','--port',self.port.get(),'chip_id']
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
