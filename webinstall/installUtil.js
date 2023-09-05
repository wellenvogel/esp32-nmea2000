import {ESPLoader,Transport} from "https://cdn.jsdelivr.net/npm/esptool-js@0.2.1/bundle.js";
/**
 * write all messages to the console
 */
class ConsoleOutputHandler{
    clean() {
    }
    writeLine(data) {
        console.log("ESPInstaller:",data);
    }
    write(data) {
        console.log(data);
    }
}

/**
 * write messages to an instance of xterm 
 * to use this, include in your html
    <script src="https://cdn.jsdelivr.net/npm/xterm@4.19.0/lib/xterm.min.js"></script>
	<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/xterm@4.19.0/css/xterm.css">
 * and create a div element
    <div id="terminal"/>   
 * provide the id of this div to the constructor   
 */
class XtermOutputHandler {
    constructor(termId) {
        let termElement = document.getElementById(termId);
        if (termElement) {
            this.term = new Terminal({ cols: 120, rows: 40 , convertEol: true });
            this.term.open(termElement);
        }
        this.clean=this.clean.bind(this);
        this.writeLine=this.writeLine.bind(this);
        this.write=this.write.bind(this);
    }
    clean() {
        if (!this.term) return;
        this.term.clear();
    }
    writeLine(data) {
        if (!this.term) {
            console.log("TERM:", data);
            return;
        };
        this.term.writeln(data);
    }
    write(data) {
        if (!this.term) {
            console.log("TERM:", data);
            return;
        };
        this.term.write(data)
    }
};
class ESPInstaller{
    constructor(outputHandler){
        this.espLoaderTerminal=outputHandler|| new ConsoleOutputHandler();
        this.transport=undefined;
        this.esploader=undefined;
        this.chipFamily=undefined;
        this.base=import.meta.url.replace(/[^/]*$/,"install.php");
        this.consoleDevice=undefined;
        this.consoleReader=undefined;
    }
    /**
     * get an URL query parameter
     * @param  key 
     * @returns 
     */
    static getParam(key){
        let value=RegExp(""+key+"[^&]+").exec(window.location.search);
        // Return the unescaped value minus everything starting from the equals sign or an empty string
        return decodeURIComponent(!!value ? value.toString().replace(/^[^=]+./,"") : "");
    };
    static checkAvailable(){
        if (! navigator.serial || ! navigator.serial.requestPort) return false;
        return true;
    }
    /**
     * execute a reset on the connected device
     */
    async resetTransport() {
        if (!this.transport) {
            throw new Error("not connected");
        }
        this.espLoaderTerminal.writeLine("Resetting...");
        await this.transport.device.setSignals({
            dataTerminalReady: false,
            requestToSend: true,
        });
        await this.transport.device.setSignals({
            dataTerminalReady: false,
            requestToSend: false,
        });
    };
    
    async disconnect(){
        if (this.consoleDevice){
            try{
                if (this.consoleReader){
                    await this.consoleReader.cancel();
                    this.consoleReader=undefined;
                }
                await this.consoleDevice.close();
            }catch(e){
                console.log(`error cancel serial read ${e}`);
            }
            this.consoleDevice=undefined;
        }
        if (this.transport){
            try{
                await this.transport.disconnect();
                await this.transport.waitForUnlock(1500);
            }catch (e){}
            this.transport=undefined;
        }
        this.esploader=undefined;
    }
    async connect() {
        this.espLoaderTerminal.clean();
        await this.disconnect();
        let device = await navigator.serial.requestPort({});
        if (!device) {
            return;
        }
        try {
            this.transport = new Transport(device);
            this.esploader = new ESPLoader(this.transport, 115200, this.espLoaderTerminal);
            let foundChip = await this.esploader.main_fn();
            if (!foundChip) {
                throw new Error("unable to read chip id");
            }
            this.espLoaderTerminal.writeLine(`chip: ${foundChip}`);
            await this.esploader.flash_id();
            this.chipFamily = this.esploader.chip.CHIP_NAME;
            this.espLoaderTerminal.writeLine(`chipFamily: ${this.chipFamily}`);
        } catch (e) {
            this.disconnect();
            throw e;
        }
    }

    async startConsole(baud) {
        await this.disconnect();
        try {
            let device = await navigator.serial.requestPort({});
            if (!device) {
                return;
            }
            this.consoleDevice=device;
            let br=baud || 115200;
            await device.open({
                baudRate: br
            });
            this.consoleReader=device.readable.getReader();
            this.espLoaderTerminal.clean();
            this.espLoaderTerminal.writeLine(`Console at ${br}:`);
            while (this.consoleReader) {
                let {value:val,done:done} = await this.consoleReader.read();
                if (typeof val !== 'undefined') {
                    this.espLoaderTerminal.write(val);
                }
                if (done){
                    console.log("Console reader stopped");
                    break;
                }
            }
        } catch (e) { this.espLoaderTerminal.writeLine(`Error: ${e}`) }
        this.espLoaderTerminal.writeLine("Console reader stopped");
    }
    async stopConsole(){
        await this.disconnect();
    }
    
    isConnected(){
        return this.transport !== undefined;
    }
    checkConnected(){
        if (! this.isConnected){
            throw new Error("not connected");
        }
    }

    getChipFamily(){
        this.checkConnected();
        return this.chipFamily;
    }
    /**
     * flass the device
     * @param {*} fileList : an array of entries {data:blob,address:number}
     */
    async writeFlash(fileList){
        this.checkConnected();
        this.espLoaderTerminal.writeLine(`Flashing....`);
        await this.esploader.write_flash(
            fileList,
            "keep",
            "keep",
            "keep",
            false
        )
        await this.resetTransport();
        this.espLoaderTerminal.writeLine(`Done.`);
    }
    /**
     * fetch a release asset from github
     * @param {*} user 
     * @param {*} repo 
     * @param {*} version 
     * @param {*} name 
     * @returns 
     */
    async getReleaseAsset(user,repo,version,name){
        const url=this.base+"?dlName="+encodeURIComponent(name)+
            "&dlVersion="+encodeURIComponent(version)+
            "&user="+encodeURIComponent(user)+
            "&repo="+encodeURIComponent(repo);
        this.espLoaderTerminal.writeLine(`downloading image from ${url}`);    
        const resp=await fetch(url);
        if (! resp.ok){
            throw new Error(`unable to download image from ${url}: ${resp.status}`);
        }
        const reader=new FileReader();
        const blob= await resp.blob();
        let data=await new Promise((resolve)=>{
            reader.addEventListener("load",() => resolve(reader.result));
            reader.readAsBinaryString(blob);
        });
        this.espLoaderTerminal.writeLine(`successfully loaded ${data.length} bytes`);
        return data;
    }
    /**
     * handle the click of an install button
     * @param {*} isFull 
     * @param {*} user 
     * @param {*} repo 
     * @param {*} version 
     * @param {*} address 
     * @param {*} assetName the name of the asset file.
     *                      can be a function - will be called with the chip family
     *                      and must return the asset file name
     * @returns 
     */
    async installClicked(isFull, user, repo, version, address, assetName) {
        try {
            await this.connect();
            let assetFileName = assetName;
            if (typeof (assetName) === 'function') {
                assetFileName = assetName(this.getChipFamily());
            }
            let imageData = await this.getReleaseAsset(user, repo, version, assetFileName);
            if (!imageData || imageData.length == 0) {
                throw new Error(`no image data fetched`);
            }
            let fileList = [
                { data: imageData, address: address }
            ];
            let txt = isFull ? "baseImage (all data will be erased)" : "update";
            if (!confirm(`ready to install ${version}\n${txt}`)) {
                this.espLoaderTerminal.writeLine("aborted by user...");
                await this.disconnect();
                return;
            }
            await this.writeFlash(fileList);
            await this.disconnect();
        } catch (e) {
            this.espLoaderTerminal.writeLine(`Error: ${e}`);
            alert(`Error: ${e}`);
        }
    }
    /**
     * fetch the release info from the github API
     * @param {*} user 
     * @param {*} repo 
     * @returns 
     */
    async getReleaseInfo(user,repo){
        let url=this.base+"?api=1&user="+encodeURIComponent(user)+"&repo="+encodeURIComponent(repo)
        let resp=await fetch(url);
        if (! resp.ok){
            throw new Error(`unable to query release info from ${url}: ${resp.status}`);
        }
        return await resp.json();
    }
    /**
     * get the release info in a parsed form
     * @param {*} user 
     * @param {*} repo 
     * @returns an object: {version:nnn, assets:[name1,name2,...]}
     */
    async getParsedReleaseInfo(user,repo){
        let raw=await this.getReleaseInfo(user,repo);
        let rt={
            version:raw.name,
            assets:[]
        };
        if (! raw.assets) return rt;
        raw.assets.forEach((asset)=>{
            rt.assets.push(asset.name);
        })
        return rt;
    }
};
export {ConsoleOutputHandler, XtermOutputHandler};
export default ESPInstaller;