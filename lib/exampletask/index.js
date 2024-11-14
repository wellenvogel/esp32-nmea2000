(function(){
    const api=window.esp32nmea2k;
    if (! api) return;
    //we only do something if a special capability is set
    //on our case this is "testboard"
    //so we only start any action when we receive the init event
    //and we successfully checked that our requested capability is there
    let isActive=false;
    const tabName="example";
    const configName="exampleBDSel";
    const infoUrl='https://github.com/wellenvogel/esp32-nmea2000/tree/master/lib/exampletask';
    let boatItemName;
    let boatItemElement;
    api.registerListener((id,data)=>{
        if (isActive){
            console.log("exampletask status listener",data);
        }
    },api.EVENTS.status)
    api.registerListener((id,data)=>{
        if (id === api.EVENTS.init){
            //data is capabilities
            //check if our requested capability is there (see GwExampleTask.h)
            if (data.testboard) isActive=true;
            if (isActive){
                //add a simple additional tab page
                //you will have to build the content of the page dynamically
                //using normal dom manipulation methods
                //you can use the helper addEl to create elements
                let page=api.addTabPage(tabName,"Example");
                api.addEl('div','hdg',page,"this is a test tab");
                let vrow=api.addEl('div','row',page);
                api.addEl('span','label',vrow,'loops: ');
                let lcount=api.addEl('span','value',vrow,'0');
                //query the loop count
                window.setInterval(()=>{
                    fetch('/api/user/exampleTask/data')
                        .then((res)=>{
                            if (! res.ok) throw Error("server error: "+res.status);
                            return res.text();
                        })
                        .then((txt)=>{
                            lcount.textContent=txt;
                        })
                        .catch((e)=>console.log("rq:",e));
                },1000);
                api.addEl('button','',page,'Info').addEventListener('click',function(ev){
                    window.open(infoUrl,'info');
                })
                //add a tab for an external URL
                api.addTabPage('exhelp','Info',infoUrl);
            }
        }
        if (isActive){
            //console.log("exampletask listener",id,data);
            if (id === api.EVENTS.tab){
                if (data === tabName){
                    //maybe we need some activity when our page is being activated
                    console.log("example tab activated");
                }
            }
            if (id == api.EVENTS.config){
                //we have a configuration that
                //gives us the name of a boat data item we would like to 
                //handle special
                //in our case we just use an own formatter and add some
                //css to the display field
                //as this item can change we need to keep track of the 
                //last item we handled
                let nextboatItemName=data[configName];
                console.log("value of "+configName,nextboatItemName);
                if (nextboatItemName){
                    //register a user formatter that will be called whenever
                    //there is a new valid value
                    //we simply add an "X:" in front
                    api.addUserFormatter(nextboatItemName,"m(x)",function(v,valid){
                        if (!valid) return;
                        return "X:"+v;
                    })
                    //after this call the item will be recreated
                }
                if (boatItemName !== undefined && boatItemName != nextboatItemName){
                    //if the boat item that we handle has changed, remove
                    //the previous user formatter (this will recreate the item)
                    api.removeUserFormatter(boatItemName);
                }
                boatItemName=nextboatItemName;
                boatItemElement=undefined;
            }
            if (id == api.EVENTS.dataItemCreated){
                //this event is called whenever a data item has
                //been created (or recreated)
                //if this is the item we handle, we just add a css class
                //we could also completely rebuild the dom below the element
                //and use our formatter to directly write/draw the data
                //avoid direct manipulation of the element (i.e. changing the classlist)
                //as this element remains there all the time
                if (boatItemName && boatItemName == data.name){
                    boatItemElement=data.element;
                    //use the helper forEl to find elements within the dashboard item
                    //the value element has the class "dashValue"
                    api.forEl(".dashValue",function(el){
                        el.classList.add("examplecss");
                    },boatItemElement);
                }
            }
        }
    })
})();
