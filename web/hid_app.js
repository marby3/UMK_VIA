const protocol = new UIAPduinoProtocol();

// QMKの標準的な基本的なキーコードリスト (一部抜粋)
const QMK_KEYCODES = {
  0x00: '▽ (TRNS)',
  0x04: 'A', 0x05: 'B', 0x06: 'C', 0x07: 'D', 0x08: 'E', 0x09: 'F', 0x0A: 'G',
  0x0B: 'H', 0x0C: 'I', 0x0D: 'J', 0x0E: 'K', 0x0F: 'L', 0x10: 'M', 0x11: 'N',
  0x12: 'O', 0x13: 'P', 0x14: 'Q', 0x15: 'R', 0x16: 'S', 0x17: 'T', 0x18: 'U',
  0x19: 'V', 0x1A: 'W', 0x1B: 'X', 0x1C: 'Y', 0x1D: 'Z',
  0x1E: '1', 0x1F: '2', 0x20: '3', 0x21: '4', 0x22: '5', 0x23: '6', 0x24: '7',
  0x25: '8', 0x26: '9', 0x27: '0', 0x28: 'Enter', 0x29: 'ESC', 0x2A: 'Bksp',
  0x2B: 'Tab', 0x2C: 'Space', 0x2D: '-', 0x2E: '=', 0x2F: '[', 0x30: ']',
  0x31: '\\', 0x33: ';', 0x34: '\'', 0x35: '`', 0x36: ',', 0x37: '.', 0x38: '/',
  0x39: 'Caps', 0x3A: 'F1', 0x3B: 'F2', 0x3C: 'F3', 0x3D: 'F4', 0x3E: 'F5',
  0x3F: 'F6', 0x40: 'F7', 0x41: 'F8', 0x42: 'F9', 0x43: 'F10', 0x44: 'F11', 0x45: 'F12',
  0x4F: 'Right', 0x50: 'Left', 0x51: 'Down', 0x52: 'Up'
};

const MODIFIER_KEYCODES = {
  0xE0: 'LCtrl', 0xE1: 'LShift', 0xE2: 'LAlt', 0xE3: 'LGUI',
  0xE4: 'RCtrl', 0xE5: 'RShift', 0xE6: 'RAlt', 0xE7: 'RGUI'
};

const LAYER_KEYCODES = {
  0x0101: 'MO(1)', 0x0102: 'MO(2)', 0x0103: 'MO(3)',
  0x0201: 'TG(1)', 0x0202: 'TG(2)', 0x0203: 'TG(3)',
  0x0300: 'TO(0)', 0x0301: 'TO(1)', 0x0302: 'TO(2)', 0x0303: 'TO(3)'
};

// 内部状態
const state = {
    layer: 0,
    rows: 4,
    cols: 6,
    keymap: Array(4).fill(0).map(() => Array(16).fill(0).map(() => Array(16).fill(0x00))),
    selectedKeyId: null, // Layout上の選択中キー（インデックス）
    layoutDef: [], // JSONから読み込んだキーの座標や行列情報
    unitSize: 54
};

// DOM
const elOverlay = document.getElementById('keymapOverlay');
const elBtnConnectOverlay = document.getElementById('btnConnectOverlay');
const elUploadConfigJson = document.getElementById('uploadConfigJson');
const elUploadConfigJsonOverlay = document.getElementById('uploadConfigJsonOverlay');
const elBtnConnect = document.getElementById('btnConnect');

const elStatusIndicator = document.getElementById('statusIndicator');
const elStatusText = document.getElementById('statusText');

const hidCanvas = document.getElementById('keymapCanvas');
const hidCanvasContainer = document.getElementById('keymapCanvasContainer');
const elLayerBtns = document.querySelectorAll('.layer-btn');
const elBtnSaveToFlash = document.getElementById('btnSaveToFlash');
const elBtnResetKeymap = document.getElementById('btnResetKeymap');

const elPaletteBasicGrid = document.getElementById('paletteBasicGrid');
const elPaletteLayersGrid = document.getElementById('paletteLayersGrid');
const elPaletteTabs = document.querySelectorAll('#palettePanel .builder-tab');
const elPalettePanes = document.querySelectorAll('#palettePanel .tab-pane');

const elModCtrl = document.getElementById('modCtrl');
const elModShift = document.getElementById('modShift');
const elModAlt = document.getElementById('modAlt');
const elModWin = document.getElementById('modWin');

const elKeyBindModal = document.getElementById('keyBindModal');
const elKeycodeSelect = document.getElementById('keycodeSelect');
const elBtnApplyKey = document.getElementById('btnApplyKey');
const elModalClose = document.getElementById('modalClose');


// --- UI Initialization ---

function initUI() {
    buildPalette();
    renderLayoutKeys();
}

function buildPalette() {
    // Basic Keycodes
    for (const [code, label] of Object.entries(QMK_KEYCODES)) {
        const btn = document.createElement('div');
        btn.className = 'palette-key';
        btn.textContent = label;
        btn.onclick = () => assignKeycode(parseInt(code));
        elPaletteBasicGrid.appendChild(btn);
    }
    for (const [code, label] of Object.entries(MODIFIER_KEYCODES)) {
        const btn = document.createElement('div');
        btn.className = 'palette-key';
        btn.textContent = label;
        btn.onclick = () => assignKeycode(parseInt(code));
        elPaletteBasicGrid.appendChild(btn);
    }

    // Layer Keycodes
    for (const [code, label] of Object.entries(LAYER_KEYCODES)) {
        const btn = document.createElement('div');
        btn.className = 'palette-key';
        btn.textContent = label;
        btn.onclick = () => assignKeycode(parseInt(code));
        elPaletteLayersGrid.appendChild(btn);
    }
}

// パレットタブの切り替え
elPaletteTabs.forEach(tab => {
    tab.addEventListener('click', (e) => {
        elPaletteTabs.forEach(t => t.classList.remove('active'));
        elPalettePanes.forEach(p => p.classList.add('hidden'));
        e.target.classList.add('active');
        document.getElementById('cat-' + e.target.dataset.cat).classList.remove('hidden');
    });
});

// JSONからレイアウトを読み込む処理共通化
function handleUploadJson(e) {
    const file = e.target.files[0];
    if(!file) return;
    const reader = new FileReader();
    reader.onload = (evt) => {
        try {
            const def = JSON.parse(evt.target.result);
            if (parseKeyboardDefinitionFile(def)) {
                // オーバーレイを解除（仮想モードとして操作可能にする）
                elOverlay.classList.add('hidden');
                alert("レイアウト定義を読み込みました。デバイスに未接続の場合、変更は保存されません。");
            } else {
                alert("不正なJSONファイルです。");
            }
        } catch(err) {
            alert("JSONのパースエラー");
        }
        e.target.value = ''; // 🌟同じファイルを再度アップロードできるように値をリセット
    };
    reader.readAsText(file);
}

elUploadConfigJson.addEventListener('change', handleUploadJson);
if (elUploadConfigJsonOverlay) {
    elUploadConfigJsonOverlay.addEventListener('change', handleUploadJson);
}

function parseKeyboardDefinitionFile(def) {
    if(def.matrix) {
        state.rows = def.matrix.rows || 4;
        state.cols = def.matrix.cols || 6;
    }
    const keymap = def.layouts && def.layouts.keymap ? def.layouts.keymap : def;
    if (!Array.isArray(keymap)) return false;
    
    let parsedKeys = [];
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
    state.layoutDef = parsedKeys;
    renderLayoutKeys();
    return true;
}

// レイアウトをキャンバスに描画
function renderLayoutKeys() {
    hidCanvas.innerHTML = '';
    
    // レイアウト定義がない場合はデフォルト描画を作る
    const layout = state.layoutDef.length > 0 ? state.layoutDef : generateDefaultLayout();

    layout.forEach((k, idx) => {
        const div = document.createElement('div');
        div.className = 'layout-key key';
        if (state.selectedKeyId === idx) div.classList.add('selected');
        
        // キーの結合を防ぐために上下左右2pxの隙間（計4px）を入れる
        div.style.left = (k.x * state.unitSize + 2) + 'px';
        div.style.top = (k.y * state.unitSize + 2) + 'px';
        div.style.width = (k.w * state.unitSize - 4) + 'px';
        div.style.height = (k.h * state.unitSize - 4) + 'px';
        div.style.position = 'absolute';
        
        if (k.r) {
            div.style.transform = `rotate(${k.r}deg)`;
            let originX = ((k.rx || 0) - k.x) * state.unitSize;
            let originY = ((k.ry || 0) - k.y) * state.unitSize;
            div.style.transformOrigin = `${originX}px ${originY}px`;
        }

        // --- 特殊形状 (ISO Enter等) 対応 ---
        if (k.w2 > 0 || k.h2 > 0) {
            const shp = document.createElement('div');
            shp.className = 'key-shape-extension';
            shp.style.left = ((k.x2 || 0) * state.unitSize) + 'px';
            shp.style.top = ((k.y2 || 0) * state.unitSize) + 'px';
            shp.style.width = ((k.w2 || k.w) * state.unitSize) + 'px';
            shp.style.height = ((k.h2 || k.h) * state.unitSize) + 'px';
            div.appendChild(shp);
        }

        // キーコードから表示ラベルを取得
        const rawCode = state.keymap[state.layer][k.row][k.col];
        const baseCode = rawCode & 0x0FFF; // QMK互換フラグ想定
        const mods = (rawCode >> 12) & 0xF;
        
        let label = QMK_KEYCODES[baseCode] || MODIFIER_KEYCODES[baseCode] || LAYER_KEYCODES[baseCode] || `0x${baseCode.toString(16)}`;
        if (baseCode === 0) label = "▽";
        
        let modStr = "";
        if(mods & 0x1) modStr += "C-";
        if(mods & 0x2) modStr += "S-";
        if(mods & 0x4) modStr += "A-";
        if(mods & 0x8) modStr += "W-";

        div.innerHTML = `<span style="font-size:0.9rem;">${modStr}${label}</span><br><span class="keycode-label">[${k.row},${k.col}]</span>`;
        div.addEventListener('click', () => {
            state.selectedKeyId = idx;
            renderLayoutKeys(); // activeクラス更新
        });
        div.addEventListener('dblclick', () => {
            state.selectedKeyId = idx;
            renderLayoutKeys();
            openKeycodeModal(idx);
        });
        
        hidCanvas.appendChild(div);
    });
}

function generateDefaultLayout() {
    // 4x6の基本レイアウトを生成
    let def = [];
    for(let r=0; r<4; r++){
        for(let c=0; c<6; c++){
            def.push({ x: c*1.1, y: r*1.1, w: 1, h: 1, row: r, col: c });
        }
    }
    return def;
}

// パレットからキーコードが選ばれたときの処理
async function assignKeycode(baseCode, targetIdx = state.selectedKeyId) {
    if (targetIdx === null) return;
    
    const k = state.layoutDef.length > 0 ? state.layoutDef[targetIdx] : generateDefaultLayout()[targetIdx];
    if (!k) return;

    let mods = 0;
    if (elModCtrl.checked) mods |= 0x1;
    if (elModShift.checked) mods |= 0x2;
    if (elModAlt.checked) mods |= 0x4;
    if (elModWin.checked) mods |= 0x8;
    
    // VIA等のQMK互換エンコーディング: mods(4bit) << 12 | baseCode(12bit)
    const finalCode = (mods << 12) | (baseCode & 0x0FFF);
    
    // RAMマップ更新
    state.keymap[state.layer][k.row][k.col] = finalCode;
    renderLayoutKeys();

    // デバイスが接続されていればすぐ送信
    if (protocol.isConnected) {
        const ok = await protocol.writeKey(state.layer, k.row, k.col, finalCode);
        if (!ok) console.error("Failed to write key to device");
    }
    
    // 自動的に次のキーへフォーカスを移す
    if (targetIdx === state.selectedKeyId && state.selectedKeyId < (state.layoutDef.length || 24) - 1) {
        state.selectedKeyId++;
        renderLayoutKeys();
    }
}

// モーダル操作
let modalTargetIdx = null;

function openKeycodeModal(idx) {
    modalTargetIdx = idx;
    elKeycodeSelect.innerHTML = '';
    
    const k = state.layoutDef.length > 0 ? state.layoutDef[idx] : generateDefaultLayout()[idx];
    const rawCode = state.keymap[state.layer][k.row][k.col];
    const baseCode = rawCode & 0x0FFF;

    const addGroup = (groupName, obj) => {
        const optgroup = document.createElement('optgroup');
        optgroup.label = groupName;
        for (const [code, label] of Object.entries(obj)) {
            const opt = document.createElement('option');
            opt.value = code;
            opt.textContent = label;
            if (parseInt(code) === baseCode) opt.selected = true;
            optgroup.appendChild(opt);
        }
        elKeycodeSelect.appendChild(optgroup);
    };

    addGroup("Basic (A-Z, Num)", QMK_KEYCODES);
    addGroup("Modifiers", MODIFIER_KEYCODES);
    addGroup("Layers & System", LAYER_KEYCODES);

    elKeyBindModal.classList.remove('hidden');
}

function closeKeycodeModal() {
    elKeyBindModal.classList.add('hidden');
    modalTargetIdx = null;
}

elModalClose.addEventListener('click', closeKeycodeModal);
elKeyBindModal.addEventListener('click', (e) => {
    if (e.target === elKeyBindModal) closeKeycodeModal();
});

elBtnApplyKey.addEventListener('click', async () => {
    if (modalTargetIdx === null) return;
    const baseCode = parseInt(elKeycodeSelect.value);
    await assignKeycode(baseCode, modalTargetIdx);
    closeKeycodeModal();
});


// --- Hardware Connection ---

function updateConnectionStatus() {
    if (protocol.isConnected) {
        elOverlay.classList.add('hidden'); // 接続したら待機UIを消す
        elStatusIndicator.className = 'indicator connected';
        elStatusText.textContent = 'デバイス接続済 (UIAPduino)';
        elBtnConnect.disabled = true;
        elBtnSaveToFlash.disabled = false;
        elBtnConnect.textContent = '接続済み';
        elBtnConnectOverlay.style.display = 'none';
        elBtnSaveToFlash.textContent = '設定をFlashに保存';
    } else {
        elStatusIndicator.className = 'indicator disconnected';
        elStatusText.textContent = 'デバイス未接続';
        elBtnConnect.disabled = false;
        elBtnSaveToFlash.disabled = true;
        elBtnConnect.textContent = 'デバイスに接続';
        elBtnConnectOverlay.style.display = 'inline-block';
        elBtnSaveToFlash.textContent = '設定をFlashに保存';
        
        if (state.layoutDef.length === 0) {
            elOverlay.classList.remove('hidden'); // 未定義なら待機UIを出す
        }
    }
}

async function connectAndSync() {
    const success = await protocol.connect();
    if (success) {
        updateConnectionStatus();
        await syncKeymapFromDevice();
    }
}

async function syncKeymapFromDevice() {
    elStatusText.textContent = 'デバイスから設定を読み込み中...';
    // 行優先でデバイスの現在のキーマップを取得する
    for (let l = 0; l < 4; l++) {
        for (let r = 0; r < state.rows; r++) {
            for (let c = 0; c < state.cols; c++) {
                await protocol.readKeyRequest(l, r, c);
                // 約10ms待機 (USB HIDのフラッディング防止)
                await new Promise(resolve => setTimeout(resolve, 10));
            }
        }
    }
    elStatusText.textContent = 'デバイス接続済 (同期完了)';
}

// デバイスから0x02コマンドによるキーコード読み出しの返答
protocol.onInputReport = (data) => {
    if (data[0] === 0x02) {
        const layer = data[1];
        const row = data[2];
        const col = data[3];
        const keycode = (data[4] << 8) | data[5];
        
        if (layer < 4) {
            state.keymap[layer][row][col] = keycode;
            if (state.layer === layer) renderLayoutKeys();
        }
    }
};

protocol.onDisconnect = () => {
    updateConnectionStatus();
    elStatusText.textContent = '切断されました';
};

elBtnConnect.addEventListener('click', connectAndSync);
elBtnConnectOverlay.addEventListener('click', connectAndSync);

elBtnSaveToFlash.addEventListener('click', async () => {
    if (!protocol.isConnected) return;
    try {
        await protocol.saveToFlash();
        elBtnSaveToFlash.disabled = true;
        elBtnSaveToFlash.textContent = 'Flashへ保存中...';
    } catch (e) {
        alert("保存に失敗しました");
    }
});

elBtnResetKeymap.addEventListener('click', () => {
    if(confirm('キーマップを現在デバイスに保存されている状態にリセットしますか？')) {
        if(protocol.isConnected) syncKeymapFromDevice();
    }
});

// レイヤータブ
elLayerBtns.forEach(btn => {
    btn.addEventListener('click', (e) => {
        elLayerBtns.forEach(b => b.classList.remove('active'));
        e.target.classList.add('active');
        state.layer = parseInt(e.target.dataset.layer);
        renderLayoutKeys();
    });
});


// ルーティング
const navItems = document.querySelectorAll('.nav-item');
const appViews = document.querySelectorAll('.app-view');
navItems.forEach(item => {
    item.addEventListener('click', () => {
        if (!item.dataset.target) return;
        navItems.forEach(nav => nav.classList.remove('active'));
        item.classList.add('active');
        appViews.forEach(view => {
            view.classList.add('hidden');
            view.classList.remove('active');
        });
        const targetView = document.getElementById('view-' + item.dataset.target);
        if (targetView) {
            targetView.classList.remove('hidden');
            targetView.classList.add('active');
        }
    });
});

initUI();
