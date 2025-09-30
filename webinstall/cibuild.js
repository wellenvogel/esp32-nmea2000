import { addEl, setButtons,fillValues, setValue, buildUrl, fetchJson, setVisible, enableEl, setValues, getParam, fillSelect, forEachEl, readFile } from "./helper.js";
import {load as yamlLoad} from "https://cdn.skypack.dev/js-yaml@4.1.0";
import fileDownload from "https://cdn.skypack.dev/js-file-download@0.4.12"
class PipelineInfo{
    constructor(id){
        this.STFIELDS=['status','error','status_url'];
        this.reset(id);
        this.lastUpdate=0;
    }
    update(state){
        this.lastUpdate=(new Date()).getTime();
        if (state.pipeline_id !== undefined && state.pipeline_id !== this.id){
            return false;
        }
        this.STFIELDS.forEach((i)=>{
            let v=state[i];
            if (v !== undefined)this[i]=v;
        });
    }
    reset(id,opt_state){
        this.id=id;
        this.STFIELDS.forEach((i)=>this[i]=undefined);
        this.downloadUrl=undefined;
        if (opt_state) {
            this.update(opt_state);
        }
        else{
            if (id !== undefined) this.status='fetching';
        }
    }
    valid(){
        return this.id !== undefined;
    }
    isRunning(){
        if (! this.valid()) return false;
        if (this.status === undefined) return false;
        return ['error','success','canceled','failed','errored'].indexOf(this.status) < 0;
    }

}
(function(){
    const STATUS_INTERVAL=2000;
    const CURRENT_PIPELINE='pipeline';
    const API="cibuild.php";
    const GITAPI="install.php";
    const GITUSER="wellenvogel";
    const GITREPO="esp32-nmea2000";
    let currentPipeline=new PipelineInfo();
    let timer=undefined;
    let structure=undefined;
    let config={}; //values as read and stored
    let configStruct={}; //complete struct merged of config and struct
    let displayMode='last';
    let delayedSearch=undefined;
    let gitSha=undefined;
    let buildVersion=undefined;
    let configName="buildconfig";
    let isModified=false;
    const modeStrings={
        last: 'Last Build',
        existing: 'Existing Build',
        current: 'Current Build'
    };
    const setDisplayMode=(mode)=>{
        let old=displayMode;
        let ms=modeStrings[mode];
        if (ms === undefined){
            return false;
        }
        displayMode=mode;
        setValue('resultTitle',ms);
        return mode !== old;
    }
    const updateStatus=()=>{
        setValues(currentPipeline,{
            id: 'pipeline'
        });
        setVisible('download',currentPipeline.valid() && currentPipeline.downloadUrl!==undefined,true);
        setVisible('status_url',currentPipeline.valid() && currentPipeline.status_url!==undefined,true);
        setVisible('error',currentPipeline.error!==undefined,true);
        let values={};
        fillValues(values,['configError','environment']);
        setVisible('buildCommand',values.environment !== "" && ! values.configError);
        if (values.configError) {
            enableEl('start',false);
            return;
        }
        if (!values.environment){
            enableEl('start',false);
            return;
        }
        if (displayMode != 'existing'){
            if (currentPipeline.valid()){
                //check pipeline state
                if (['error','success','canceled','failed'].indexOf(currentPipeline.status) >= 0){
                    enableEl('start',true);
                    return;       
                }
                enableEl('start',false);
                return;    
            }
            enableEl('start',true);
            return;
        }
        //display mode existing
        //allow start if either no pipeline or not running and status != success
        enableEl('start',!currentPipeline.valid() || (!currentPipeline.isRunning() && currentPipeline.status != "success"));
    }
    const isRunning=()=>{
        return currentPipeline.isRunning();
    }
    const fetchStatus=(initial)=>{
        if (! currentPipeline.valid()){
            updateStatus();
            return;
        }
        let queryPipeline=currentPipeline.id;
        fetchJson(API,{api:'status',pipeline:currentPipeline.id})
                .then((st)=>{
                    if (queryPipeline !== currentPipeline.id) return;
                    let stid=st.pipeline_id||st.id;
                    if (stid !== undefined &&  currentPipeline.id !== stid) return;
                    if (st.status === undefined) st.status=st.state;
                    currentPipeline.update(st);
                    updateStatus();
                    if (st.status === 'error' || st.status === 'errored' || st.status === 'canceled'){
                        return;
                    }
                    if (st.status === 'success'){
                        fetchJson(API,{api:'artifacts',pipeline:currentPipeline.id})
                        .then((ar)=>{
                            if (! ar.items || ar.items.length < 1){
                                throw new Error("no download link");
                            }
                            currentPipeline.downloadUrl=buildUrl(API,{
                                download: currentPipeline.id
                            });
                            updateStatus();

                        })
                        .catch((err)=>{
                            currentPipeline.update({
                                status:'error',
                                error:"Unable to get build result: "+err
                            });
                            updateStatus();
                        });
                        return;
                    }
                    timer=window.setTimeout(fetchStatus,STATUS_INTERVAL)
                })
                .catch((e)=>{
                    timer=window.setTimeout(fetchStatus,STATUS_INTERVAL);
                })
    }
    const setCurrentPipeline=(pipeline,doStore)=>{
        currentPipeline.reset(pipeline);
        if (doStore) window.localStorage.setItem(CURRENT_PIPELINE,pipeline);
    };
    const startBuild=()=>{
        let param={};
        currentPipeline.reset(undefined,{status:'requested'});
        if (timer) window.clearTimeout(timer);
        timer=undefined;
        fillValues(param,['environment','buildflags']);
        setDisplayMode('current');
        updateStatus();
        if (gitSha !== undefined) param.tag=gitSha;
        param.config=JSON.stringify(config);
        if (buildVersion !== undefined){
            param.suffix="-"+buildVersion;
        }
        fetchJson(API,Object.assign({
            api:'start'},param))
        .then((json)=>{
            let status=json.status || json.state|| 'error';
            if (status === 'error'){
                currentPipeline.update({status:status,error:json.error})
                updateStatus();
                throw new Error("unable to create job "+(json.error||''));
            }
            if (!json.id) {
                let error="unable to create job, no id"
                currentPipeline.update({status:'error',error:error});
                updateStatus();
                throw new Error(error);
            }
            setCurrentPipeline(json.id,true);
            updateStatus();
            timer=window.setTimeout(fetchStatus,STATUS_INTERVAL);
        })
        .catch((err)=>{
            currentPipeline.update({status:'error',error:err});
            updateStatus();
        });
    }
    const runDownload=()=>{
        if (! currentPipeline.downloadUrl) return;
        let df=document.getElementById('dlframe');
        if (df){
            df.setAttribute('src',null);
            df.setAttribute('src',currentPipeline.downloadUrl);
        }
    }
    const webInstall=()=>{
        if (! currentPipeline.downloadUrl) return;
        let url=buildUrl("install.html",{custom:currentPipeline.downloadUrl});
        window.location.href=url;
    }
    const uploadConfig=()=>{
       let form=document.getElementById("upload");
       form.reset();
       let fsel=document.getElementById("fileSelect");
       fsel.onchange=async ()=>{
            if (fsel.files.length < 1) return;
            let file=fsel.files[0];
            if (! file.name.match(/json$/)){
                alert("only json files");
                return;
            }
            try{
                let content=await readFile(file,true);
                let newConfig=JSON.parse(content);
                removeSelectors(ROOT_PATH,true);
                config=newConfig;
                buildSelectors(ROOT_PATH,structure.config.children,true);
                findPipeline();
            } catch (e){
                alert("upload "+fsel.files[0].name+" failed: "+e);
            }

       }
       fsel.click(); 
    }
    const downloadConfig=()=>{
        let name=configName;
        const buildname=config["root:buildname"]
        if (buildname && name != buildname){
            name+="-"+buildname+"-";
        }
        name=name.replace(/[0-9]*$/,'')+formatDate(undefined,true);
        name+=".json";
        fileDownload(JSON.stringify(config),name);
    }
    const showOverlay=(text, isHtml, secondButton)=>{
        let el = document.getElementById('overlayContent');
        if (isHtml) {
            el.innerHTML = text;
            el.classList.remove("text");
        }
        else {
            el.textContent = text;
            el.classList.add("text");
        }
        let container = document.getElementById('overlayContainer');
        container.classList.remove('hidden');
        let db=document.getElementById("secondDialogButton");
        if (db) {
            if (secondButton && secondButton.callback) {
                db.classList.remove("hidden");
                if (secondButton.title){db.textContent=secondButton.title}
                db.onclick=secondButton.callback;
            }
            else {
                db.classList.add("hidden");
            }
        }
    }
    const hideOverlay=()=> {
        let container = document.getElementById('overlayContainer');
        container.classList.add('hidden');
    }
    const loadConfig=async (url)=>{
        let config=await fetch(url).then((r)=>{
            if (!r.ok) throw new Error("unable to fetch: "+r.statusText);
            return r.text()
        });
        let parsed=yamlLoad(config);
        return parsed;
    }
    const showBuildCommand= async ()=>{
        let v={};
        fillValues(v,['environment','buildflags']);
        if (v.environment !== ""){
            let help="Run the build from a command line:\n";
            let cmd="PLATFORMIO_BUILD_FLAGS=\"";
            cmd+=v.buildflags;
            cmd+="\" pio run -e "+v.environment;
            help+=cmd;
            showOverlay(help,false,{title:"Copy",callback:(ev)=>{
                try{
                    navigator.clipboard.writeText(cmd);
                    //alert("copied:"+cmd);
                }
                catch (e){
                    alert("Unable to copy:"+e);
                }
            }});
        }
    }
    const btConfig={
        start:startBuild,
        download:runDownload,
        webinstall:webInstall,
        uploadConfig: uploadConfig,
        downloadConfig: downloadConfig,
        hideOverlay: hideOverlay,
        buildCommand: showBuildCommand
    };
    
    
    const PATH_ATTR='data-path';
    const SEPARATOR=':';
    const expandObject=(obj,parent)=>{
        if (typeof(obj) !== 'object'){
            obj={value:obj}
        }
        let rt=Object.assign({},obj);
        if (rt.value === undefined && rt.key !== undefined) rt.value=rt.key;
        if (rt.key === undefined) rt.key=rt.value;
        if (rt.value === null) rt.value=undefined;
        if (rt.label === undefined){
            if (rt.value !== undefined) rt.label=rt.value;
            else rt.label=rt.key;
        }
        if (rt.resource === undefined && typeof(parent) === 'object'){
            if (parent.resource !== undefined){
                if (parent.resource.match(/:$/)){
                    if(rt.value !== undefined && rt.value !== null){
                        rt.resource=parent.resource+rt.value;
                    }
                }
                else{
                    rt.resource=parent.resource;
                }
            }    
        }
        if (rt.target === undefined && typeof(parent) === 'object'){
            rt.target=parent.target;
        }
        if (rt.mandatory === undefined && typeof(parent) === 'object'){
            rt.mandatory=parent.mandatory;
        }
        return rt;
    }
    const expandList=(lst,parent)=>{
        let rt=[];
        if (! lst) return rt;
        lst.forEach((e)=>rt.push(expandObject(e,parent)));
        return rt;
    }

    const addDescription=(v,frame)=>{
        if (frame === undefined) return;
        if (v.description){ 
            if(v.url) {
                let lnk = addEl('a', 'description', frame, v.description);
                lnk.setAttribute('href', v.url);
                lnk.setAttribute('target', '_');
            }
            else{
                let de=addEl('div','description',frame,v.description);
            }
        }
        if (v.help){
            let bt=addEl('button','help',frame,'?');
            bt.addEventListener('click',()=>showOverlay(v.help));
        }
        else if (v.helpHtml){
            let bt=addEl('button','help',frame,'?');
            bt.addEventListener('click',()=>showOverlay(v.helpHtml,true));
        }    
    }
    /**
     * 
     * @param {build a selector} parent 
     * @param {*} config 
     * @param {*} name 
     * @param {*} current 
     * @param {*} callback will be called with: children,key,value,initial
     * @returns 
     */
    const buildSelector=(parent,cfgBase,name,current,callback)=>{
        let config=expandObject(cfgBase);
        if (current === undefined && config.default !== undefined){
            current=config.default;
        }
        let rep=new RegExp("[^"+SEPARATOR+"]*","g");
        let level=name.replace(rep,'');
        let frame=addEl('div','selector level'+level.length+' t'+config.type,parent);
        frame.setAttribute(PATH_ATTR,name);
        let inputFrame=addEl('div','inputFrame',frame);
        let titleFrame=undefined;
        if (config.label !== undefined){
            titleFrame=addEl('div','titleFrame t'+config.type,inputFrame);
            addEl('div','title t'+config.type,titleFrame,config.label);
        }
        let initialConfig=undefined
        if (config.type === 'frame' || config.type === undefined){
            initialConfig=config;
        }
        let expandedValues=expandList(config.values,config);
        expandedValues.forEach((v)=>{
            if (v.type !== undefined && v.type !== "frame"){
                let err="value element with wrong type "+v.type+" at "+name;
                alert(err);
                throw new Error(err);
            }
        })
        if (config.type === 'select') {
            addDescription(config,titleFrame);
            for (let idx=0;idx<expandedValues.length;idx++){
                let v=expandedValues[idx];
                if (v.key === undefined) continue;
                let ef = addEl('div', 'radioFrame', inputFrame);
                addEl('div', 'label', ef, v.label);
                let re = addEl('input', 'radioCi', ef);
                re.setAttribute('type', 'radio');
                re.setAttribute('name', name);
                re.addEventListener('change', (ev) => callback(v,false));
                addDescription(v,ef);
                if (v.key == current) {
                    re.setAttribute('checked','checked');
                    initialConfig=v;
                }
            };
        }
        if (expandedValues.length > 0 &&  config.type === 'dropdown'){
            let sel=addEl('select','t'+config.type,inputFrame);
            for (let idx=0;idx<expandedValues.length;idx++){
                let v=expandedValues[idx];
                if (v.key === undefined) continue;
                let opt=addEl('option','',sel,v.label);
                opt.setAttribute('value',idx);
                if (v.key == current){
                    opt.setAttribute('selected',true);
                    initialConfig=v;
                }
            };
            addDescription(config,inputFrame);
            sel.addEventListener('change',(ev)=>{
                let v=expandedValues[ev.target.value];
                if (! v) return;
                callback(v,false);
            });
        }
        if (config.type === 'range'){
            if (config.min !== undefined && config.max !== undefined) {
                let min=config.min+0;
                let step=1;
                if (config.step !== undefined) step=config.step+0;
                let max=config.max+0;
                let valid=false;
                if (step > 0){
                    if (min < max) valid=true;
                }
                else{
                    if (min > max) {
                        let tmp=max;
                        max=min;
                        min=tmp;
                        valid=true;
                    }
                }
                if (! valid){
                    console.log("invalid range config",config);
                }
                else {
                    let sel = addEl('select', 'tdropdown', inputFrame);
                    for (let idx=min;idx <=max;idx+=step){
                        let opt=addEl('option','',sel,idx);
                        opt.setAttribute('value',idx);
                        if (idx == current){
                            opt.setAttribute('selected',true);
                            initialConfig=expandObject({key:idx,value:idx},config);
                        }    
                    }
                    if (! initialConfig){
                        initialConfig=expandObject({key:min,value:min},config);
                    }
                    addDescription(config, inputFrame);
                    sel.addEventListener('change', (ev) => {
                        let v = expandObject({ key: ev.target.value, value: ev.target.value },config);
                        callback(v, false);
                    });
                }   
            }
        }
        if (expandedValues.length > 0 && config.type === 'checkbox'){
            let act=undefined;
            let inact=undefined;
            expandedValues.forEach((ev)=>{
                if (ev.key === true || ev.key === undefined){
                    act=ev;
                    if (act.key === undefined) act.key=true;
                    return;
                }
                inact=ev;
            });
            if (act !== undefined){
                if (inact === undefined) inact={key:false};
                let cb=addEl('input','t'+config.type,inputFrame);
                cb.setAttribute('type','checkbox');
                if (current) {
                    cb.setAttribute('checked',true);
                    initialConfig=act;
                }
                else{
                    initialConfig=inact;
                }
                addDescription(config,inputFrame);
                cb.addEventListener('change',(ev)=>{
                    if (ev.target.checked){
                        callback(act,false);
                    }
                    else
                    {
                        callback(inact,false);
                    }
                });
            }
        }
        if (expandedValues.length > 0 && config.type === 'display'){
            let cb=addEl('div','t'+config.type,inputFrame);
            addDescription(config,inputFrame);
            initialConfig=expandedValues[0];
        }
        if (config.type === 'string'){
            let ip=addEl('input','t'+config.type,inputFrame);
            addDescription(config,inputFrame);
            ip.value=current?current:"";
            ip.addEventListener('change',(ev)=>{
                let value=ev.target.value;
                let modified=false;
                if (config.max){
                    if (value && value.length > config.max){
                        modified=true;
                        value=value.substring(0,config.max);
                    }
                }
                if (config.allowed){
                    let check=new RegExp("[^"+config.allowed+"]","g");
                    let nv=value.replace(check,"");
                    if (nv != value){
                        modified=true;
                        value=nv;
                    }
                }
                if (modified){
                    ev.target.value=value;
                }
                callback(Object.assign({},config,{key: value,value:value}),false);

            });
        }
        let childFrame=addEl('div','childFrame',frame);
        if (initialConfig !== undefined){
            callback(initialConfig,true,childFrame);
        }
        return childFrame;
    }
    const removeSelectors=(prefix,removeValues)=>{
        forEachEl('.selectorFrame',(el)=>{
            let path=el.getAttribute(PATH_ATTR);
            if (! path) return;
            if (path.indexOf(prefix) == 0){
                el.remove();
            }
        })
        if (removeValues){
            let removeKeys=[];
            for (let k in configStruct){
                if (k.indexOf(prefix) == 0) removeKeys.push(k);
            }
            for (let k in config){
                if (k.indexOf(prefix) == 0) removeKeys.push(k);
            }
            removeKeys.forEach((k)=>{
                delete config[k];
                delete configStruct[k];
            });
        }
    }
    const buildSelectors=(prefix,configList,initial,base,parent)=>{
        if (!parent) parent=document.getElementById("selectors");;
        if (!configList) return;
        let frame=addEl('div','selectorFrame',parent);
        frame.setAttribute(PATH_ATTR,prefix);
        let expandedList=expandList(configList);
        expandedList.forEach((cfg)=>{
            let currentBase=Object.assign({},base,cfg.base);
            cfg=replaceValues(cfg,currentBase);
            if (cfg.key === undefined){
                if (cfg.type !== undefined && cfg.type !== 'frame'){
                    console.log("config without key",cfg);
                    return;
                }
            }
            let name=prefix;
            if (name !== undefined){
                if (cfg.key !== undefined) {
                    name=prefix+SEPARATOR+cfg.key;
                }
            }
            else{
                name=cfg.key;
            }
            let current=config[name];
            let childFrame=buildSelector(frame,cfg,name,current,
                (child,initial,opt_frame)=>{
                    if(cfg.key !== undefined) removeSelectors(name,!initial);
                    if (! initial) isModified=true;
                    buildSelectors(name,child.children,initial,Object.assign({},currentBase,child.base),opt_frame||childFrame);
                    if (cfg.key !== undefined) configStruct[name]={cfg:child,base:currentBase};
                    buildValues(initial);
            })
        })
    }
    const replaceValues=(str,base)=>{
        if (! base) return str;
        if (typeof(str) === 'string'){
            for (let k in base){
                if (typeof(base[k]) !== 'string'){
                    //special replacement
                    //for complete parts
                    if (str === '#'+k+'#'){
                        return base[k];
                    }
                }
                else{
                    let r=new RegExp("#"+k+"#","g");
                    str=str.replace(r,base[k]);
                }
            }
            return str;
        }
        if (str instanceof Array){
            let rt=[];
            str.forEach((el)=>{
                rt.push(replaceValues(el,base));
            })
            return rt;
        }
        if (str instanceof Object){
            let rt={};
            for (let k in str){
                if (k == 'children') rt[k]=str[k];
                else rt[k]=replaceValues(str[k],base);
            }
            return rt;
        }
        return str;
    }
    const ROOT_PATH='root';
    const buildValues=(initial)=>{
        let environment;
        let flags="";
        if (! initial){
            config={};
        }
        let allowedResources={};
        let currentResources={};
        let errors="";
        for (let round = 0; round <= 1; round++) {
            //round1: find allowed resources
            //round2: really collect values
            for (let k in configStruct) {
                let container = configStruct[k];
                if (! container.cfg) continue;
                let struct=container.cfg;
                if (round > 0) config[k] = struct.key;
                if (struct.target !== undefined ) {
                    if (struct.value === undefined){
                        if (struct.mandatory && round > 0){
                            errors+=" missing value for "+k+"\n";
                        }
                        continue;
                    }
                    let target=replaceValues(struct.target,container.base);
                    if (target === 'environment' ) {
                        if (round > 0 && struct.key !== undefined) environment = struct.value;
                        else allowedResources=struct.resource;
                        continue;
                    }
                    if (round < 1) continue;
                    if (struct.resource){
                        let splitted=struct.resource.split(",");
                        splitted.forEach((resource) => {
                            let resList = currentResources[resource];
                            if (!resList) {
                                resList = [];
                                currentResources[resource] = resList;
                            }
                            resList.push(struct);
                        });
                    }
                    if (target === 'define') {
                        flags += " -D" + struct.value;
                        continue;
                    }
                    const DEFPRFX = "define:";
                    if (target.indexOf(DEFPRFX) == 0) {
                        let def = target.substring(DEFPRFX.length);
                        flags += " -D" + def + "=" + struct.value;
                        continue;
                    }
                }
            }
        }
        if (buildVersion !== undefined){
            flags+=" -DGWRELEASEVERSION="+buildVersion;
        }
        setValues({environment:environment,buildflags:flags});
        //check resources
        for (let k in currentResources){
            let ak=k.replace(/:.*/,'');
            let resList=currentResources[k];
            let allowed=allowedResources[ak];
            if (allowed === undefined) allowed=1;
            if (resList.length > allowed){
                errors+=" more than "+allowed+" device(s) of type "+k+" used";
            }
            
        }
        if (errors){
            setValue('configError',errors);
            setVisible('configError',true,true);
        }
        else{
            setValue('configError','');
            setVisible('configError',false,true);
        }
        if (! initial) findPipeline();
        updateStatus();
    }
    let findIdx=0;
    const findPipeline=()=>{
        if (delayedSearch !== undefined){
            window.clearTimeout(delayedSearch);
            delayedSearch=undefined;
        }
        if (isRunning()) {
            delayedSearch=window.setTimeout(findPipeline,500);
            return;
        }
        findIdx++;
        let queryIdx=findIdx;
        let param={find:1};
        fillValues(param,['environment','buildflags']);
        if (gitSha !== undefined) param.tag=gitSha;
        fetchJson(API,param)
            .then((res)=>{
                if (queryIdx != findIdx) return;
                setCurrentPipeline(res.pipeline);
                if (res.pipeline) currentPipeline.status="found";
                setDisplayMode('existing');
                updateStatus();
                fetchStatus(true); 
            })
            .catch((e)=>{
                console.log("findPipeline error ",e)
                if (displayMode == 'existing'){
                    setCurrentPipeline();
                    updateStatus();
                }
            });

    }
    const formatDate=(opt_date,opt_includeMs)=>{
        const fmt=(v)=>{
            return ((v<10)?"0":"")+v;
        }
        let now=opt_date|| new Date();
        let rt=now.getFullYear()+fmt(now.getMonth()+1)+fmt(now.getDate());
        if (opt_includeMs){
            rt+=fmt(now.getHours())+fmt(now.getMinutes())+fmt(now.getSeconds());
        }
        return rt;
    }
    window.onload=async ()=>{ 
        setButtons(btConfig);
        let pipeline=window.localStorage.getItem(CURRENT_PIPELINE);
        setDisplayMode('last');
        if (pipeline){
            setCurrentPipeline(pipeline);
            updateStatus();
            fetchStatus(true);
        }
        let gitParam={user:GITUSER,repo:GITREPO};
        let branch=getParam('branch');
        if (branch){
            try{
                let info=await fetchJson(GITAPI,Object.assign({},gitParam,{branch:branch}));
                if (info.object){
                    gitSha=info.object.sha;
                    setValue('branchOrTag','branch');
                    setValue('branchOrTagValue',branch);
                }
            }catch (e){
                console.log("branch query error",e);
            }
        }
        if (gitSha === undefined) {
            let tag = getParam('tag');
            let type="tag";
            if (!tag) {
                try {
                    let relinfo = await fetchJson(GITAPI, Object.assign({}, gitParam, { api: 1 }));
                    if (relinfo.tag_name) {
                        tag = relinfo.tag_name;
                        type="release";
                    }
                    else {
                        alert("unable to query latest release");
                    }
                } catch (e) {
                    alert("unable to query release info " + e);
                }
            }
            if (tag){
                try{
                    let info=await fetchJson(GITAPI,Object.assign({},gitParam,{tag:tag}));
                    if (info.object){
                        gitSha=info.object.sha;
                        setValue('branchOrTag',type);
                        setValue('branchOrTagValue',tag);    
                    }
                }catch(e){
                    alert("cannot get sha for tag "+tag+": "+e);
                }
            }
        }
        if (gitSha === undefined){
            //last resort: no sha, let the CI pick up latest
            setValue('gitSha','unknown');
            setValue('branchOrTag','branch');
            setValue('branchOrTagValue','master');
        }
        else{
            setValue('gitSha',gitSha);
        }
        let bot=document.getElementById('branchOrTag');
        let botv=document.getElementById('branchOrTagValue');
        if (bot  && botv){
            let type=bot.textContent;
            let val=botv.textContent;
            if (type && val){
                if (type != 'release' && type != 'tag' && type != 'branch'){
                    val=type+val;
                }
                if (type == 'branch'){
                    val=val+formatDate();
                }
                val=val.replace(/[:.]/g,'_');
                val=val.replace(/[^a-zA-Z0-9_]*/g,'');
                if (val.length > 32){
                    val=val.substring(val.length-32)
                }
                if (val.length > 0){
                    buildVersion=val;
                    setValue('buildVersion',buildVersion);
                }
            }
        }
        if (gitSha !== undefined){
            let url=buildUrl(GITAPI,Object.assign({},gitParam,{sha:gitSha,proxy:'webinstall/build.yaml'}));
            try{
                structure=await loadConfig(url);
            }catch (e){
                alert("unable to load config for selected release:\n "+e+"\n falling back to default");
            }
        }
        if (! structure){
            structure=await loadConfig("build.yaml");
        }
        let ucfg=getParam('config');
        let loadedCfg=undefined;
        if (ucfg){
            ucfg=ucfg.replace(/[^.a-zA-Z_0-9-]/g,'');
            if (gitSha !== undefined){
                try{
                    loadedCfg=await fetchJson(GITAPI,Object.assign({},gitParam,{sha:gitSha,proxy:'webinstall/config/'+ucfg+".json"}));
                }catch(e){
                    alert("unable to load config "+ucfg+" for selected release, trying latest");
                }
            }
            if (loadedCfg === undefined){
                try{
                    loadedCfg=await fetchJson('config/'+ucfg+".json");
                }catch(e){
                    alert("unable to load config "+ucfg+": "+e);
                }
            }
            if (loadedCfg !== undefined){
                configName=ucfg;
                config=loadedCfg;
            }
        }
        buildSelectors(ROOT_PATH,structure.config.children,true);
        if (! isRunning()) findPipeline();
        updateStatus();
        const translationCheck=()=>{
            const lang = document.documentElement.lang;
            if (lang != "en"){
                alert(
                    "This page will not work correctly with translation enabled"
                );
            }
        }
        // Works at least for Chrome, Firefox, Safari and probably more. Not Microsoft
        // Edge though. They're special.
        // Yell at clouds if a translator doesn't change it
        const observer = new MutationObserver(() => {
            translationCheck();
        });
        observer.observe(document.documentElement, {
            attributes: true,
            attributeFilter: ['lang'],
            childList: false,
            characterData: false,
        });
        translationCheck();

    }
})();