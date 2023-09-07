import {XtermOutputHandler} from "./installUtil.js";
import ESPInstaller from "./installUtil.js";
import { addEl, getParam, setValue, setVisible } from "./helper.js";
import * as zip from "https://cdn.jsdelivr.net/npm/@zip.js/zip.js@2.7.29/+esm";
(function(){
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
    const checkChip=(chipFamily,assetName)=>{
        //for now only ESP32
        if (chipFamily != "ESP32"){
            throw new Error(`unexpected chip family ${chipFamily}, expected ESP32`);
        }
        return assetName;
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
        addEl('div','version',bFrame,`Version: ${version}`);
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
                    4096,
                    (chip)=>checkChip(chip,item.basic)
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
                    65536,
                    (chip)=>checkChip(chip,item.update)
                )
                enableConsole(true);
            });
        }

    }
    const buildCustomButtons = (name, updateData, fullData,version,info,element) => {
        let bFrame = document.querySelector(element || '.content');
        if (!bFrame) return;
        bFrame.textContent = '';
        let item=addEl('div','item',bFrame);
        addEl('div', 'version', item, "Custom "+version);
        if (info){
            addEl('div','version',item,info);
        }
        let btLine = addEl('div', 'buttons', item);
        let tb = addEl('button', 'installButton', btLine, 'Initial');
        tb.addEventListener('click', async () => {
            enableConsole(false, true);
            await espInstaller.runFlash(
                true,
                fullData,
                4096,
                version,
                (ch) => checkChip(ch, name)
            )
            enableConsole(true);
        });
        tb = addEl('button', 'installButton', btLine, 'Update');
        tb.addEventListener('click', async () => {
            enableConsole(false, true);
            await espInstaller.runFlash(
                false,
                updateData,
                65536,
                version,
                (ch) => checkChip(ch, name)
            )
            enableConsole(true);
        });
    }
    const showLoading=(on)=>{
        setVisible('loadingFrame',on);
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
            user = window.gitHubUser||getParam('user');
            repo = window.gitHubRepo || getParam('repo');
            if (!user || !repo) {
                alert("missing parameter user or repo");
            }
        }
        try {
            espLoaderTerminal = new XtermOutputHandler('terminal');
            espInstaller = new ESPInstaller(espLoaderTerminal);
            buildConsoleButtons();
            if (! custom){
                buildHeading(`${user}:${repo}`);
                releaseData = await espInstaller.getReleaseInfo(user, repo);
                buildButtons(user, repo);
            }
            else{
                errorText="unable to download custom build";
                showLoading(true);
                setValue('loadingText','downloading custom build')
                let reader= new zip.HttpReader(custom);
                let zipReader= new zip.ZipReader(reader);
                const entries=(await zipReader.getEntries());
                let fullData;
                let updateData;
                let base="";
                let environment;
                let buildflags;
                for (let i=0;i<entries.length;i++){
                    if (entries[i].filename.match(/-all.bin$/)){
                        fullData=await(entries[i].getData(new BinaryStringWriter()))
                        base=entries[i].filename.replace("-all.bin","");
                    }
                    if (entries[i].filename.match(/-update.bin$/)){
                        updateData=await(entries[i].getData(new BinaryStringWriter()))
                        base=entries[i].filename.replace("-update.bin","");
                    }
                    if (entries[i].filename === 'buildconfig.txt'){
                        let txt=await(entries[i].getData(new zip.TextWriter()));
                        environment=txt.replace(/.*pio run *.e */,'').replace(/ .*/,'');
                        buildflags=txt.replace(/.*PLATFORMIO_BUILD_FLAGS="/,'').replace(/".*/,'');
                    }
                }
                let info;
                if (environment !== undefined && buildflags !== undefined){
                    info=`env=${environment}, flags=${buildflags}`;
                }
                buildCustomButtons("dummy",updateData,fullData,base,info);
                showLoading(false);
            }
        } catch(error){
            showLoading(false);
            alert(errorText+error)
        };
    }
})();