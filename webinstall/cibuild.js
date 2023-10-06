import { addEl, setButtons,fillValues, setValue, buildUrl, fetchJson, setVisible, enableEl, setValues, getParam, fillSelect, forEachEl, readFile } from "./helper.js";
import {load as yamlLoad} from "https://cdn.skypack.dev/js-yaml@4.1.0";
import fileDownload from "https://cdn.skypack.dev/js-file-download@0.4.12"
class PipelineInfo{
    constructor(id){
        this.STFIELDS=['status','error','status_url'];
        this.reset(id);
    }
    update(state){
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
        return ['error','success','canceled'].indexOf(this.status) < 0;
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
                    if (currentPipeline.id !== st.pipeline_id) return;
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
                config=JSON.parse(content);
                buildSelectors(ROOT_PATH,structure.config.children,true);
                findPipeline();
            } catch (e){
                alert("upload "+fsel.files[0].name+" failed: "+e);
            }

       }
       fsel.click(); 
    }
    const downloadConfig=()=>{
        let name="buildconfig.json";
        fileDownload(JSON.stringify(config),name);
    }
    const btConfig={
        start:startBuild,
        download:runDownload,
        webinstall:webInstall,
        uploadConfig: uploadConfig,
        downloadConfig: downloadConfig
    };
    const loadConfig=async (url)=>{
        let config=await fetch(url).then((r)=>{
            if (!r.ok) throw new Error("unable to fetch: "+r.statusText);
            return r.text()
        });
        let parsed=yamlLoad(config);
        return parsed;
    }
    const getVal=(cfg,keys)=>{
        for (let i in keys){
            let k=cfg[keys[i]];
            if (k !== undefined) return k;
        }
    }
    const KEY_NAMES=['key','value','label'];
    const LABEL_NAMES=['label','value'];
    const PATH_ATTR='data-path';
    const SEPARATOR=':';
    /**
     * 
     * @param {build a selector} parent 
     * @param {*} config 
     * @param {*} name 
     * @param {*} current 
     * @param {*} callback will be called with: children,key,value,initial
     * @returns 
     */
    const buildSelector=(parent,config,name,current,callback)=>{
        let rep=new RegExp("[^"+SEPARATOR+"]*","g");
        let level=name.replace(rep,'');
        let frame=addEl('div','selector level'+level.length+' t'+config.type,parent);
        frame.setAttribute(PATH_ATTR,name);
        let title=addEl('div','title t'+config.type,frame,config.label);
        if (config.type === 'frame'){
            callback(config.children,true,true,undefined,true);
        }
        if (config.type === 'select') {
            if (!config.values) return;
            config.values.forEach((v) => {
                let ef = addEl('div', 'radioFrame', frame);
                addEl('div', 'label', ef, getVal(v, LABEL_NAMES));
                let re = addEl('input', 'radioCi', ef);
                let val = v.value;
                let key=getVal(v,KEY_NAMES);
                if (val === undefined) val=key;
                re.setAttribute('type', 'radio');
                re.setAttribute('name', name);
                re.addEventListener('change', (ev) => callback(v.children,key,val,v.resource,false));
                if (v.description){ 
                    if(v.url) {
                        let lnk = addEl('a', 'radioDescription', ef, v.description);
                        lnk.setAttribute('href', v.url);
                        lnk.setAttribute('target', '_');
                    }
                    else{
                        let de=addEl('span','radioDescription',ef,v.description);
                    }
                }
                if (key == current) {
                    re.setAttribute('checked','checked');
                    callback(v.children,key,val,v.resource,true);
                }
            });
        }
        if (config.type === 'dropdown'){
            if (!config.values) return;
            const valForIdx=(idx)=>{
                let v=config.values[idx];
                if (typeof(v) !== 'object'){
                    v={label:v,value:v};
                }
                if (v.value === null) v.value=undefined;
                if (v.key === null) v.key=undefined;
                return v;
            };
            const resourceForVal=(v)=>{
                if (v === undefined) return undefined;
                let key=getVal(v,KEY_NAMES);
                if (key === undefined) return key;
                let resource=v.resource;
                if (! resource && config.resource && config.resource.match(/:$/)){
                    resource=config.resource+key;
                }
                return resource;
            };
            let sel=addEl('select','t'+config.type,frame);
            for (let idx=0;idx<config.values.length;idx++){
                let v=valForIdx(idx);
                let opt=addEl('option','',sel,v.label);
                let key=getVal(v,KEY_NAMES);
                if (key === null) key=undefined;
                opt.setAttribute('value',idx);
                if (key == current){
                    opt.setAttribute('selected',true);
                    callback(undefined,key,key,resourceForVal(v),true);
                }
            };
            sel.addEventListener('change',(ev)=>{
                let resource;
                let v=valForIdx(ev.target.value);
                if (! v) return;
                callback(undefined,getVal(v,KEY_NAMES), v.value,resourceForVal(v),false);
            });
        }
        return frame;
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
    const buildSelectors=(prefix,configList,initial)=>{
        removeSelectors(prefix,!initial);
        if (!configList) return;
        let parent=document.getElementById("selectors");
        if (!parent) return;
        let frame=addEl('div','selectorFrame',parent);
        frame.setAttribute(PATH_ATTR,prefix);
        configList.forEach((cfg)=>{
            let key=getVal(cfg,KEY_NAMES);
            let name=prefix?(prefix+SEPARATOR+key):key;
            let current=config[name];
            buildSelector(frame,cfg,name,current,(children,key,value,resource,initial)=>{
                buildSelectors(name,children,initial);
                configStruct[name]={cfg:cfg, key: key, value:value,resource:resource};
                buildValues(initial);
            })
        })
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
                let struct = configStruct[k];
                if (!struct || !struct.cfg || struct.value === undefined) continue;
                if (round > 0) config[k] = struct.key;
                if (struct.cfg.target !== undefined) {
                    if (struct.cfg.target === 'environment') {
                        if (round > 0) environment = struct.value;
                        else allowedResources=struct.resource;
                        continue;
                    }
                    if (round < 1) continue;
                    if (struct.resource){
                        let resList=currentResources[struct.resource];
                        if (! resList){
                            resList=[];
                            currentResources[struct.resource]=resList;
                        }
                        resList.push(struct);
                    }
                    if (struct.cfg.target === 'define') {
                        flags += " -D" + struct.value;
                        continue;
                    }
                    const DEFPRFX = "define:";
                    if (struct.cfg.target.indexOf(DEFPRFX) == 0) {
                        let def = struct.cfg.target.substring(DEFPRFX.length);
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
            if (allowedResources[ak] !== undefined){
                if (resList.length > allowedResources[ak]){
                    errors+=" more than "+allowedResources[ak]+" "+k+" device(s) used";
                }
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
                if (type != 'release' && type != 'tag'){
                    val=type+val;
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
        buildSelectors(ROOT_PATH,structure.config.children,true);
        if (! isRunning()) findPipeline();
        updateStatus();
    }
})();