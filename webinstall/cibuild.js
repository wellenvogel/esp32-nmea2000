import { setButtons,fillValues, setValue, buildUrl, fetchJson, setVisible, enableEl } from "./helper";
(function(){
    const STATUS_INTERVAL=2000;
    const CURRENT_PIPELINE='pipeline';
    let API="cibuild.php";
    let currentPipeline=undefined;
    let downloadUrl=undefined;
    let timer=undefined;
    const fetchStatus=()=>{
        if (currentPipeline === undefined) return;
        fetchJson(API,{api:'status',pipeline:currentPipeline})
                .then((st)=>{
                    setValue('status',st.status);
                    let l=document.getElementById('link');
                    if (l){
                        if (st.status_url){
                            l.setAttribute('href',st.status_url);
                            setVisible(l.parentElement,true);
                        }
                        else{
                            setVisible(l.parentElement,false);
                        }
                    }
                    if (st.status === 'success'){
                        enableEl('start',true);
                        fetchJson(API,{api:'artifacts',pipeline:currentPipeline})
                        .then((ar)=>{
                            if (! ar.items || ar.items.length < 1){
                                throw new Error("no download link");
                            }
                            downloadUrl=ar.items[0].url;
                            setVisible(document.getElementById('download'),true,true);

                        })
                        .catch((err)=>alert("Unable to get build result: "+err));
                        return;
                    }
                    else{
                        setVisible(document.getElementById('download'),false,true);
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
            enableEl('start',false);
            timer=window.setTimeout(fetchStatus,STATUS_INTERVAL);
        })
        .catch((err)=>{
            setValue('status','error');
            enableEl('start',true);
            alert(err);
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
    const btConfig={
        start:startBuild,
        download:runDownload
    };
    window.onload=()=>{ 
        setButtons(btConfig);
        currentPipeline=window.localStorage.getItem(CURRENT_PIPELINE);
        if (currentPipeline){
            setValue('pipeline',currentPipeline);
            enableEl('start',false);
            fetchStatus();
        }
    }
})();