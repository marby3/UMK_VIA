// ----------------------------------------------------
// UIAPduino Builder App (Firmware & Layout Editor)
// ----------------------------------------------------

const UNIT_SIZE = 54; // 1U = 54px

// --- State ---
let keys = [];
let selectedKeyIds = [];
let keyIdCounter = 0;

// --- DOM Elements ---
// Tabs
const builderTabs = document.querySelectorAll('#view-dev .builder-tab');
const tabPanes = document.querySelectorAll('#view-dev .tab-pane');

// Hardware Config
const CH32V003_PINS = [
    'PA1', 'PA2', 'PC0', 'PC1', 'PC2', 'PC3', 'PC4', 'PC5', 'PC6', 'PC7',
    'PD0', 'PD1', 'PD2', 'PD3', 'PD4', 'PD5', 'PD6', 'PD7'
];
const elDevName = document.getElementById('devName');
const elDevVid = document.getElementById('devVid');
const elDevPid = document.getElementById('devPid');
const elDevRows = document.getElementById('devRows');
const elDevCols = document.getElementById('devCols');
const elMatrixPinsContainer = document.getElementById('matrixPinsContainer');

const elEnableRGB = document.getElementById('enableRGB');
const elRgbConfig = document.getElementById('rgbConfig');
const elPinRgbDin = document.getElementById('pinRgbDin');

const btnBuildFirmware = document.getElementById('btnBuildFirmware');
const btnDownloadFw = document.getElementById('btnDownloadFw');
const buildLog = document.getElementById('buildLog');
const matrixWarning = document.getElementById('matrixWarning');

// Layout Editor
const elCanvas = document.getElementById('layoutCanvas');
const elCanvasContainer = document.getElementById('layoutCanvasContainer');
const btnAddKey = document.getElementById('btnAddKey');
const btnDeleteKey = document.getElementById('btnDeleteKey');
const btnDownloadJson = document.getElementById('btnDownloadJson');
const uploadJson = document.getElementById('uploadJson');

const matrixSvgLayer = document.getElementById('matrixSvgLayer');
const checkMatrixRow = document.getElementById('checkMatrixRow');
const checkMatrixCol = document.getElementById('checkMatrixCol');

// Property Panel
const propW = document.getElementById('propW');
const propH = document.getElementById('propH');
const propX = document.getElementById('propX');
const propY = document.getElementById('propY');
const propR = document.getElementById('propR');
const propRx = document.getElementById('propRx');
const propRy = document.getElementById('propRy');
const propRow = document.getElementById('propRow');
const propCol = document.getElementById('propCol');
const propLabel = document.getElementById('propLabel');
const allProps = [propW, propH, propX, propY, propR, propRx, propRy, propRow, propCol, propLabel];

const layoutHelpModal = document.getElementById('layoutHelpModal');
const layoutHelpClose = document.getElementById('layoutHelpClose');
const shortcutHelpIcon = document.getElementById('shortcutHelpIcon');

// --- Tab Logic ---
builderTabs.forEach(tab => {
    tab.addEventListener('click', () => {
        builderTabs.forEach(t => t.classList.remove('active'));
        tabPanes.forEach(p => p.classList.add('hidden'));
        tab.classList.add('active');
        document.getElementById('pane-' + tab.dataset.pane).classList.remove('hidden');
        if (tab.dataset.pane === 'layout') {
            renderCanvas();
        } else if (tab.dataset.pane === 'hw') {
            checkMatrixConsistency();
        }
    });
});

// --- Hardware Setup Logic ---
function createPinSelect(id) {
    let sel = document.createElement('select');
    sel.className = 'modern-select pin-select';
    if(id) sel.id = id;
    let opt = document.createElement('option');
    opt.value = ""; opt.textContent = "未割当";
    sel.appendChild(opt);
    CH32V003_PINS.forEach(p => {
        let o = document.createElement('option');
        o.value = p; o.textContent = p;
        sel.appendChild(o);
    });
    return sel;
}

function updateMatrixPinsUI() {
    const rows = parseInt(elDevRows.value) || 0;
    const cols = parseInt(elDevCols.value) || 0;
    elMatrixPinsContainer.innerHTML = '';
    
    for(let r=0; r<rows; r++){
        const rowDiv = document.createElement('div');
        rowDiv.className = 'pin-row';
        rowDiv.innerHTML = `<span class="pin-label">Row${r}</span>`;
        rowDiv.appendChild(createPinSelect(`pin_row_${r}`));
        elMatrixPinsContainer.appendChild(rowDiv);
    }
    for(let c=0; c<cols; c++){
        const colDiv = document.createElement('div');
        colDiv.className = 'pin-row';
        colDiv.innerHTML = `<span class="pin-label">Col${c}</span>`;
        colDiv.appendChild(createPinSelect(`pin_col_${c}`));
        elMatrixPinsContainer.appendChild(colDiv);
    }
    checkMatrixConsistency();
}

function checkMatrixConsistency() {
    const hwR = parseInt(elDevRows.value) || 0;
    const hwC = parseInt(elDevCols.value) || 0;
    const maxR = keys.length > 0 ? Math.max(...keys.map(k=>k.row||0)) : -1;
    const maxC = keys.length > 0 ? Math.max(...keys.map(k=>k.col||0)) : -1;

    // 常に警告は出さず、内部ロジックやビルド時に呼び出す
    return { maxR, maxC, hwR, hwC };
}

// Selectボックス(Label)をQMKキーコード一覧で満たす
if (typeof QMK_KEYCODES !== 'undefined') {
    for (const [code, label] of Object.entries(QMK_KEYCODES)) {
        const option = document.createElement('option');
        option.value = label;
        option.textContent = `${label} (0x${parseInt(code).toString(16).padStart(2, '0').toUpperCase()})`;
        propLabel.appendChild(option);
    }
}

elDevRows.addEventListener('change', updateMatrixPinsUI);
elDevCols.addEventListener('change', updateMatrixPinsUI);
elEnableRGB.addEventListener('change', (e) => {
    if(e.target.checked) elRgbConfig.classList.remove('hidden');
    else elRgbConfig.classList.add('hidden');
});
elPinRgbDin.replaceWith(createPinSelect('pinRgbDin'));
updateMatrixPinsUI();

function appendLog(str) {
    buildLog.textContent += '\n' + str;
    buildLog.parentElement.scrollTop = buildLog.parentElement.scrollHeight;
}
btnBuildFirmware.addEventListener('click', async () => {
    // Check Matrix
    const { maxR, maxC, hwR, hwC } = checkMatrixConsistency();
    if (maxR >= hwR || maxC >= hwC) {
        matrixWarning.classList.remove('hidden');
        buildLog.textContent = '> Build Failed: Matrix Config Mismatch!\n(レイアウトのRow/ColがHardware設定の最大値を超えています)';
        return;
    } else {
        matrixWarning.classList.add('hidden');
    }

    buildLog.textContent = '> Starting Local Build Environment Process...\n> Requesting build from Local Server...';
    btnBuildFirmware.disabled = true;
    btnDownloadFw.classList.add('hidden');
    
    // Gather Pin Configurations
    const rowPins = [];
    for (let r = 0; r < hwR; r++) {
        const sel = document.getElementById(`pin_row_${r}`);
        rowPins.push(sel ? sel.value : "");
    }
    const colPins = [];
    for (let c = 0; c < hwC; c++) {
        const sel = document.getElementById(`pin_col_${c}`);
        colPins.push(sel ? sel.value : "");
    }
    
    const requestData = {
        name: elDevName.value,
        vid: elDevVid.value,
        pid: elDevPid.value,
        rows: hwR,
        cols: hwC,
        row_pins: rowPins,
        col_pins: colPins
    };

    try {
        appendLog('Sending configuration to local server...');
        const response = await fetch('/api/build', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(requestData)
        });

        if (response.ok) {
            appendLog('Server compiled successfully! Downloading main.bin...');
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            
            // Cache the file into memory for the Flash tab to use
            const arrayBuffer = await blob.arrayBuffer();
            window.UIAPduinoCache = window.UIAPduinoCache || {};
            window.UIAPduinoCache.firmwareBin = new Uint8Array(arrayBuffer);
            
            btnBuildFirmware.disabled = false;
            btnDownloadFw.style.display = 'inline-block';
            btnDownloadFw.classList.remove('hidden');
            btnDownloadFw.href = url;
            btnDownloadFw.download = "main.bin";
            appendLog('Build SUCCESS 🎉 (Cached for Flasher)');
        } else {
            const errText = await response.text();
            appendLog(`Build failed on server:\n${errText}`);
            btnBuildFirmware.disabled = false;
        }
    } catch (err) {
        appendLog(`Network or Server error:\n${err.message}\n(Is build_server.py running?)`);
        btnBuildFirmware.disabled = false;
    }
});

// --- Layout Editor Logic ---
let isDraggingKeys = false;
let isDrawingRect = false;

let dragStartX = 0, dragStartY = 0;
let draggedKeysInitialState = [];

let selectionBoxEl = null;

function renderCanvas() {
    // Keep SVG node
    const svgChild = matrixSvgLayer;
    elCanvas.innerHTML = '';
    elCanvas.appendChild(svgChild);
    
    keys.forEach(k => {
        const div = document.createElement('div');
        div.className = 'layout-key';
        if (selectedKeyIds.includes(k.id)) div.classList.add('selected');
        
        div.style.left = (k.x * UNIT_SIZE) + 'px';
        div.style.top = (k.y * UNIT_SIZE) + 'px';
        div.style.width = (k.w * UNIT_SIZE) + 'px';
        div.style.height = (k.h * UNIT_SIZE) + 'px';
        
        if (k.r) {
            div.style.transform = `rotate(${k.r}deg)`;
            let originX = ((k.rx || 0) - k.x) * UNIT;
            let originY = ((k.ry || 0) - k.y) * UNIT;
            div.style.transformOrigin = `${originX}px ${originY}px`;
            
            const marker = document.createElement('div');
            marker.className = 'origin-marker';
            marker.style.left = ((k.rx || 0) * UNIT) + 'px';
            marker.style.top = ((k.ry || 0) * UNIT) + 'px';
            elCanvas.appendChild(marker);
        }

        // --- 特殊形状 (ISO Enter等) 対応 ---
        if (k.w2 > 0 || k.h2 > 0) {
            const shp = document.createElement('div');
            shp.className = 'key-shape-extension';
            shp.style.left = ((k.x2 || 0) * UNIT) + 'px';
            shp.style.top = ((k.y2 || 0) * UNIT) + 'px';
            shp.style.width = ((k.w2 || k.w) * UNIT) + 'px';
            shp.style.height = ((k.h2 || k.h) * UNIT) + 'px';
            div.appendChild(shp);
        }

        const labelText = k.label || '';
        const matrixText = `<div style="font-size: 0.65rem; color: #888; pointer-events:none;">[${k.row||0},${k.col||0}]</div>`;
        div.innerHTML = labelText.replace(/\n/g, '<br>') + (labelText ? '<br>' : '') + matrixText;

        div.addEventListener('mousedown', (e) => startKeyDrag(e, k.id));
        elCanvas.appendChild(div);
        
        // Draw Origin Marker for selected items if they have rotation origin
        if (selectedKeyIds.includes(k.id) && (k.rx !== undefined || k.ry !== undefined)) {
             const marker = document.createElement('div');
             marker.className = 'origin-marker';
             marker.style.left = ((k.rx||0) * UNIT_SIZE) + 'px';
             marker.style.top = ((k.ry||0) * UNIT_SIZE) + 'px';
             elCanvas.appendChild(marker);
        }
    });

    drawMatrixLines();

    if (selectedKeyIds.length > 0) {
        btnDeleteKey.disabled = false;
        allProps.forEach(p => p.disabled = false);
        
        if (selectedKeyIds.length === 1) {
            const k = keys.find(x => x.id === selectedKeyIds[0]);
            propW.value = k.w; propH.value = k.h;
            propX.value = k.x; propY.value = k.y;
            propR.value = k.r || 0; propRx.value = k.rx || 0; propRy.value = k.ry || 0;
            propRow.value = k.row || 0; propCol.value = k.col || 0;
            propLabel.value = k.label || '';
        } else {
            // Multiple Selection
            allProps.forEach(p => p.value = ''); // Clear all
            // option contains (Multiple Selected)
            propLabel.options[0].text = "(Multiple Selected)";
            propLabel.value = "";
            propLabel.disabled = true; // Disable label edit for multiple
        }
    } else {
        btnDeleteKey.disabled = true;
        allProps.forEach(p => { p.disabled = true; p.value = ''; });
        if(propLabel.options[0]) propLabel.options[0].text = "未設定";
    }
}

function drawMatrixLines() {
    matrixSvgLayer.innerHTML = '';
    const drawRow = checkMatrixRow.checked;
    const drawCol = checkMatrixCol.checked;
    if (!drawRow && !drawCol) return;

    const createPolyLine = (points, color) => {
        if(points.length < 2) return;
        const pts = points.map(p => `${p.x},${p.y}`).join(" ");
        const poly = document.createElementNS("http://www.w3.org/2000/svg", "polyline");
        poly.setAttribute("points", pts);
        poly.setAttribute("fill", "none");
        poly.setAttribute("stroke", color);
        poly.setAttribute("stroke-width", "2");
        poly.setAttribute("stroke-dasharray", "4");
        matrixSvgLayer.appendChild(poly);
    };

    const getCenter = (k) => {
        // Simple center. Does not account for perfect rotated center, but good enough for visual guide.
        return { x: (k.x + k.w/2) * UNIT_SIZE, y: (k.y + k.h/2) * UNIT_SIZE };
    };

    if (drawRow) {
        let rowsMap = {};
        keys.forEach(k => { if(!rowsMap[k.row]) rowsMap[k.row] = []; rowsMap[k.row].push(k); });
        for (const [r, kArr] of Object.entries(rowsMap)) {
            // Sort by X
            kArr.sort((a,b)=>a.x - b.x);
            createPolyLine(kArr.map(getCenter), 'var(--secondary-color)');
        }
    }
    
    if (drawCol) {
        let colsMap = {};
        keys.forEach(k => { if(!colsMap[k.col]) colsMap[k.col] = []; colsMap[k.col].push(k); });
        for (const [c, kArr] of Object.entries(colsMap)) {
            // Sort by Y
            kArr.sort((a,b)=>a.y - b.y);
            createPolyLine(kArr.map(getCenter), 'var(--primary-color)');
        }
    }
}

checkMatrixRow.addEventListener('change', drawMatrixLines);
checkMatrixCol.addEventListener('change', drawMatrixLines);

function startKeyDrag(e, id) {
    if (e.button !== 0) return; 
    e.stopPropagation();

    if (e.ctrlKey || e.metaKey) {
        // Toggle selection
        if (selectedKeyIds.includes(id)) {
            selectedKeyIds = selectedKeyIds.filter(x => x !== id);
        } else {
            selectedKeyIds.push(id);
        }
        renderCanvas();
        return;
    }

    if (!selectedKeyIds.includes(id)) {
        selectedKeyIds = [id];
    }

    isDraggingKeys = true;
    dragStartX = e.clientX; 
    dragStartY = e.clientY;
    
    draggedKeysInitialState = selectedKeyIds.map(kid => {
        const k = keys.find(x => x.id === kid);
        return { id: kid, originalX: k.x, originalY: k.y };
    });

    renderCanvas();
    document.addEventListener('mousemove', onDragKeys);
    document.addEventListener('mouseup', endDragKeys);
}

function onDragKeys(e) {
    if (!isDraggingKeys) return;
    const dx = (e.clientX - dragStartX) / UNIT_SIZE;
    const dy = (e.clientY - dragStartY) / UNIT_SIZE;
    
    draggedKeysInitialState.forEach(st => {
        const k = keys.find(x => x.id === st.id);
        if(k) {
            k.x = Math.max(0, Math.round((st.originalX + dx) * 4) / 4);
            k.y = Math.max(0, Math.round((st.originalY + dy) * 4) / 4);
        }
    });
    renderCanvas();
}

function endDragKeys() {
    isDraggingKeys = false;
    document.removeEventListener('mousemove', onDragKeys);
    document.removeEventListener('mouseup', endDragKeys);
}

// Canvas Drag (Rectangle Selection)
elCanvas.addEventListener('mousedown', (e) => {
    if (e.button !== 0) return;
    if (e.target !== elCanvas && e.target !== matrixSvgLayer) return;
    if (!e.ctrlKey && !e.metaKey) {
        selectedKeyIds = [];
        renderCanvas();
    }
    
    isDrawingRect = true;
    const rect = elCanvas.getBoundingClientRect();
    dragStartX = e.clientX - rect.left + elCanvas.scrollLeft;
    dragStartY = e.clientY - rect.top + elCanvas.scrollTop;

    selectionBoxEl = document.createElement('div');
    selectionBoxEl.className = 'selection-box';
    selectionBoxEl.style.left = dragStartX + 'px';
    selectionBoxEl.style.top = dragStartY + 'px';
    selectionBoxEl.style.width = '0px';
    selectionBoxEl.style.height = '0px';
    elCanvas.appendChild(selectionBoxEl);

    document.addEventListener('mousemove', onDrawRect);
    document.addEventListener('mouseup', endDrawRect);
});

function onDrawRect(e) {
    if (!isDrawingRect) return;
    const rect = elCanvas.getBoundingClientRect();
    const curX = e.clientX - rect.left + elCanvas.scrollLeft;
    const curY = e.clientY - rect.top + elCanvas.scrollTop;

    const left = Math.min(dragStartX, curX);
    const top = Math.min(dragStartY, curY);
    const width = Math.abs(curX - dragStartX);
    const height = Math.abs(curY - dragStartY);

    selectionBoxEl.style.left = left + 'px';
    selectionBoxEl.style.top = top + 'px';
    selectionBoxEl.style.width = width + 'px';
    selectionBoxEl.style.height = height + 'px';
}

function endDrawRect(e) {
    if (!isDrawingRect) return;
    isDrawingRect = false;

    const rect = elCanvas.getBoundingClientRect();
    const curX = e.clientX - rect.left + elCanvas.scrollLeft;
    const curY = e.clientY - rect.top + elCanvas.scrollTop;

    const selLeft = Math.min(dragStartX, curX);
    const selTop = Math.min(dragStartY, curY);
    const selRight = Math.max(dragStartX, curX);
    const selBottom = Math.max(dragStartY, curY);

    // Filter intersect keys
    keys.forEach(k => {
        const kLeft = k.x * UNIT_SIZE;
        const kTop = k.y * UNIT_SIZE;
        const kRight = (k.x + k.w) * UNIT_SIZE;
        const kBottom = (k.y + k.h) * UNIT_SIZE;

        if (!(kLeft > selRight || kRight < selLeft || kTop > selBottom || kBottom < selTop)) {
            if (!selectedKeyIds.includes(k.id)) {
                selectedKeyIds.push(k.id);
            }
        }
    });

    if (selectionBoxEl && selectionBoxEl.parentNode) {
        selectionBoxEl.parentNode.removeChild(selectionBoxEl);
    }
    selectionBoxEl = null;

    document.removeEventListener('mousemove', onDrawRect);
    document.removeEventListener('mouseup', endDrawRect);
    renderCanvas();
}

btnAddKey.addEventListener('click', () => {
    let newX = 0, newY = 0, r = 0, rx = 0, ry = 0, row = 0, col = 0;
    
    if (selectedKeyIds.length === 1) {
        const sel = keys.find(x => x.id === selectedKeyIds[0]);
        if (sel) {
            newX = sel.x + sel.w; 
            newY = sel.y;
            r = sel.r || 0; rx = sel.rx || 0; ry = sel.ry || 0;
            row = sel.row || 0;
            col = (sel.col !== undefined) ? sel.col + 1 : 0; // Auto Col +1
        }
    } else {
        if (keys.length > 0) {
            newY = Math.max(...keys.map(k => k.y)) + 1;
            row = Math.max(...keys.map(k => k.row || 0)) + 1; // Auto Row +1
            col = 0;
        }
    }
    
    const k = {
        id: keyIdCounter++,
        x: newX, y: newY, w: 1, h: 1, r: r, rx: rx, ry: ry,
        row: row, col: col, label: ''
    };
    keys.push(k);
    selectedKeyIds = [k.id];
    renderCanvas();
    checkMatrixConsistency();
});

btnDeleteKey.addEventListener('click', () => {
    keys = keys.filter(k => !selectedKeyIds.includes(k.id));
    selectedKeyIds = [];
    renderCanvas();
    checkMatrixConsistency();
});

function updateSelectedProps() {
    if (selectedKeyIds.length === 0) return;
    
    selectedKeyIds.forEach(id => {
        const k = keys.find(x => x.id === id);
        if(!k) return;
        
        if (propW.value !== '') k.w = parseFloat(propW.value);
        if (propH.value !== '') k.h = parseFloat(propH.value);
        if (propX.value !== '') k.x = parseFloat(propX.value);
        if (propY.value !== '') k.y = parseFloat(propY.value);
        if (propR.value !== '') k.r = parseFloat(propR.value);
        if (propRx.value !== '') k.rx = parseFloat(propRx.value);
        if (propRy.value !== '') k.ry = parseFloat(propRy.value);
        if (propRow.value !== '') k.row = parseInt(propRow.value);
        if (propCol.value !== '') k.col = parseInt(propCol.value);
        
        if (selectedKeyIds.length === 1) {
            k.label = propLabel.value || '';
        }
    });

    renderCanvas();
    checkMatrixConsistency();
}
allProps.forEach(p => p.addEventListener('input', updateSelectedProps));

// --- JSON Import/Export ---
function generateKeyboardDefinition() {
    let kleRows = [];
    keys.forEach(k => {
        let yGroup = Math.floor(k.y); 
        if(!kleRows[yGroup]) kleRows[yGroup] = [];
        kleRows[yGroup].push(k);
    });
    kleRows = kleRows.filter(r => r !== undefined);

    let keymapArray = [];
    let curX = 0, curY = 0;

    kleRows.forEach((rowKeys, rIdx) => {
        rowKeys.sort((a,b) => a.x - b.x);
        let rowArray = [];
        let rowStartX = 0;
        
        rowKeys.forEach((k, idx) => {
            let props = {};
            if (k.w !== 1) props.w = k.w;
            if (k.h !== 1) props.h = k.h;
            if (k.x - rowStartX > 0) props.x = k.x - rowStartX;
            if (idx === 0 && (k.y - curY) > 0) props.y = k.y - curY;
            if (k.r) props.r = k.r;
            if (k.rx) props.rx = k.rx;
            if (k.ry) props.ry = k.ry;
            
            props.row = k.row || 0;
            props.col = k.col || 0;

            if (Object.keys(props).length > 0) {
                rowArray.push(props);
            }
            rowArray.push(k.label || "");
            rowStartX = k.x + k.w;
        });
        keymapArray.push(rowArray);
        curY++;
    });

    return {
        name: elDevName.value || "UIAPduino Custom Keyboard",
        vendorId: elDevVid.value || "0x1209",
        productId: elDevPid.value || "0xb803",
        matrix: {
            rows: parseInt(elDevRows.value) || 4,
            cols: parseInt(elDevCols.value) || 6
        },
        layouts: {
            keymap: keymapArray
        }
    };
}

btnDownloadJson.addEventListener('click', () => {
    const def = generateKeyboardDefinition();
    const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(def, null, 2));
    const dlAnchorElem = document.createElement('a');
    dlAnchorElem.setAttribute("href", dataStr);
    dlAnchorElem.setAttribute("download", "keyboard_definition.json");
    dlAnchorElem.click();
});

// btnDownloadFw Event Listener removed, handled directly via pseudo href

function parseKeyboardDefinition(def) {
    if(def.name) elDevName.value = def.name;
    if(def.vendorId) elDevVid.value = def.vendorId;
    if(def.productId) elDevPid.value = def.productId;
    if(def.matrix) {
        elDevRows.value = def.matrix.rows || 4;
        elDevCols.value = def.matrix.cols || 6;
        updateMatrixPinsUI();
    }
    
    let keymap = def.layouts && def.layouts.keymap ? def.layouts.keymap : def;
    if (!Array.isArray(keymap)) return false;
    
    let parsedKeys = [];
    let idCnt = 0;
    let curX = 0, curY = 0;
    
    let cProps = { w: 1, h: 1, x: 0, y: 0, r: 0, rx: 0, ry: 0, row: 0, col: 0, w2: 0, h2: 0, x2: 0, y2: 0 };
    
    for (let r = 0; r < keymap.length; r++) {
        let row = keymap[r];
        if (!Array.isArray(row)) continue;
        
        for (let i = 0; i < row.length; i++) {
            let item = row[i];
            if (typeof item === 'string') {
                curX += cProps.x;
                curY += cProps.y;
                
                parsedKeys.push({
                    id: idCnt++,
                    x: curX, y: curY, w: cProps.w, h: cProps.h,
                    r: cProps.r, rx: cProps.rx, ry: cProps.ry,
                    w2: cProps.w2, h2: cProps.h2, x2: cProps.x2, y2: cProps.y2,
                    row: cProps.row || 0, col: cProps.col || 0,
                    label: item
                });
                
                curX += cProps.w;
                cProps.w = 1; cProps.h = 1; cProps.x = 0; cProps.y = 0;
                cProps.w2 = 0; cProps.h2 = 0; cProps.x2 = 0; cProps.y2 = 0;
            } else if (typeof item === 'object') {
                Object.assign(cProps, item);
            }
        }
        curY++;
        curX = 0;
    }
    
    keys = parsedKeys;
    keyIdCounter = idCnt;
    selectedKeyIds = [];
    renderCanvas();
    checkMatrixConsistency();
    return true;
}

uploadJson.addEventListener('change', (e) => {
    const file = e.target.files[0];
    if(!file) return;
    const reader = new FileReader();
    reader.onload = (evt) => {
        try {
            const def = JSON.parse(evt.target.result);
            if (parseKeyboardDefinition(def)) {
                alert("Definition loaded successfully.");
            } else {
                alert("Invalid Definition Format");
            }
        } catch(err) {
            alert("Error parsing JSON");
        }
        uploadJson.value = '';
    };
    reader.readAsText(file);
});

// Shortcuts
document.addEventListener('keydown', (e) => {
    const devView = document.getElementById('view-dev');
    if (!devView || devView.classList.contains('hidden')) return;
    const layoutPane = document.getElementById('pane-layout');
    if(layoutPane.classList.contains('hidden')) return;

    if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;

    if (e.key === 'Escape') { selectedKeyIds = []; renderCanvas(); return; }
    if (e.key === 'Insert' || e.key === 'i') { btnAddKey.click(); return; }
    if (e.key === 'Delete') { if (selectedKeyIds.length > 0) btnDeleteKey.click(); return; }
    
    // Quick focus change handles single selection
    if (e.key === 'j') {
        if(keys.length>0) {
            const currentId = selectedKeyIds[selectedKeyIds.length-1];
            let idx = keys.findIndex(k => k.id === currentId);
            idx = idx <= 0 ? keys.length - 1 : idx - 1;
            selectedKeyIds = [keys[idx].id]; renderCanvas();
        }
        return;
    }
    if (e.key === 'k') {
         if(keys.length>0) {
            const currentId = selectedKeyIds[selectedKeyIds.length-1];
            let idx = keys.findIndex(k => k.id === currentId);
            idx = (idx === -1 || idx >= keys.length - 1) ? 0 : idx + 1;
            selectedKeyIds = [keys[idx].id]; renderCanvas();
        }
        return;
    }

    if (selectedKeyIds.length > 0) {
        let moved = false; const step = 0.25;
        selectedKeyIds.forEach(id => {
            const k = keys.find(x => x.id === id);
            if (!k) return;
            if (e.shiftKey) {
                if (e.key === 'ArrowRight') { k.w += step; moved = true; }
                if (e.key === 'ArrowLeft') { k.w = Math.max(0.25, k.w - step); moved = true; }
                if (e.key === 'ArrowUp') { k.h = Math.max(0.25, k.h - step); moved = true; }
                if (e.key === 'ArrowDown') { k.h += step; moved = true; }
            } else {
                if (e.key === 'ArrowRight') { k.x += step; moved = true; }
                if (e.key === 'ArrowLeft') { k.x = Math.max(0, k.x - step); moved = true; }
                if (e.key === 'ArrowUp') { k.y = Math.max(0, k.y - step); moved = true; }
                if (e.key === 'ArrowDown') { k.y += step; moved = true; }
            }
        });
        if (moved) { e.preventDefault(); renderCanvas(); checkMatrixConsistency(); }
    }
});

shortcutHelpIcon.addEventListener('click', () => layoutHelpModal.classList.remove('hidden'));
layoutHelpClose.addEventListener('click', () => layoutHelpModal.classList.add('hidden'));
layoutHelpModal.addEventListener('click', (e) => {
    if (e.target === layoutHelpModal) layoutHelpModal.classList.add('hidden');
});

// Start
renderCanvas();
