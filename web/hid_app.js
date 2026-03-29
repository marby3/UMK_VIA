const protocol = new UIAPduinoProtocol();

// QMKの標準的な基本的なキーコードリスト (一部抜粋)
const QMK_KEYCODES = {
  0x00: '未割当',
  0x04: 'A', 0x05: 'B', 0x06: 'C', 0x07: 'D', 0x08: 'E', 0x09: 'F', 0x0A: 'G',
  0x0B: 'H', 0x0C: 'I', 0x0D: 'J', 0x0E: 'K', 0x0F: 'L', 0x10: 'M', 0x11: 'N',
  0x12: 'O', 0x13: 'P', 0x14: 'Q', 0x15: 'R', 0x16: 'S', 0x17: 'T', 0x18: 'U',
  0x19: 'V', 0x1A: 'W', 0x1B: 'X', 0x1C: 'Y', 0x1D: 'Z',
  0x1E: '1', 0x1F: '2', 0x20: '3', 0x21: '4', 0x22: '5', 0x23: '6', 0x24: '7',
  0x25: '8', 0x26: '9', 0x27: '0', 0x28: 'Enter', 0x29: 'ESC', 0x2A: 'Backspace',
  0x2B: 'Tab', 0x2C: 'Space', 0x2D: '-', 0x2E: '=', 0x2F: '[', 0x30: ']',
  0x31: '\\', 0x33: ';', 0x34: '\'', 0x35: '`', 0x36: ',', 0x37: '.', 0x38: '/',
  0x39: 'Caps Lock', 0x3A: 'F1', 0x3B: 'F2', 0x3C: 'F3', 0x3D: 'F4', 0x3E: 'F5',
  0x3F: 'F6', 0x40: 'F7', 0x41: 'F8', 0x42: 'F9', 0x43: 'F10', 0x44: 'F11', 0x45: 'F12',
  0x4F: 'Right', 0x50: 'Left', 0x51: 'Down', 0x52: 'Up',
  0xE0: 'L Ctrl', 0xE1: 'L Shift', 0xE2: 'L Alt', 0xE3: 'L GUI',
  0xE4: 'R Ctrl', 0xE5: 'R Shift', 0xE6: 'R Alt', 0xE7: 'R GUI'
};

// 内部状態 (デフォルト値で初期化)
const state = {
    layer: 0,
    rows: 4,
    cols: 6,
    // [layer][row][col] の3次元配列 (最初はすべて0)
    keymap: Array(4).fill(0).map(() => Array(4).fill(0).map(() => Array(6).fill(0x00))),
    selectedKey: { r: -1, c: -1 }
};

// 一部初期データを見栄え用に入れておく (ファームウェアのPhase 2の初期値に合わせる)
state.keymap[0][0] = [ 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 ]; // A-F
state.keymap[0][1] = [ 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F ]; // G-L

// DOM Elements
const elBtnConnect = document.getElementById('btnConnect');
const elBtnSaveToFlash = document.getElementById('btnSaveToFlash');
const elStatusIndicator = document.getElementById('statusIndicator');
const elStatusText = document.getElementById('statusText');
const elKeyboardGrid = document.getElementById('keyboardGrid');
const elLayerBtns = document.querySelectorAll('.layer-btn');

const elModal = document.getElementById('keyBindModal');
const elModalClose = document.getElementById('modalClose');
const elKeycodeSelect = document.getElementById('keycodeSelect');
const elBtnApplyKey = document.getElementById('btnApplyKey');

// --- Initialization ---

function initUI() {
    // セレクトボックスの中身を作成
    for (const [code, label] of Object.entries(QMK_KEYCODES)) {
        const option = document.createElement('option');
        option.value = code;
        option.textContent = `${label} (0x${parseInt(code).toString(16).padStart(2, '0').toUpperCase()})`;
        elKeycodeSelect.appendChild(option);
    }

    renderKeyboard();
}

function renderKeyboard() {
    elKeyboardGrid.innerHTML = '';
    const currentLayerMap = state.keymap[state.layer];

    for (let r = 0; r < state.rows; r++) {
        for (let c = 0; c < state.cols; c++) {
            const code = currentLayerMap[r][c];
            const label = QMK_KEYCODES[code] || `0x${code.toString(16)}`;
            
            const btn = document.createElement('button');
            btn.className = 'key';
            if (state.selectedKey.r === r && state.selectedKey.c === c) {
                btn.classList.add('active');
            }
            
            const mainLabel = document.createElement('span');
            mainLabel.textContent = label;
            
            const subLabel = document.createElement('span');
            subLabel.className = 'keycode-label';
            subLabel.textContent = `[${r},${c}]`;

            btn.appendChild(mainLabel);
            btn.appendChild(subLabel);

            btn.addEventListener('click', () => openModal(r, c));
            elKeyboardGrid.appendChild(btn);
        }
    }
}

function updateConnectionStatus() {
    if (protocol.isConnected) {
        elStatusIndicator.className = 'indicator connected';
        elStatusText.textContent = 'デバイス接続済 (UIAPduino)';
        elBtnConnect.disabled = true;
        elBtnSaveToFlash.disabled = false;
        elBtnConnect.textContent = '接続済み';
    } else {
        elStatusIndicator.className = 'indicator disconnected';
        elStatusText.textContent = 'デバイス未接続';
        elBtnConnect.disabled = false;
        elBtnSaveToFlash.disabled = true;
        elBtnConnect.textContent = 'デバイスに接続';
    }
}

// --- Interactions ---

protocol.onInputReport = (data) => {
    // 0x02 コマンドの返答が来た場合、ローカルマップを更新する
    if (data[0] === 0x02) {
        const layer = data[1];
        const row = data[2];
        const col = data[3];
        const keycode = (data[4] << 8) | data[5];
        
        if (layer < state.layer.length || layer < 4) {
            state.keymap[layer][row][col] = keycode;
            // 現在表示中のレイヤーなら即座に再描画
            if (state.layer === layer) renderKeyboard();
        }
    }
};

async function syncKeymapFromDevice() {
    elStatusText.textContent = 'デバイスから設定を読み込み中...';
    // 96キーすべてを連続でリクエストする (USBの過負荷を避けるため少しずつ)
    for (let l = 0; l < 4; l++) {
        for (let r = 0; r < state.rows; r++) {
            for (let c = 0; c < state.cols; c++) {
                await protocol.readKeyRequest(l, r, c);
                // 約10ms待機 (全体で約1秒で同期完了)
                await new Promise(resolve => setTimeout(resolve, 10));
            }
        }
    }
    elStatusText.textContent = 'デバイス接続済 (UIAPduino)';
}

elBtnConnect.addEventListener('click', async () => {
    const success = await protocol.connect();
    if (success) {
        updateConnectionStatus();
        await syncKeymapFromDevice();
    }
});

protocol.onDisconnect = () => {
    console.warn("Device disconnected!");
    updateConnectionStatus();
    elStatusText.textContent = '切断されました (保存後・または抜去)';
};

elBtnSaveToFlash.addEventListener('click', async () => {
    if (!protocol.isConnected) return;
    try {
        await protocol.saveToFlash();
        elBtnSaveToFlash.disabled = true;
        elBtnSaveToFlash.textContent = 'Flashへ保存中...';
        // マイコンが再起動するため、直後に disconnect イベントが発火します。
    } catch (e) {
        alert("保存に失敗しました");
    }
});

elLayerBtns.forEach(btn => {
    btn.addEventListener('click', (e) => {
        elLayerBtns.forEach(b => b.classList.remove('active'));
        e.target.classList.add('active');
        state.layer = parseInt(e.target.dataset.layer);
        renderKeyboard();
    });
});

function openModal(r, c) {
    state.selectedKey = { r, c };
    const currentCode = state.keymap[state.layer][r][c];
    elKeycodeSelect.value = currentCode;
    
    renderKeyboard(); // active表示更新
    elModal.classList.remove('hidden');
}

function closeModal() {
    state.selectedKey = { r: -1, c: -1 };
    renderKeyboard();
    elModal.classList.add('hidden');
}

elModalClose.addEventListener('click', closeModal);
elModal.addEventListener('click', (e) => {
    if (e.target === elModal) closeModal();
});

elBtnApplyKey.addEventListener('click', async () => {
    const r = state.selectedKey.r;
    const c = state.selectedKey.c;
    if (r === -1 || c === -1) return;

    const newCode = parseInt(elKeycodeSelect.value);

    // RAMマップをローカルでも更新
    state.keymap[state.layer][r][c] = newCode;
    
    // USBデバイスへ送信
    if (protocol.isConnected) {
        const ok = await protocol.writeKey(state.layer, r, c, newCode);
        if (!ok) console.error("Failed to write key to device");
    }

    closeModal();
});

// Start
initUI();
