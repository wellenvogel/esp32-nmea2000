import { setButtons,fillValues, setValue, buildUrl, fetchJson, setVisible, enableEl, setValues } from "./helper";
(function(){
    const STATUS_INTERVAL=2000;
    const CURRENT_PIPELINE='pipeline';
    let API="cibuild.php";
    let currentPipeline=undefined;
    let downloadUrl=undefined;
    let timer=undefined;
    const showError=(text)=>{
        if (text === undefined){
            setVisible('buildError',false,true);
            return;
        }
        setValue('buildError',text);
        setVisible('buildError',true,true);
    }
    const setRunning=(active)=>{
        if (active){
            downloadUrl=undefined;
            showError();
            setVisible('download',false,true);
            setVisible('status_url',false,true);
        }
        enableEl('start',!active);
    }
    const fetchStatus=(initial)=>{
        if (currentPipeline === undefined) return;
        fetchJson(API,{api:'status',pipeline:currentPipeline})
                .then((st)=>{
                    setValues(st);
                    setVisible('status_url',st.status_url !== undefined,true);
                    setVisible('error',st.error !== undefined,true);
                    if (st.status === 'error'){
                        setRunning(false);
                        setVisible('download',false,true);
                        return;
                    }
                    if (st.status === 'success'){
                        setRunning(false);
                        fetchJson(API,{api:'artifacts',pipeline:currentPipeline})
                        .then((ar)=>{
                            if (! ar.items || ar.items.length < 1){
                                throw new Error("no download link");
                            }
                            downloadUrl=buildUrl(API,{
                                download: currentPipeline
                            });
                            setVisible('download',true,true);

                        })
                        .catch((err)=>{
                            showError("Unable to get build result: "+err);
                            setVisible('download',false,true);
                        });
                        return;
                    }
                    else{
                        setVisible('download',false,true);
                    }
                    timer=window.setTimeout(fetchStatus,STATUS_INTERVAL)
                })
                .catch((e)=>{
                    timer=window.setTimeout(fetchStatus,STATUS_INTERVAL);
                })
    }
    const setCurrentPipeline=(pipeline)=>{
        currentPipeline=pipeline;
        window.localStorage.setItem(CURRENT_PIPELINE,pipeline);
    };
    const startBuild=()=>{
        let param={};
        currentPipeline=undefined;
        if (timer) window.clearTimeout(timer);
        timer=undefined;
        fillValues(param,['environment','buildflags']);
        setValue('status','requested');
        setRunning(true);
        fetchJson(API,Object.assign({
            api:'start'},param))
        .then((json)=>{
            if (json.status === 'error'){
                throw new Error("unable to create job "+(json.error||''));
            }
            if (!json.id) throw new Error("unable to create job, no id");
            setCurrentPipeline(json.id);
            setValue('pipeline',currentPipeline);
            setValue('status',json.status);
            timer=window.setTimeout(fetchStatus,STATUS_INTERVAL);
        })
        .catch((err)=>{
            setRunning(false);
            setValue('status','error');
            showError(err);
        });
    }
    const runDownload=()=>{
        if (! downloadUrl) return;
        let df=document.getElementById('dlframe');
        if (df){
            df.setAttribute('src',null);
            df.setAttribute('src',downloadUrl);
        }
    }
    const webInstall=()=>{
        if (! downloadUrl) return;
        let url=buildUrl("install.html",{custom:downloadUrl});
        window.location.href=url;
    }
    const btConfig={
        start:startBuild,
        download:runDownload,
        webinstall:webInstall
    };
    window.onload=()=>{ 
        setButtons(btConfig);
        currentPipeline=window.localStorage.getItem(CURRENT_PIPELINE);
        if (currentPipeline){
            setValue('pipeline',currentPipeline);
            setRunning(true);
            fetchStatus(true);
        }
    }
})();