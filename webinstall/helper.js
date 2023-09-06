const getParam = (key) => {
    let value = RegExp("" + key + "[^&]+").exec(window.location.search);
    // Return the unescaped value minus everything starting from the equals sign or an empty string
    return decodeURIComponent(!!value ? value.toString().replace(/^[^=]+./, "") : "");
};
/**
     * add an HTML element
     * @param {*} type 
     * @param {*} clazz 
     * @param {*} parent 
     * @param {*} text 
     * @returns 
     */
const addEl = (type, clazz, parent, text) => {
    let el = document.createElement(type);
    if (clazz) {
        if (!(clazz instanceof Array)) {
            clazz = clazz.split(/  */);
        }
        clazz.forEach(function (ce) {
            el.classList.add(ce);
        });
    }
    if (text) el.textContent = text;
    if (parent) parent.appendChild(el);
    return el;
}
/**
 * call a function for each matching element
 * @param {*} selector 
 * @param {*} cb 
 */
const forEachEl = (selector, cb) => {
    let arr = document.querySelectorAll(selector);
    for (let i = 0; i < arr.length; i++) {
        cb(arr[i]);
    }
}

const setButtons=(config)=>{
    for (let k in config){
        let bt=document.getElementById(k);
        if (bt){
            bt.addEventListener('click',config[k]);
        }
    }
}
const fillValues=(values,items)=>{
    items.forEach((it)=>{
        let e=document.getElementById(it);
        if (e){
            values[it]=e.value; //TODO: type of el
        }
    })
};
const setValue=(id,value)=>{
    let el=document.getElementById(id);
    if (! el) return;
    if (el.tagName == 'DIV'){
        el.textContent=value;
        return;
    }
    if (el.tagName == 'INPUT'){
        el.value=value;
        return;
    }
    if (el.tagName == 'A'){
        el.setAttribute('href',value);
        return;
    }
}
const setValues=(data,translations)=>{
    for (let k in data){
        let id=k;
        if (translations){
            let t=translations[k];
            if (t !== undefined) id=t;
        }
        setValue(id,data[k]);
    }
}
const buildUrl=(url,pars)=>{
    let delim=(url.match("[?]"))?"&":"?";
    for (let k in pars){
        url+=delim;
        delim="&";
        url+=encodeURIComponent(k);
        url+="=";
        url+=encodeURIComponent(pars[k]);
    }
    return url;
}
const fetchJson=(url,pars)=>{
    let furl=buildUrl(url,pars);
    return fetch(furl).then((rs)=>rs.json());
}
const setVisible=(el,vis,useParent)=>{
    if (typeof(el) !== 'object') el=document.getElementById(el);
    if (! el) return;
    if (useParent) el=el.parentElement;
    if (! el) return;
    if (vis) el.classList.remove('hidden');
    else el.classList.add('hidden');
}
const enableEl=(id,en)=>{
    let el=document.getElementById(id);
    if (!el) return;
    if (en) el.disabled=false;
    else el.disabled=true;
}

export { getParam, addEl, forEachEl,setButtons,fillValues, setValue,setValues,buildUrl,fetchJson,setVisible, enableEl }