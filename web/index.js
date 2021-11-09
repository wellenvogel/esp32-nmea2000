let self = this;
let lastUpdate = (new Date()).getTime();
let reloadConfig = false;
function addEl(type, clazz, parent, text) {
    let el = document.createElement(type);
    if ( ! (clazz instanceof Array)){
        clazz=clazz.split(/  */);
    }
    clazz.forEach(function(ce){
        el.classList.add(ce);
    });
    if (text) el.textContent = text;
    if (parent) parent.appendChild(el);
    return el;
}
function forEl(query, callback,base) {
    if (! base) base=document;
    let all = base.querySelectorAll(query);
    for (let i = 0; i < all.length; i++) {
        callback(all[i]);
    }
}
function alertRestart() {
    reloadConfig = true;
    alert("Board reset triggered, reconnect WLAN if necessary");
}
function getJson(url) {
    return fetch(url)
        .then(function (r) { return r.json() });
}
function reset() {
    fetch('/api/reset');
    alertRestart();
}
function update() {
    let now = (new Date()).getTime();
    let ce = document.getElementById('connected');
    if (ce) {
        if ((lastUpdate + 3000) > now) {
            ce.classList.add('ok');
        }
        else {
            ce.classList.remove('ok');
        }
    }
    getJson('/api/status')
        .then(function (jsonData) {
            for (let k in jsonData) {
                if (typeof (jsonData[k]) === 'object') {
                    for (let sk in jsonData[k]) {
                        let key = k + "." + sk;
                        if (typeof (jsonData[k][sk]) === 'object') {
                            //msg details
                            updateMsgDetails(key, jsonData[k][sk]);
                        }
                        else {
                            let el = document.getElementById(key);
                            if (el) el.textContent = jsonData[k][sk];
                        }
                    }
                }
                else {
                    let el = document.getElementById(k);
                    if (el) el.textContent = jsonData[k];
                }
            }
            lastUpdate = (new Date()).getTime();
            if (reloadConfig) {
                reloadConfig = false;
                resetForm();
            }
        })
}
function resetForm(ev) {
    getJson("/api/config")
        .then(function (jsonData) {
            for (let k in jsonData) {
                let el = document.querySelector("[name='" + k + "']");
                if (el) {
                    let v = jsonData[k];
                    el.value = v;
                    el.setAttribute('data-loaded', v);
                    let changeEvent = new Event('change');
                    el.dispatchEvent(changeEvent);
                }
            }
        });
}
function checkMaxClients(v) {
    let parsed = parseInt(v);
    if (isNaN(parsed)) return "not a valid number";
    if (parsed < 0) return "must be >= 0";
    if (parsed > 10) return "max is 10";
}
function checkSystemName(v) {
    //2...32 characters for ssid
    let allowed = v.replace(/[^a-zA-Z0-9]*/g, '');
    if (allowed != v) return "contains invalid characters, only a-z, A-Z, 0-9";
    if (v.length < 2 || v.length > 32) return "invalid length (2...32)";
}
function checkApPass(v) {
    //min 8 characters
    if (v.length < 8) {
        return "password must be at least 8 characters";
    }
}
function changeConfig() {
    let url = "/api/setConfig?";
    let values = document.querySelectorAll('.configForm select , .configForm input');
    for (let i = 0; i < values.length; i++) {
        let v = values[i];
        let name = v.getAttribute('name');
        if (!name) continue;
        if (name.indexOf("_") >= 0) continue;
        let check = v.getAttribute('data-check');
        if (check) {
            if (typeof (self[check]) === 'function') {
                let res = self[check](v.value);
                if (res) {
                    let value = v.value;
                    if (v.type === 'password') value = "******";
                    alert("invalid config for " + v.getAttribute('name') + "(" + value + "):\n" + res);
                    return;
                }
            }
        }
        url += name + "=" + encodeURIComponent(v.value) + "&";
    }
    getJson(url)
        .then(function (status) {
            if (status.status == 'OK') {
                alertRestart();
            }
            else {
                alert("unable to set config: " + status.status);
            }
        })
}
function factoryReset() {
    if (!confirm("Really delete all configuration?\n" +
        "This will reset all your Wifi settings and disconnect you.")) {
        return;
    }
    getJson("/api/resetConfig")
        .then(function (status) {
            alertRestart();
        })
}
function createCounterDisplay(parent,label,key,isEven){
    let clazz="row icon-row counter-row";
    if (isEven) clazz+=" even";
    let row=addEl('div',clazz,parent);
    let icon=addEl('span','icon icon-more',row);
    addEl('span','label',row,label);
    let value=addEl('span','value',row,'---');
    value.setAttribute('id',key+".sumOk");
    let display=addEl('div',clazz+" msgDetails hidden",parent);
    display.setAttribute('id',key+".ok");
    row.addEventListener('click',function(ev){
        let rs=display.classList.toggle('hidden');
        if (rs){
            icon.classList.add('icon-more');
            icon.classList.remove('icon-less');
        }
        else{
            icon.classList.remove('icon-more');
            icon.classList.add('icon-less');
        }
    });
}

function updateMsgDetails(key, details) {
    forEl('.msgDetails', function (frame) {
        if (frame.getAttribute('id') !== key) return;
        for (let k in details) {
            let el = frame.querySelector("[data-id=\"" + k + "\"] ");
            if (!el) {
                el = addEl('div', 'row', frame);
                let cv = addEl('span', 'label', el, k);
                cv = addEl('span', 'value', el, details[k]);
                cv.setAttribute('data-id', k);
            }
            else {
                el.textContent = details[k];
            }
        }
        forEl('.value',function(el){
            let k=el.getAttribute('data-id');
            if (k && ! details[k]){
                el.parentElement.remove();
            }
        },frame);
    });
}
let counters={
    count2Kin: 'NMEA2000 in',
    count2Kout: 'NMEA2000 out',
    countTCPin: 'TCP in',
    countTCPout: 'TCP out',
    countUSBin: 'USB in',
    countUSBout: 'USB out',
    countSerialIn: 'Serial in',
    countSerialOut: 'Serial out'
}
function showOverlay(text, isHtml) {
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
}
function hideOverlay() {
    let container = document.getElementById('overlayContainer');
    container.classList.add('hidden');
}
function checkChange(el, row) {
    let loaded = el.getAttribute('data-loaded');
    if (loaded !== undefined) {
        if (loaded != el.value) {
            row.classList.add('changed');
        }
        else {
            row.classList.remove("changed");
        }
    }
}
function createInput(configItem, frame) {
    let el;
    if (configItem.type === 'boolean' || configItem.type === 'list') {
        el = document.createElement('select')
        el.setAttribute('name', configItem.name)
        let slist = [];
        if (configItem.list) {
            configItem.list.forEach(function (v) {
                slist.push({ l: v, v: v });
            })
        }
        else {
            slist.push({ l: 'on', v: 'true' })
            slist.push({ l: 'off', v: 'false' })
        }
        slist.forEach(function (sitem) {
            let sitemEl = document.createElement('option');
            sitemEl.setAttribute('value', sitem.v);
            sitemEl.textContent = sitem.l;
            el.appendChild(sitemEl);
        })
        frame.appendChild(el);
        return el;
    }
    if (configItem.type === 'filter') {
        el = document.createElement('div');
        el.classList.add('filter');
        let ais = createInput({
            type: 'list',
            name: configItem.name + "_ais",
            list: ['aison', 'aisoff']
        }, el);
        let mode = createInput({
            type: 'list',
            name: configItem.name + "_mode",
            list: ['whitelist', 'blacklist']
        }, el);
        let sentences = createInput({
            type: 'text',
            name: configItem.name + "_sentences",
        }, el);
        let data = document.createElement('input');
        data.setAttribute('type', 'hidden');
        el.appendChild(data);
        let changeFunction = function () {
            let cv = data.value || "";
            let parts = cv.split(":");
            ais.value = (parts[0] == '0') ? "aisoff" : "aison";
            mode.value = (parts[1] == '0') ? "whitelist" : "blacklist";
            sentences.value = parts[2] || "";
        }
        let updateFunction = function () {
            let nv = (ais.value == 'aison') ? "1" : "0";
            nv += ":";
            nv += (mode.value == 'blacklist') ? "1" : "0";
            nv += ":";
            nv += sentences.value;
            data.value = nv;
            let chev = new Event('change');
            data.dispatchEvent(chev);
        }
        mode.addEventListener('change', updateFunction);
        ais.addEventListener("change", updateFunction);
        sentences.addEventListener("change", updateFunction);
        data.addEventListener('change', function (ev) {
            changeFunction();
        });
        data.setAttribute('name', configItem.name);
        frame.appendChild(el);
        return data;
    }
    el = document.createElement('input');
    el.setAttribute('name', configItem.name)
    frame.appendChild(el);
    if (configItem.type === 'password') {
        el.setAttribute('type', 'password');
        let vis = addEl('span', 'icon-eye icon', frame);
        vis.addEventListener('click', function (v) {
            if (vis.classList.toggle('active')) {
                el.setAttribute('type', 'text');
            }
            else {
                el.setAttribute('type', 'password');
            }
        });
    }
    else if (configItem.type === 'number') {
        el.setAttribute('type', 'number');
    }
    else {
        el.setAttribute('type', 'text');
    }
    return el;
}
let configDefinitions;
function loadConfigDefinitions() {
    getJson("api/capabilities")
        .then(function (capabilities) {
            getJson("config.json")
                .then(function (defs) {
                    let category;
                    let categoryEl;
                    let frame = document.querySelector('.configFormRows');
                    if (!frame) throw Error("no config form");
                    frame.innerHTML = '';
                    configDefinitions = defs;
                    defs.forEach(function (item) {
                        if (!item.type) return;
                        if (item.category != category || !categoryEl) {
                            let categoryFrame = addEl('div', 'category', frame);
                            let categoryTitle = addEl('div', 'title', categoryFrame);
                            let categoryButton = addEl('span', 'icon icon-more', categoryTitle);
                            addEl('span', 'label', categoryTitle, item.category);
                            categoryEl = addEl('div', 'content', categoryFrame);
                            categoryEl.classList.add('hidden');
                            let currentEl = categoryEl;
                            categoryTitle.addEventListener('click', function (ev) {
                                let rs = currentEl.classList.toggle('hidden');
                                if (rs) {
                                    categoryButton.classList.add('icon-more');
                                    categoryButton.classList.remove('icon-less');
                                }
                                else {
                                    categoryButton.classList.remove('icon-more');
                                    categoryButton.classList.add('icon-less');
                                }
                            })
                            category = item.category;
                        }
                        if (item.capabilities !== undefined) {
                            for (let capability in item.capabilities) {
                                let values = item.capabilities[capability];
                                if (!capabilities[capability]) return;
                                let found = false;
                                values.forEach(function (v) {
                                    if (capabilities[capability] == v) found = true;
                                });
                                if (!found) return;
                            }
                        }
                        let row = addEl('div', 'row', categoryEl);
                        let label = item.label || item.name;
                        addEl('span', 'label', row, label);
                        let valueFrame = addEl('div', 'value', row);
                        let valueEl = createInput(item, valueFrame);
                        if (!valueEl) return;
                        valueEl.setAttribute('data-default', item.default);
                        valueEl.addEventListener('change', function (ev) {
                            let el = ev.target;
                            checkChange(el, row);
                        })
                        if (item.check) valueEl.setAttribute('data-check', item.check);
                        let btContainer = addEl('div', 'buttonContainer', row);
                        let bt = addEl('button', 'defaultButton', btContainer, 'X');
                        bt.setAttribute('data-default', item.default);
                        bt.addEventListener('click', function (ev) {
                            valueEl.value = valueEl.getAttribute('data-default');
                            let changeEvent = new Event('change');
                            valueEl.dispatchEvent(changeEvent);
                        })
                        bt = addEl('button', 'infoButton', btContainer, '?');
                        bt.addEventListener('click', function (ev) {
                            showOverlay(item.description);
                        });
                    })
                    resetForm();
                })
        })
        .catch(function (err) { alert("unable to load config: " + err) })
}
function converterInfo() {
    getJson("api/converterInfo").then(function (json) {
        let text = "<h3>Converted entities</h3>";
        text += "<p><b>NMEA0183 to NMEA2000:</b><br/>";
        text += "   " + (json.nmea0183 || "").replace(/,/g, ", ");
        text += "</p>";
        text += "<p><b>NMEA2000 to NMEA0183:</b><br/>";
        text += "   " + (json.nmea2000 || "").replace(/,/g, ", ");
        text += "</p>";
        showOverlay(text, true);
    });
}
function handleTab(el) {
    let activeName = el.getAttribute('data-page');
    if (!activeName) return;
    let activeTab = document.getElementById(activeName);
    if (!activeTab) return;
    let all = document.querySelectorAll('.tabPage');
    for (let i = 0; i < all.length; i++) {
        all[i].classList.add('hidden');
    }
    let tabs = document.querySelectorAll('.tab');
    for (let i = 0; i < all.length; i++) {
        tabs[i].classList.remove('active');
    }
    el.classList.add('active');
    activeTab.classList.remove('hidden');
}
function createDashboardItem(name, def, parent) {
    let frame = addEl('div', 'dash', parent);
    let title = addEl('span', 'dashTitle', frame, name);
    let value = addEl('span', 'dashValue', frame);
    value.setAttribute('id', 'data_' + name);
    return value;
}
function createDashboard() {
    let frame = document.getElementById('dashboardPage');
    if (!frame) return;
    getJson("api/boatData").then(function (json) {
        frame.innerHTML = '';
        for (let n in json) {
            createDashboardItem(n, json[n], frame);
        }
        updateDashboard(json);
    });
}
let valueFormatters = {
    formatCourse: function (v) { 
        let x = parseFloat(v); 
        let rt=x*180.0 / Math.PI;
        if (rt > 360) rt -= 360;
        if (rt < 0) rt += 360;
        return rt.toFixed(0); 
    },
    formatKnots: function (v) { 
        let x = parseFloat(v); 
        x=x *3600.0/1852.0;
        return x.toFixed(2); 
    },
    formatWind: function (v) { 
        let x = parseFloat(v); 
        x=x*180.0 / Math.PI;
        if (x > 180) x=180-x; 
        return x.toFixed(0); 
    },
    mtr2nm: function (v) { 
        let x = parseFloat(v); 
        x=x/1852.0;
        return x.toFixed(2); 
    },
    kelvinToC: function (v) { 
        let x = parseFloat(v); 
        x=x-273.15;
        return x.toFixed(0); 
    },
    formatFixed0: function (v) { 
        let x = parseFloat(v); 
        return x.toFixed(0); 
    },
    formatDepth: function (v) { 
        let x = parseFloat(v); 
        return x.toFixed(1); 
    },
    formatLatitude: function(v){
        let x = parseFloat(v); 
        return x.toFixed(4);
    },
    formatLongitued: function(v){
        let x = parseFloat(v); 
        return x.toFixed(4);
    },
}
function updateDashboard(data) {
    for (let n in data) {
        let de = document.getElementById('data_' + n);
        if (de) {
            if (data[n].valid) {
                let formatter;
                if (data[n].format && data[n].format != "NULL") {
                    let key = data[n].format.replace(/^\&/, '');
                    formatter = valueFormatters[key];
                }
                if (formatter) {
                    de.textContent = formatter(data[n].value);
                }
                else {
                    let v = parseFloat(data[n].value);
                    if (!isNaN(v)) {
                        v = v.toFixed(3)
                        de.textContent = v;
                    }
                    else {
                        de.textContent = data[n].value;
                    }
                }
            }
            else de.textContent = "---";
        }
    }
}

window.setInterval(update, 1000);
window.setInterval(function () {
    let dp = document.getElementById('dashboardPage');
    if (dp.classList.contains('hidden')) return;
    getJson('api/boatData').then(function (data) {
        updateDashboard(data);
    });
}, 1000);
window.addEventListener('load', function () {
    let buttons = document.querySelectorAll('button');
    for (let i = 0; i < buttons.length; i++) {
        let be = buttons[i];
        be.onclick = window[be.id]; //assume a function with the button id
    }
    forEl('.showMsgDetails', function (cd) {
        cd.addEventListener('change', function (ev) {
            let key = ev.target.getAttribute('data-key');
            if (!key) return;
            let el = document.getElementById(key);
            if (!el) return;
            if (ev.target.checked) el.classList.remove('hidden');
            else (el.classList).add('hidden');
        });
    });
    let tabs = document.querySelectorAll('.tab');
    for (let i = 0; i < tabs.length; i++) {
        tabs[i].addEventListener('click', function (ev) {
            handleTab(ev.target);
        });
    }
    loadConfigDefinitions();
    createDashboard();
    let statusPage=document.getElementById('statusPageContent');
    if (statusPage){
        let even=true;
        for (let c in counters){
            createCounterDisplay(statusPage,counters[c],c,even);
            even=!even;
        }
    }
});