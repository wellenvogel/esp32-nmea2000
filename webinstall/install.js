import {XtermOutputHandler} from "./installUtil.js";
import ESPInstaller from "./installUtil.js";
(function(){
    let espLoaderTerminal;
    let espInstaller;
    let releaseData={};
    const addEl=ESPInstaller.addEl; //shorter typing
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
    const buildHeading=(user,repo,element)=>{
        let hFrame=document.querySelector(element||'.heading');
        if (! hFrame) return;
        hFrame.textContent='';
        let h=addEl('h2',undefined,hFrame,`ESP32 Install ${user}:${repo}`)
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
            let tb=addEl('button','installButton',line,'Initial');
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
            tb=addEl('button','installButton',line,'Update');
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
    window.onload = async () => {
        if (! ESPInstaller.checkAvailable()){
            showError("your browser does not support the ESP flashing (no serial)");
            return;
        }
        let user = window.gitHubUser||ESPInstaller.getParam('user');
        let repo = window.gitHubRepo || ESPInstaller.getParam('repo');
        if (!user || !repo) {
            alert("missing parameter user or repo");
        }
        try {
            espLoaderTerminal = new XtermOutputHandler('terminal');
            espInstaller = new ESPInstaller(espLoaderTerminal);
            buildHeading(user, repo);
            buildConsoleButtons();
            releaseData = await espInstaller.getReleaseInfo(user, repo);
            buildButtons(user, repo);
        } catch(error){alert("unable to query release info for user "+user+", repo "+repo+": "+error)};
    }
})();