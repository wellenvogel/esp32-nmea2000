(function(){
    const api=window.esp32nmea2k;
    if (! api) return;
    let isActive=false;
    const tabName="example";
    const configName="exampleBDSel";
    let boatItemName;
    api.registerListener((id,data)=>{
        if (id === api.EVENTS.init){
            //data is capabilities
            if (data.testboard) isActive=true;
            if (isActive){
                let page=api.addTabPage(tabName,"Example");
                api.addEl('div','',page,"this is a test tab");
            }
        }
        if (isActive){
            console.log("exampletask listener",id,data);
            if (id === api.EVENTS.tab){
                if (data === tabName){
                    console.log("example tab activated");
                }
            }
            if (id == api.EVENTS.config){
                let nextboatItemName=data[configName];
                console.log("value of "+configName,nextboatItemName);
                if (nextboatItemName){
                    api.addUserFormatter(nextboatItemName,"xxx",function(v){
                        return "X"+v+"X";
                    })
                }
                if (boatItemName !== undefined && boatItemName != nextboatItemName){
                    api.removeUserFormatter(boatItemName);
                }
                boatItemName=nextboatItemName;
            }
        }
    })
})();
