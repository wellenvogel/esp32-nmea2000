(function(){
    let isActive=false;
    window.esp32nmea2k.registerListener((id,data)=>{
        if (id === 0){
            //data is capabilities
            if (data.testboard) isActive=true;
        }
        if (isActive){
            console.log("exampletask listener",id,data);
        }
    })
})();
