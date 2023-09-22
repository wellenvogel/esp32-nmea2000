import {XtermOutputHandler} from "./installUtil.js";
import ESPInstaller from "./installUtil.js";
import { addEl, getParam, setValue, setVisible } from "./helper.js";
import * as zip from "https://cdn.jsdelivr.net/npm/@zip.js/zip.js@2.7.29/+esm";
(function(){

    const UPDATE_START=65536;

    //taken from index.js
    const HDROFFSET = 288;
    const VERSIONOFFSET = 16;
    const NAMEOFFSET = 48;
    const MINSIZE = HDROFFSET + NAMEOFFSET + 32;
    const CHIPIDOFFSET=12; //2 bytes chip id here
    const imageMagic=0xe9; //at byte 0
    const imageCheckBytes = {
        288: 0x32, //app header magic
        289: 0x54,
        290: 0xcd,
        291: 0xab
    };
    /**
     * map of our known chip ids to flash starts for full images
     * see https://github.com/espressif/esptool-js/tree/main/src/targets
     * IMAGE_CHIP_ID, BOOTLOADER_FLASH_OFFSET
     * 0 - esp32 - starts at 0x1000
     * 9 - esp32s3 - starts at 0
     */
    const FLASHSTART={
        0:0x1000, //ESP32
        9:0,      //ESP32S3
        2:0x1000, //ESP32S2
        5:0       //ESP32C3
    };
    const decodeFromBuffer=(buffer, start, length)=>{
        while (length > 0 && buffer.charCodeAt(start + length - 1) == 0) {
            length--;
        }
        if (length <= 0) return "";
        return buffer.substr(start,length);
    }
    const getChipId=(buffer)=>{
        if (buffer.length < CHIPIDOFFSET+2) return -1;
        return buffer.charCodeAt(CHIPIDOFFSET)+256*buffer.charCodeAt(CHIPIDOFFSET+1);
    }
    /**
     * 
     * @param {string} content the content to be checked
     */
    const checkImage = (content,isFull) => {
        let prfx=isFull?"full":"update";
        let startOffset=0;
        let flashStart=UPDATE_START;
        let chipId=getChipId(content);
        if (isFull){
            if (chipId < 0) throw new Error(prfx+"image: no valid chip id found");
            let flashStart=FLASHSTART[chipId];
            if (flashStart === undefined) throw new Error(prfx+"image: unknown chip id "+chipId);
            startOffset=UPDATE_START-flashStart;
        }
        if (content.length < (MINSIZE+startOffset)) {
            throw new Error(prfx+"image to small, only " + content.length + " expected " + (MINSIZE+startOffset));
        }
        if (content.charCodeAt(0) != imageMagic){
            throw new Error("no image magic "+imageMagic+" at start of "+prfx+"image");
        }
        for (let idx in imageCheckBytes) {
            let cb=content.charCodeAt(parseInt(idx)+startOffset);
            if (cb != imageCheckBytes[idx]) {
                throw new Error(prfx+"image: missing magic byte at position " + idx + ", expected " +
                    imageCheckBytes[idx] + ", got " + cb);
            }
        }
        let version = decodeFromBuffer(content, startOffset+ HDROFFSET + VERSIONOFFSET, 32);
        let fwtype = decodeFromBuffer(content, startOffset+ HDROFFSET + NAMEOFFSET, 32);
        let rt = {
            fwtype: fwtype,
            version: version,
            chipId:chipId,
            flashStart: flashStart
        };
        return rt;
    }
    const readFile=(file)=>{
        return new Promise((resolve,reject)=>{
            let reader = new FileReader();
            reader.addEventListener('load', function (e) {
                resolve(e.target.result);
                
            });
            reader.readAsBinaryString(file);
        });
    }
    const checkImageFile=(file,isFull)=>{
        let minSize=MINSIZE+(isFull?(UPDATE_START-FULL_START):0);
        return new Promise(function (resolve, reject) {
            if (!file) reject("no file");
            if (file.size < minSize) reject("file is too small");
            let slice = file.slice(0, minSize);
            let reader = new FileReader();
            reader.addEventListener('load', function (e) {
                let content = e.target.result;
                try{
                    let rt=checkImage(content,isFull);
                    resolve(rt);
                }
                catch (e){
                    reject(e);    
                }
            });
            reader.readAsBinaryString(slice);
        });
    }
    let espLoaderTerminal;
    let espInstaller;
    let releaseData={};
    let showConsole;
    let hideConsole;
    const enableConsole=(enable,disableBoth)=>{
        if (showConsole) showConsole.disabled=!enable || disableBoth;
        if (hideConsole) hideConsole.disabled=enable || disableBoth;
    }
    const showError=(txt)=>{
        let hFrame=document.querySelector('.heading');
        if (hFrame){
            hFrame.textContent=txt;
            hFrame.classList.add("error");
        }
        else{
            alert(txt);
        }
    }
    const buildHeading=(info,element)=>{
        let hFrame=document.querySelector(element||'.heading');
        if (! hFrame) return;
        hFrame.textContent='';
        let h=addEl('h2',undefined,hFrame,`ESP32 Install ${info}`)
    }
    const checkChip= async (chipId,data,isFull)=>{
        let info=checkImage(data,isFull);
        if (info.chipId != chipId){
            let res=confirm("different chip signatures - image("+chipId+"), chip ("+info.chipId+")\nUse this image any way?");
            if (! res) throw new Error("user abort");
        }
        return info;
    }
    const baudRates=[1200,
        2400,
        4800,
        9600,
        14400,
        19200,
        28800,
        38400,
        57600,
        115200,
        230400,
        460800];
    const buildConsoleButtons=(element)=>{
        let bFrame=document.querySelector(element||'.console');
        if (! bFrame) return;
        bFrame.textContent='';
        let cLine=addEl('div','buttons',bFrame);
        let bSelect=addEl('select','consoleBaud',cLine);
        baudRates.forEach((baud)=>{
            let v=addEl('option',undefined,bSelect,baud+'');
            v.setAttribute('value',baud);
        });
        bSelect.value=115200;
        showConsole=addEl('button','showConsole',cLine,'ShowConsole');
        showConsole.addEventListener('click',async()=>{
            enableConsole(false);
            await espInstaller.startConsole(bSelect.value);
        })
        hideConsole=addEl('button','hideConsole',cLine,'HideConsole');
        hideConsole.addEventListener('click',async()=>{
            await espInstaller.stopConsole();
            enableConsole(true);
        })
    }
    const handleLocalFile= async (file)=>{
        setValue('content','');
        showLoading("loading "+file.name);
        try {
            if (file.name.match(/.zip$/)) {
                //zip
                await handleZip(file);
            }
            else {
                if (! file.name.match(/\.bin$/)){
                    throw new Error("only .zip or .bin");
                }
                let data=await readFile(file);
                let isFull=false;
                let info;
                try{
                    info=checkImage(data,true);
                    isFull=true;
                }catch (e){
                    try{
                        info=checkImage(data);
                    }
                    catch(x){
                        throw new Error(file.name+" is no image: "+x);
                    }
                }
                if (isFull){
                    buildCustomButtons("dummy",undefined,data,file.name,"Local");
                }
                else{
                    buildCustomButtons("dummy",data,undefined,file.name,"Local");
                }
            }
        } catch (e) {
            alert(e);
        }
        showLoading();
    }
    const buildUploadButtons= (element)=>{
        let bFrame=document.querySelector(element||'.upload');
        if (! bFrame) return;
        bFrame.textContent='';
        let it=addEl('div','item',bFrame);
        addEl('div','version',it,`Local File`);
        let cLine=addEl('div','buttons',it);
        let fi=addEl('input','uploadFile',cLine);
        fi.setAttribute('type','file');
        fi.addEventListener('change',async (ev)=>{
            let files=ev.target.files;
            if (files.length < 1) return;
            await handleLocalFile(files[0]);
        });
        let bt=addEl('button','uploadButton',cLine,'upload');
        bt.addEventListener('click',()=>{
            fi.click();
        });
    }
    const buildButtons=(user,repo,element)=>{
        let bFrame=document.querySelector(element||'.content');
        if (! bFrame) return;
        bFrame.textContent='';
        if (!releaseData.assets) return;
        let version=releaseData.name;
        if (! version){
            alert("no version found in release data");
            return;
        }
        addEl('div','version',bFrame,`Prebuild: ${version}`);
        let items={};
        releaseData.assets.forEach((asset)=>{
            let name=asset.name;
            let base=name.replace(/-all\.bin/,'').replace(/-update\.bin/,'');
            if (items[base] === undefined){
                items[base]={};
            }
            let item=items[base];
            item.label=base.replace(/-[0-9][0-9]*/,'');
            if (name.match(/-update\./)){
                item.update=name;
            }
            else{
                item.basic=name;
            }
        });
        for (let k in items){
            let item=items[k];
            let line=addEl('div','item',bFrame);
            addEl('div','itemTitle',line,item.label);
            let btLine=addEl('div','buttons',line);
            let tb=addEl('button','installButton',btLine,'Initial');
            tb.addEventListener('click',async ()=>{
                enableConsole(false,true);
                await espInstaller.installClicked(
                    true,
                    user,
                    repo,
                    version,
                    item.basic,
                    checkChip
                )
                enableConsole(true);
            });
            tb=addEl('button','installButton',btLine,'Update');
            tb.addEventListener('click',async ()=>{
                enableConsole(false,true);
                await espInstaller.installClicked(
                    false,
                    user,
                    repo,
                    version,
                    item.update,
                    checkChip
                )
                enableConsole(true);
            });
        }

    }
    const buildCustomButtons = (name, updateData, fullData,info,title,element) => {
        let bFrame = document.querySelector(element || '.content');
        if (!bFrame) return;
        if (fullData === undefined && updateData === undefined) return;
        let version;
        let vinfo;
        let uinfo;
        if (fullData !== undefined){
            vinfo=checkImage(fullData,true);
            version=vinfo.version;
        }
        if (updateData !== undefined){
            uinfo=checkImage(updateData);
            if (version !== undefined){
                if (uinfo.version != version){
                    throw new Error("different versions in full("+version+") and update("+uinfo.version+") image");
                }
            }
            else{
                version=uinfo.version;
            }
        }
        bFrame.textContent = '';
        let item=addEl('div','item',bFrame);
        addEl('div', 'version', item, title+" "+version);
        if (info){
            addEl('div','version',item,info);
        }
        let btLine = addEl('div', 'buttons', item);
        let tb;
        if (fullData !== undefined) {
            tb = addEl('button', 'installButton', btLine, 'Initial');
            tb.addEventListener('click', async () => {
                enableConsole(false, true);
                await espInstaller.runFlash(
                    true,
                    fullData,
                    version,
                    checkChip
                )
                enableConsole(true);
            });
        }
        if (updateData !== undefined) {
            tb = addEl('button', 'installButton', btLine, 'Update');
            tb.addEventListener('click', async () => {
                enableConsole(false, true);
                await espInstaller.runFlash(
                    false,
                    updateData,
                    version,
                    checkChip
                )
                enableConsole(true);
            });
        }
    }
    const showLoading=(title)=>{
        setVisible('loadingFrame',title !== undefined);
        if (title){
            setValue('loadingText',title);
        }
    };
    class BinaryStringWriter extends zip.Writer {

        constructor() {
          super();
          this.binaryString = "";
        }
      
        writeUint8Array(array) {
          for (let indexCharacter = 0; indexCharacter < array.length; indexCharacter++) {
            this.binaryString += String.fromCharCode(array[indexCharacter]);
          }
        }
      
        getData() {
          return this.binaryString;
        }
      }
    const handleZip = async (zipFile) => {
        showLoading("loading zip");
        let reader;
        let title="Custom";
        if (typeof(zipFile) === 'string'){
            setValue('loadingText', 'downloading custom build')
            reader= new zip.HttpReader(zipFile);
        }
        else{
            setValue('loadingText', 'loading zip file');
            reader = new zip.BlobReader(zipFile);
            title="Local";
        }
        let zipReader = new zip.ZipReader(reader);
        const entries = (await zipReader.getEntries());
        let fullData;
        let updateData;
        let base = "";
        let environment;
        let buildflags;
        for (let i = 0; i < entries.length; i++) {
            if (entries[i].filename.match(/-all.bin$/)) {
                fullData = await (entries[i].getData(new BinaryStringWriter()));
                base = entries[i].filename.replace("-all.bin", "");
            }
            if (entries[i].filename.match(/-update.bin$/)) {
                updateData = await (entries[i].getData(new BinaryStringWriter()));
                base = entries[i].filename.replace("-update.bin", "");
            }
            if (entries[i].filename === 'buildconfig.txt') {
                let txt = await (entries[i].getData(new zip.TextWriter()));
                environment = txt.replace(/.*pio run *.e */, '').replace(/ .*/, '');
                buildflags = txt.replace(/.*PLATFORMIO_BUILD_FLAGS="/, '').replace(/".*/, '');
            }
        }
        let info;
        if (environment !== undefined && buildflags !== undefined) {
            info = `env=${environment}, flags=${buildflags}`;
        }
        if (updateData === undefined && fullData === undefined){
            throw new Error("no firmware files found in zip");
        }
        buildCustomButtons("dummy", updateData, fullData, info,title);
        showLoading();
    }  
    window.onload = async () => {
        if (! ESPInstaller.checkAvailable()){
            showError("your browser does not support the ESP flashing (no serial)");
            return;
        }
        let custom=getParam('custom');
        let user;
        let repo;
        let errorText=`unable to query release info for user ${user}, repo ${repo}: `;
        if (! custom){
            user = window.gitHubUser||getParam('user','wellenvogel');
            repo = window.gitHubRepo || getParam('repo','esp32-nmea2000');
            if (!user || !repo) {
                alert("missing parameter user or repo");
            }
        }
        try {
            espLoaderTerminal = new XtermOutputHandler('terminal');
            espInstaller = new ESPInstaller(espLoaderTerminal);
            buildConsoleButtons();
            buildUploadButtons();
            if (! custom){
                buildHeading(`${user}:${repo}`);
                releaseData = await espInstaller.getReleaseInfo(user, repo);
                buildButtons(user, repo);
                showLoading();
            }
            else{
                errorText="unable to download custom build";
                await handleZip(custom);
            }
        } catch(error){
            showLoading();
            alert(errorText+error)
        };
    }
})();