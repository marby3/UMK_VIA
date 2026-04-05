// Layout Editor Logic
const UNIT_SIZE = 54; // 1U = 54px

const elCanvas = document.getElementById('layoutCanvas');
const btnAddKey = document.getElementById('btnAddKey');
const btnDeleteKey = document.getElementById('btnDeleteKey');
const btnDownloadJson = document.getElementById('btnDownloadJson');
const uploadJson = document.getElementById('uploadJson');

const keyPropsPanel = document.getElementById('keyPropsPanel');
const propW = document.getElementById('propW');
const propH = document.getElementById('propH');
const propX = document.getElementById('propX');
const propY = document.getElementById('propY');
const propLabel = document.getElementById('propLabel');

const layoutHelpModal = document.getElementById('layoutHelpModal');
const layoutHelpClose = document.getElementById('layoutHelpClose');

let keys = [];
let selectedKeyId = null;
let keyIdCounter = 0;

// Dragging state
let isDragging = false;
let dragStartX = 0;
let dragStartY = 0;
let dragKeyStartX = 0;
let dragKeyStartY = 0;

function renderCanvas() {
    elCanvas.innerHTML = '';
    keys.forEach(k => {
        const div = document.createElement('div');
        div.className = 'layout-key';
        if (k.id === selectedKeyId) div.classList.add('selected');
        
        div.style.left = (k.x * UNIT_SIZE) + 'px';
        div.style.top = (k.y * UNIT_SIZE) + 'px';
        div.style.width = (k.w * UNIT_SIZE) + 'px';
        div.style.height = (k.h * UNIT_SIZE) + 'px';
        
        // Render label nicely
        const labelText = k.label || `[${k.row || 0},${k.col || 0}]`;
        div.innerHTML = labelText.replace(/\n/g, '<br>');

        div.addEventListener('mousedown', (e) => startDrag(e, k.id));
        elCanvas.appendChild(div);
    });

    const k = keys.find(x => x.id === selectedKeyId);
    if (k) {
        btnDeleteKey.disabled = false;
        propW.disabled = false; propH.disabled = false;
        propX.disabled = false; propY.disabled = false;
        propLabel.disabled = false;
        
        propW.value = k.w;
        propH.value = k.h;
        propX.value = k.x;
        propY.value = k.y;
        propLabel.value = k.label || '';
    } else {
        btnDeleteKey.disabled = true;
        propW.disabled = true; propH.disabled = true;
        propX.disabled = true; propY.disabled = true;
        propLabel.disabled = true;
        
        propW.value = ''; propH.value = '';
        propX.value = ''; propY.value = '';
        propLabel.value = '';
    }
}

function startDrag(e, id) {
    if (e.button !== 0) return; // Only left click
    e.stopPropagation();
    selectedKeyId = id;
    const k = keys.find(x => x.id === id);
    if(!k) return;

    isDragging = true;
    dragStartX = e.clientX;
    dragStartY = e.clientY;
    dragKeyStartX = k.x;
    dragKeyStartY = k.y;

    renderCanvas();
    
    document.addEventListener('mousemove', onDrag);
    document.addEventListener('mouseup', endDrag);
}

function onDrag(e) {
    if (!isDragging) return;
    const dx = (e.clientX - dragStartX) / UNIT_SIZE;
    const dy = (e.clientY - dragStartY) / UNIT_SIZE;
    
    const k = keys.find(x => x.id === selectedKeyId);
    if(k) {
        // Snap to grid (0.25U)
        k.x = Math.max(0, Math.round((dragKeyStartX + dx) * 4) / 4);
        k.y = Math.max(0, Math.round((dragKeyStartY + dy) * 4) / 4);
        renderCanvas();
    }
}

function endDrag() {
    isDragging = false;
    document.removeEventListener('mousemove', onDrag);
    document.removeEventListener('mouseup', endDrag);
}

elCanvas.addEventListener('mousedown', () => {
    selectedKeyId = null;
    renderCanvas();
});

btnAddKey.addEventListener('click', () => {
    let newX = 0, newY = 0;
    
    if (selectedKeyId !== null) {
        const sel = keys.find(x => x.id === selectedKeyId);
        if (sel) {
            newX = sel.x + sel.w;
            newY = sel.y;
        }
    } else {
        if (keys.length > 0) {
            // Find max Y
            let maxY = Math.max(...keys.map(k => k.y));
            // Find keys at maxY
            let keysAtMaxY = keys.filter(k => k.y === maxY);
            // new pos = slightly offset from max bounds? No, user says: "Y方向にずらして配置" (Shift in Y direction)
            // KLE typically places on next row unless it overlaps. Let's just do maxY + 1
            newY = maxY + 1;
            newX = 0;
        }
    }

    const k = {
        id: keyIdCounter++,
        x: newX, y: newY, w: 1, h: 1,
        label: ''
    };
    keys.push(k);
    selectedKeyId = k.id;
    renderCanvas();
});

btnDeleteKey.addEventListener('click', () => {
    keys = keys.filter(k => k.id !== selectedKeyId);
    selectedKeyId = null;
    renderCanvas();
});

function updateSelectedProps() {
    const k = keys.find(x => x.id === selectedKeyId);
    if(k) {
        k.w = parseFloat(propW.value) || 1;
        k.h = parseFloat(propH.value) || 1;
        k.x = parseFloat(propX.value) || 0;
        k.y = parseFloat(propY.value) || 0;
        k.label = propLabel.value || '';
        renderCanvas();
    }
}

propW.addEventListener('input', updateSelectedProps);
propH.addEventListener('input', updateSelectedProps);
propX.addEventListener('input', updateSelectedProps);
propY.addEventListener('input', updateSelectedProps);
propLabel.addEventListener('input', updateSelectedProps);

btnDownloadJson.addEventListener('click', () => {
    const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(keys, null, 2));
    const dlAnchorElem = document.createElement('a');
    dlAnchorElem.setAttribute("href", dataStr);
    dlAnchorElem.setAttribute("download", "kbd_layout.json");
    dlAnchorElem.click();
});

// KLE Raw Format Parser
function parseKLE(data) {
    if (!Array.isArray(data)) return false;
    
    let parsedKeys = [];
    let idCnt = 0;
    
    // Default properties
    let curX = 0;
    let curY = 0;
    let currentProps = { w: 1, h: 1, x: 0, y: 0 };
    
    for (let r = 0; r < data.length; r++) {
        let row = data[r];
        if (!Array.isArray(row)) continue;
        
        for (let i = 0; i < row.length; i++) {
            let item = row[i];
            
            if (typeof item === 'string') {
                // Determine actual X and Y
                curX += currentProps.x;
                curY += currentProps.y;
                
                parsedKeys.push({
                    id: idCnt++,
                    x: curX,
                    y: curY,
                    w: currentProps.w,
                    h: currentProps.h,
                    label: item
                });
                
                curX += currentProps.w;
                // Reset ONLY next-key modifiers
                currentProps.w = 1;
                currentProps.h = 1;
                currentProps.x = 0;
                currentProps.y = 0;
            } else if (typeof item === 'object') {
                if (item.w !== undefined) currentProps.w = item.w;
                if (item.h !== undefined) currentProps.h = item.h;
                if (item.x !== undefined) currentProps.x = item.x;
                if (item.y !== undefined) currentProps.y = item.y;
                // Ignoring alignment 'a', width2 'w2', etc. for baseline implementation
            }
        }
        // End of row
        curY++;
        curX = 0;
    }
    
    if (parsedKeys.length > 0) {
        keys = parsedKeys;
        keyIdCounter = idCnt;
        selectedKeyId = null;
        renderCanvas();
        return true;
    }
    return false;
}

uploadJson.addEventListener('change', (e) => {
    const file = e.target.files[0];
    if(!file) return;
    const reader = new FileReader();
    reader.onload = (evt) => {
        try {
            const parsed = JSON.parse(evt.target.result);
            if(Array.isArray(parsed) && Array.isArray(parsed[0])){
                // KLE Format
                if (parseKLE(parsed)) {
                    alert("KLE Raw Data loaded successfully.");
                } else {
                    alert("Invalid KLE Data");
                }
            } else if(Array.isArray(parsed)){
                // Our internal format
                keys = parsed;
                keyIdCounter = Math.max(...keys.map(k=>k.id || 0)) + 1;
                selectedKeyId = null;
                renderCanvas();
                alert("Layout loaded successfully.");
            }
        } catch(err) {
            alert("Error parsing JSON");
        }
        uploadJson.value = '';
    };
    reader.readAsText(file);
});

// Keyboard Shortcuts
document.addEventListener('keydown', (e) => {
    // Determine if we are on Layout View
    const layoutView = document.getElementById('view-layout');
    if (!layoutView || layoutView.classList.contains('hidden')) return;

    // Help Modal
    if (e.key === 'F1' || (e.key === '?' && e.shiftKey)) {
        e.preventDefault();
        layoutHelpModal.classList.remove('hidden');
        return;
    }

    if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;

    if (e.key === 'Escape') {
        selectedKeyId = null;
        renderCanvas();
        return;
    }
    
    if (e.key === 'Insert' || e.key === 'i' || e.key === 'I') {
        btnAddKey.click();
        return;
    }

    if (e.key === 'Delete') {
        if (selectedKeyId !== null) {
            btnDeleteKey.click();
        }
        return;
    }

    if (e.key === 'j' || e.key === 'J') {
        if (keys.length === 0) return;
        let idx = keys.findIndex(k => k.id === selectedKeyId);
        idx = idx <= 0 ? keys.length - 1 : idx - 1;
        selectedKeyId = keys[idx].id;
        renderCanvas();
        return;
    }

    if (e.key === 'k' || e.key === 'K') {
        if (keys.length === 0) return;
        let idx = keys.findIndex(k => k.id === selectedKeyId);
        idx = (idx === -1 || idx >= keys.length - 1) ? 0 : idx + 1;
        selectedKeyId = keys[idx].id;
        renderCanvas();
        return;
    }

    if (selectedKeyId !== null) {
        const k = keys.find(x => x.id === selectedKeyId);
        if (!k) return;

        let moved = false;
        const step = 0.25;

        if (e.shiftKey) {
            // Resize
            if (e.key === 'ArrowRight') { k.w += step; moved = true; }
            if (e.key === 'ArrowLeft') { k.w = Math.max(0.25, k.w - step); moved = true; }
            if (e.key === 'ArrowUp') { k.h = Math.max(0.25, k.h - step); moved = true; }
            if (e.key === 'ArrowDown') { k.h += step; moved = true; }
        } else {
            // Move
            if (e.key === 'ArrowRight') { k.x += step; moved = true; }
            if (e.key === 'ArrowLeft') { k.x = Math.max(0, k.x - step); moved = true; }
            if (e.key === 'ArrowUp') { k.y = Math.max(0, k.y - step); moved = true; }
            if (e.key === 'ArrowDown') { k.y += step; moved = true; }
        }

        if (moved) {
            e.preventDefault();
            renderCanvas();
        }
    }
});

layoutHelpClose.addEventListener('click', () => {
    layoutHelpModal.classList.add('hidden');
});

layoutHelpModal.addEventListener('click', (e) => {
    if (e.target === layoutHelpModal) layoutHelpModal.classList.add('hidden');
});

// Initial Render
renderCanvas();
