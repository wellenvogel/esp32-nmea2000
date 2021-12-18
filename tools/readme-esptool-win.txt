Hwo to build the bundled esp tool for windows
=============================================
(1) install python 3 on windows, add to path
(2) pip install pyinstaller
(3) create an empty dir, cd there
(4) get esptool.py (either pip install esptool, search for it or download from 
    https://github.com/espressif/esptool - just esptool.py)
(5) run: pyinstaller -F esptool.py
    esptool.exe in dist\esptool
        
