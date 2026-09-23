const protocol = new UIAPduinoProtocol();

// QMKの標準的な基本的なキーコードリスト (一部抜粋)
// 値は QMK / VIA と同じ 16bit キーコードです。
const QMK_KEYCODES = {
  0x00: '× (NO)',
  0x01: '▽ (TRNS)',
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

// QMK のレイヤー操作キーコード (TO=0x5200+n, MO=0x5220+n, DF=0x5240+n, TG=0x5260+n)
const QK_TO = 0x5200;
const QK_MOMENTARY = 0x5220;
const QK_DEF_LAYER = 0x5240;
const QK_TOGGLE_LAYER = 0x5260;

const LAYER_KEYCODES = {
  0x5221: 'MO(1)', 0x5222: 'MO(2)', 0x5223: 'MO(3)',
  0x5261: 'TG(1)', 0x5262: 'TG(2)', 0x5263: 'TG(3)',
  0x5200: 'TO(0)', 0x5201: 'TO(1)', 0x5202: 'TO(2)', 0x5203: 'TO(3)',
  0x5240: 'DF(0)', 0x5241: 'DF(1)', 0x5242: 'DF(2)', 0x5243: 'DF(3)'
};

// QK_MODS のモディファイアビット (上位バイト側)
const QK_MOD_CTRL = 0x01;
const QK_MOD_SHIFT = 0x02;
const QK_MOD_ALT = 0x04;
const QK_MOD_GUI = 0x08;

const DEFAULT_LAYER_COUNT = 4;

// 内部状態
const state = {
    layer: 0,
    rows: 4,
    cols: 6,
    layerCount: DEFAULT_LAYER_COUNT,
    keymap: Array(DEFAULT_LAYER_COUNT).fill(0).map(() => Array(16).fill(0).map(() => Array(16).fill(0x00))),
    selectedKeyId: null, // Layout上の選択中キー（インデックス）
    layoutDef: [], // JSONから読み込んだキーの座標や行列情報
    unitSize: 54
};

// キーマップ格納用の配列を現在のレイヤー数・マトリクスサイズに合わせて確保し直す
function allocateKeymapState() {
    state.keymap = Array(state.layerCount).fill(0).map(
        () => Array(Math.max(state.rows, 16)).fill(0).map(
            () => Array(Math.max(state.cols, 16)).fill(0x0000)));
}

// 16bit キーコードを表示用ラベルへ変換する
function describeKeycode(code) {
    if (code === 0x0000) return '×';
    if (code === 0x0001) return '▽';

    if (LAYER_KEYCODES[code]) return LAYER_KEYCODES[code];

    // レイヤー操作 (テーブルに無いレイヤー番号)
    if (code >= QK_TO && code <= QK_TO + 0x1F) return `TO(${code & 0x1F})`;
    if (code >= QK_MOMENTARY && code <= QK_MOMENTARY + 0x1F) return `MO(${code & 0x1F})`;
    if (code >= QK_DEF_LAYER && code <= QK_DEF_LAYER + 0x1F) return `DF(${code & 0x1F})`;
    if (code >= QK_TOGGLE_LAYER && code <= QK_TOGGLE_LAYER + 0x1F) return `TG(${code & 0x1F})`;

    // Layer-Tap / Mod-Tap
    if (code >= 0x4000 && code <= 0x4FFF) {
        return `LT${(code >> 8) & 0x0F}(${describeKeycode(code & 0xFF)})`;
    }
    if (code >= 0x2000 && code <= 0x3FFF) {
        return `MT(${describeKeycode(code & 0xFF)})`;
    }

    // 基本キー / モディファイア付き基本キー
    const base = code & 0xFF;
    const mods = (code >> 8) & 0x1F;
    const label = QMK_KEYCODES[base] || MODIFIER_KEYCODES[base] || `0x${base.toString(16)}`;

    if (mods === 0) return label;

    const isRight = (mods & 0x10) !== 0;
    let modStr = '';
    if (mods & QK_MOD_CTRL) modStr += isRight ? 'RC-' : 'C-';
    if (mods & QK_MOD_SHIFT) modStr += isRight ? 'RS-' : 'S-';
    if (mods & QK_MOD_ALT) modStr += isRight ? 'RA-' : 'A-';
    if (mods & QK_MOD_GUI) modStr += isRight ? 'RW-' : 'W-';
    return modStr + label;
}

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

                // VIA/Remap 形式ではレジェンドの1行目が "row,col" になっています。
                // 本プロジェクト独自形式の {row, col} プロパティにも対応します。
                let row_ = cProps.row || 0;
                let col_ = cProps.col || 0;
                const posMatch = /^(\d+)\s*,\s*(\d+)/.exec(item.split('\n')[0]);
                if (posMatch) {
                    row_ = parseInt(posMatch[1], 10);
                    col_ = parseInt(posMatch[2], 10);
                }

                parsedKeys.push({
                    x: curX, y: curY, w: cProps.w, h: cProps.h,
                    r: cProps.r, rx: cProps.rx, ry: cProps.ry,
                    w2: cProps.w2, h2: cProps.h2, x2: cProps.x2, y2: cProps.y2,
                    row: row_, col: col_,
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

    // 読み込んだマトリクスサイズに合わせてキーマップ配列を確保し直す
    allocateKeymapState();

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
        const rawCode = (state.keymap[state.layer] &&
                         state.keymap[state.layer][k.row] &&
                         state.keymap[state.layer][k.row][k.col]) || 0x0000;
        const label = describeKeycode(rawCode);

        div.innerHTML = `<span style="font-size:0.9rem;">${label}</span><br><span class="keycode-label">[${k.row},${k.col}]</span>`;
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
    if (elModCtrl.checked) mods |= QK_MOD_CTRL;
    if (elModShift.checked) mods |= QK_MOD_SHIFT;
    if (elModAlt.checked) mods |= QK_MOD_ALT;
    if (elModWin.checked) mods |= QK_MOD_GUI;

    // QMK/VIA のエンコーディング: 基本キーは 0x00-0xFF、
    // モディファイア付きは mods(5bit) << 8 | 基本キー(8bit)。
    // レイヤー操作キー等 (0x0100 以上) にはモディファイアを付けない。
    const finalCode = (baseCode <= 0xFF && mods !== 0)
        ? ((mods << 8) | baseCode)
        : baseCode;

    // RAMマップ更新
    state.keymap[state.layer][k.row][k.col] = finalCode;
    renderLayoutKeys();

    // デバイスが接続されていればすぐ送信
    if (protocol.isConnected) {
        try {
            await protocol.setKeycode(state.layer, k.row, k.col, finalCode);
        } catch (e) {
            console.error("Failed to write key to device:", e);
        }
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
    const rawCode = (state.keymap[state.layer] &&
                     state.keymap[state.layer][k.row] &&
                     state.keymap[state.layer][k.row][k.col]) || 0x0000;
    // レイヤー操作キー等はキーコードそのもの、基本キーは下位8bitで選択状態を判定する
    const baseCode = rawCode <= 0x1FFF ? (rawCode & 0xFF) : rawCode;

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

    try {
        const version = await protocol.getProtocolVersion();
        state.layerCount = await protocol.getLayerCount() || DEFAULT_LAYER_COUNT;
        allocateKeymapState();

        // キーマップ全体を一括で読み出す (VIA の dynamic_keymap_get_buffer)
        const keymap = await protocol.readAllKeymaps(state.layerCount, state.rows, state.cols);
        for (let l = 0; l < keymap.length; l++) {
            for (let r = 0; r < keymap[l].length; r++) {
                for (let c = 0; c < keymap[l][r].length; c++) {
                    state.keymap[l][r][c] = keymap[l][r][c];
                }
            }
        }

        if (state.layer >= state.layerCount) state.layer = 0;
        renderLayoutKeys();
        elStatusText.textContent =
            `デバイス接続済 (VIA v${version >> 8}.${version & 0xFF} / ${state.layerCount}レイヤー)`;
    } catch (e) {
        console.error(e);
        elStatusText.textContent = `読み込みに失敗しました: ${e.message}`;
    }
}

protocol.onDisconnect = () => {
    updateConnectionStatus();
    elStatusText.textContent = '切断されました';
};

elBtnConnect.addEventListener('click', connectAndSync);
elBtnConnectOverlay.addEventListener('click', connectAndSync);

elBtnSaveToFlash.addEventListener('click', async () => {
    if (!protocol.isConnected) return;
    const originalLabel = elBtnSaveToFlash.textContent;
    elBtnSaveToFlash.disabled = true;
    elBtnSaveToFlash.textContent = 'Flashへ保存中...';
    try {
        await protocol.saveToFlash();
        elBtnSaveToFlash.textContent = '保存しました';
        setTimeout(() => { elBtnSaveToFlash.textContent = originalLabel; }, 1500);
    } catch (e) {
        console.error(e);
        alert("保存に失敗しました");
        elBtnSaveToFlash.textContent = originalLabel;
    } finally {
        elBtnSaveToFlash.disabled = false;
    }
});

elBtnResetKeymap.addEventListener('click', async () => {
    if (!protocol.isConnected) return;
    if (!confirm('デバイスのキーマップをファームウェアの初期値へ戻しますか？\n(現在の設定は失われます)')) return;
    try {
        await protocol.resetKeymap();
        await syncKeymapFromDevice();
    } catch (e) {
        console.error(e);
        alert("初期化に失敗しました");
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
