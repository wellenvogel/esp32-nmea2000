How to build the bundled esp tool for windows
=============================================
(1) install python 3 on windows, add to path
(2) pip install pyinstaller
(3) create an empty dir, cd there
(4) get esptool.py (either pip install esptool, search for it or download from
    https://github.com/espressif/esptool - just esptool.py) or copy it from here
(5) run: pyinstaller -F esptool.py
    esptool.exe in dist\esptool

How to build the bundled flashtool.exe (on windows)
===================================================
(1) install python 3, add to path
(2) pip install pyinstaller
(3) pip install pyserial
(4) in this directory:
    pyinstaller --clean -F flashtool.py
    will create flashtool.exe in dist


How to run flashtool on Linux
=============================
(1) have python 3 installed
(2) install python3-serial as package __or__ sudo pip3 install pyserial
(3) install python3-tk as package __or__ sudo pip3 install tkinter
(4) ./flashtool.py



