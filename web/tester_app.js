// ----------------------------------------------------
// UIAPduino Tester App (Key Tester)
// ----------------------------------------------------

// --- Default Layout Definitions (ANSI & JIS) ---
// We use KeyboardEvent.code as labels to properly match the physical keys.

const LAYOUT_104_US = [
  ["Escape", {x:1},"F1", "F2", "F3", "F4", {x:0.5},"F5", "F6", "F7", "F8", {x:0.5},"F9", "F10", "F11", "F12", {x:0.25},"PrintScreen", "ScrollLock", "Pause"],
  ["Backquote", "Digit1", "Digit2", "Digit3", "Digit4", "Digit5", "Digit6", "Digit7", "Digit8", "Digit9", "Digit0", "Minus", "Equal", {w:2},"Backspace", {x:0.25},"Insert", "Home", "PageUp", {x:0.25},"NumLock", "NumpadDivide", "NumpadMultiply", "NumpadSubtract"],
  [{w:1.5},"Tab", "KeyQ", "KeyW", "KeyE", "KeyR", "KeyT", "KeyY", "KeyU", "KeyI", "KeyO", "KeyP", "BracketLeft", "BracketRight", {w:1.5},"Backslash", {x:0.25},"Delete", "End", "PageDown", {x:0.25},"Numpad7", "Numpad8", "Numpad9", {h:2},"NumpadAdd"],
  [{w:1.75},"CapsLock", "KeyA", "KeyS", "KeyD", "KeyF", "KeyG", "KeyH", "KeyJ", "KeyK", "KeyL", "Semicolon", "Quote", {w:2.25},"Enter", {x:3.5},"Numpad4", "Numpad5", "Numpad6"],
  [{w:2.25},"ShiftLeft", "KeyZ", "KeyX", "KeyC", "KeyV", "KeyB", "KeyN", "KeyM", "Comma", "Period", "Slash", {w:2.75},"ShiftRight", {x:1.25},"ArrowUp", {x:1.25},"Numpad1", "Numpad2", "Numpad3", {h:2},"NumpadEnter"],
  [{w:1.25},"ControlLeft", {w:1.25},"MetaLeft", {w:1.25},"AltLeft", {w:6.25},"Space", {w:1.25},"AltRight", {w:1.25},"MetaRight", {w:1.25},"ContextMenu", {w:1.25},"ControlRight", {x:0.25},"ArrowLeft", "ArrowDown", "ArrowRight", {x:0.25},{w:2},"Numpad0", "NumpadDecimal"]
];

const LAYOUT_109_JIS = [
  ["Escape", {x:1},"F1", "F2", "F3", "F4", {x:0.5},"F5", "F6", "F7", "F8", {x:0.5},"F9", "F10", "F11", "F12", {x:0.25},"PrintScreen", "ScrollLock", "Pause"],
  ["Backquote", "Digit1", "Digit2", "Digit3", "Digit4", "Digit5", "Digit6", "Digit7", "Digit8", "Digit9", "Digit0", "Minus", "Equal", "IntlYen", {w:1},"Backspace", {x:0.25},"Insert", "Home", "PageUp", {x:0.25},"NumLock", "NumpadDivide", "NumpadMultiply", "NumpadSubtract"],
  [{w:1.5},"Tab", "KeyQ", "KeyW", "KeyE", "KeyR", "KeyT", "KeyY", "KeyU", "KeyI", "KeyO", "KeyP", "BracketLeft", "BracketRight", {x:0.25, w:1.25, h:2, w2:1.5, h2:1, x2:-0.25},"Enter", {x:0.25},"Delete", "End", "PageDown", {x:0.25},"Numpad7", "Numpad8", "Numpad9", {h:2},"NumpadAdd"],
  [{w:1.75},"CapsLock", "KeyA", "KeyS", "KeyD", "KeyF", "KeyG", "KeyH", "KeyJ", "KeyK", "KeyL", "Semicolon", "Quote", "Backslash", {x:4.75},"Numpad4", "Numpad5", "Numpad6"],
  [{w:2.25},"ShiftLeft", "KeyZ", "KeyX", "KeyC", "KeyV", "KeyB", "KeyN", "KeyM", "Comma", "Period", "Slash", "IntlRo", {w:1.75},"ShiftRight", {x:1.25},"ArrowUp", {x:1.25},"Numpad1", "Numpad2", "Numpad3", {h:2},"NumpadEnter"],
  [{w:1.25},"ControlLeft", {w:1.25},"MetaLeft", {w:1.25},"AltLeft", {w:1.25},"NonConvert", {w:2.75},"Space", {w:1.25},"Convert", {w:1.25},"KanaMode", {w:1.25},"AltRight", {w:1.25},"ContextMenu", {w:1.25},"ControlRight", {x:0.5},"ArrowLeft", "ArrowDown", "ArrowRight", {x:0.25},{w:2},"Numpad0", "NumpadDecimal"]
];

function generateTesterKeysFromDef(layoutDef) {
    let parsedKeys = [];
    let idCnt = 0;
    let curX = 0, curY = 0;
    let cProps = { w: 1, h: 1, x: 0, y: 0, r: 0, rx: 0, ry: 0, w2: 0, h2: 0, x2: 0, y2: 0 };
    
    for (let r = 0; r < layoutDef.length; r++) {
        let row = layoutDef[r];
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
    return parsedKeys;
}

// --- Display Formatter ---
function formatKeyCode(code) {
    if (!code) return '';
    code = code.replace('Key', '');
    code = code.replace('Digit', '');
    
    // Convert shifted symbols natively where possible (Upper symbol on top)
    const symbolMap = {
        'Backquote': '~\n`', 'Minus': '_\n-', 'Equal': '+\n=',
        'BracketLeft': '{\n[', 'BracketRight': '}\n]', 'Backslash': '|\n\\',
        'Semicolon': ':\n;', 'Quote': '"\n\'', 'Comma': '<\n,',
        'Period': '>\n.', 'Slash': '?\n/'
    };
    if (symbolMap[code]) return symbolMap[code];

    if (code === 'ControlLeft') return 'LCtrl';
    if (code === 'ControlRight') return 'RCtrl';
    if (code === 'ShiftLeft') return 'LShift';
    if (code === 'ShiftRight') return 'RShift';
    if (code === 'AltLeft') return 'LAlt';
    if (code === 'AltRight') return 'RAlt';
    if (code === 'MetaLeft') return 'LWin';
    if (code === 'MetaRight') return 'RWin';
    if (code === 'Escape') return 'Esc';
    if (code === 'ArrowUp') return '↑';
    if (code === 'ArrowDown') return '↓';
    if (code === 'ArrowLeft') return '←';
    if (code === 'ArrowRight') return '→';
    if (code === 'NumpadEnter') return 'NumEnt';
    if (code === 'NumpadAdd') return '+';
    if (code === 'NumpadSubtract') return '-';
    if (code === 'NumpadMultiply') return '*';
    if (code === 'NumpadDivide') return '/';
    if (code === 'NumpadDecimal') return 'Num.';
    
    // Scale down long text via spans if needed, or simply let CSS handle it
    if (code.startsWith('Numpad')) return code.replace('Numpad', 'Num\n');
    if (code === 'PrintScreen') return 'PrtSc';
    if (code === 'ScrollLock') return 'ScrLk';
    if (code === 'CapsLock') return 'Caps';
    if (code === 'Insert') return 'Ins';
    if (code === 'Delete') return 'Del';
    if (code === 'PageUp') return 'PgUp';
    if (code === 'PageDown') return 'PgDn';
    
    return code;
}

// Map custom QMK / basic labels to event.code roughly
function mapLabelToEventCode(label) {
    if (!label) return '';
    const orig = label;
    label = label.toUpperCase();

    if (label.startsWith('KC_')) label = label.substring(3);
    
    if (/^[A-Z]$/.test(label)) return 'Key' + label;
    if (/^[0-9]$/.test(label)) return 'Digit' + label;
    
    const mapping = {
        'ENT': 'Enter', 'ENTER': 'Enter',
        'ESC': 'Escape',
        'SPC': 'Space', 'SPACE': 'Space',
        'BS': 'Backspace', 'BACKSPACE': 'Backspace',
        'DEL': 'Delete',
        'TAB': 'Tab',
        'LCTL': 'ControlLeft', 'RCTL': 'ControlRight',
        'LSFT': 'ShiftLeft', 'RSFT': 'ShiftRight',
        'LALT': 'AltLeft', 'RALT': 'AltRight',
        'LGUI': 'MetaLeft', 'LWIN': 'MetaLeft',
        'RGUI': 'MetaRight', 'RWIN': 'MetaRight',
        'UP': 'ArrowUp', 'DOWN': 'ArrowDown', 'LEFT': 'ArrowLeft', 'RGHT': 'ArrowRight', 'RIGHT': 'ArrowRight',
        'MINS': 'Minus', 'EQL': 'Equal',
        'LBRC': 'BracketLeft', 'RBRC': 'BracketRight',
        'BSLS': 'Backslash', 'SCLN': 'Semicolon',
        'QUOT': 'Quote', 'GRV': 'Backquote',
        'COMM': 'Comma', 'DOT': 'Period', 'SLSH': 'Slash',
        'CAPS': 'CapsLock',
        'NUM': 'NumLock', 'SCRL': 'ScrollLock', 'PAUS': 'Pause',
        'INS': 'Insert', 'HOME': 'Home', 'PGUP': 'PageUp', 'PGDN': 'PageDown', 'END': 'End',
        'PSCR': 'PrintScreen', 'PRTSC': 'PrintScreen',
        // JP Specific
        'JYEN': 'IntlYen', 'RO': 'IntlRo', 'HENK': 'Convert', 'MHEN': 'NonConvert', 'KANA': 'KanaMode'
    };
    if (mapping[label]) return mapping[label];
    
    // Reverse lookup from the symbol mapping in visual formatter
    const reverseSymbolMap = {
        '-': 'Minus', '=': 'Equal', '[': 'BracketLeft', ']': 'BracketRight',
        '\\': 'Backslash', ';': 'Semicolon', '\'': 'Quote', ',': 'Comma',
        '.': 'Period', '/': 'Slash', '`': 'Backquote'
    };
    if (reverseSymbolMap[orig]) return reverseSymbolMap[orig];

    // F keys
    if (/^F[0-9]{1,2}$/.test(label)) return label;

    // Numpad
    if (label.startsWith('P')) {
        let numpadMap = {
            'P1': 'Numpad1', 'P2': 'Numpad2', 'P3': 'Numpad3', 'P4': 'Numpad4',
            'P5': 'Numpad5', 'P6': 'Numpad6', 'P7': 'Numpad7', 'P8': 'Numpad8', 'P9': 'Numpad9', 'P0': 'Numpad0',
            'PDOT': 'NumpadDecimal', 'PENT': 'NumpadEnter', 'PPLS': 'NumpadAdd', 'PMNS': 'NumpadSubtract',
            'PAST': 'NumpadMultiply', 'PSLS': 'NumpadDivide'
        };
        if (numpadMap[label]) return numpadMap[label];
    }
    
    return orig; // Fallback
}

// --- DOM & State ---
const elTesterCanvasContainer = document.getElementById('testerCanvasContainer');
const elTesterCanvas = document.getElementById('testerCanvas');
const elTesterLogContainer = document.getElementById('testerLogContainer');
const btnTesterReset = document.getElementById('btnTesterReset');
const testerLayoutSelect = document.getElementById('testerLayoutSelect');
const testerLayoutStatus = document.getElementById('testerLayoutStatus');
const elTesterUploadJson = document.getElementById('testerUploadJson');

let testerLocalLayoutDef = null;
let testerKeys = [];
// Store DOM elements mapped by their expected event.code
let codeToDomMap = {}; 
let pressedCodes = new Set();
// To prevent standard shortcuts from activating while in Tester view
let isTesterViewActive = false;

// --- Rendering Logic ---
function renderTesterCanvas() {
    elTesterCanvas.innerHTML = '';
    codeToDomMap = {};
    pressedCodes.clear();

    const mode = testerLayoutSelect.value;
    let baseLayout = mode === '109' ? LAYOUT_109_JIS : LAYOUT_104_US;
    
    // If Custom Layout is selected, we fetch from the Builder App's keys state
    // builder_app.js exposes 'keys' globally, hid_app.js exposes 'state' globally.
    if (mode === 'custom') {
        let candidateDef = null;
        let sourceMsg = '';

        if (testerLocalLayoutDef && testerLocalLayoutDef.length > 0) {
            candidateDef = testerLocalLayoutDef;
            sourceMsg = '動作テスターで読み込んだレイアウト';
        } else if (typeof state !== 'undefined' && state.layoutDef && state.layoutDef.length > 0) {
            candidateDef = state.layoutDef;
            sourceMsg = 'キーマッピング画面から共有されたレイアウト';
        } else if (typeof keys !== 'undefined' && keys.length > 0) {
            candidateDef = keys;
            sourceMsg = '開発ユーザー画面から共有されたレイアウト';
        }

        if (candidateDef) {
            testerKeys = candidateDef.map(k => ({ ...k })); // clone
            testerLayoutStatus.textContent = `カスタム表示中 (${sourceMsg})`;
        } else {
            // Revert back and ask to upload
            testerLayoutSelect.value = '109';
            testerLayoutStatus.textContent = 'カスタムレイアウトが未設定です。「Upload JSON」からアップロードしてください。';
            if (elTesterUploadJson) elTesterUploadJson.click(); // Trigger upload window
            renderTesterCanvas();
            return;
        }
    } else {
        testerKeys = generateTesterKeysFromDef(baseLayout);
        testerLayoutStatus.textContent = `標準レイアウト (${mode === '109' ? '109日本語' : '104英語'}) を表示中`;
    }

    // Calculate max dimensions
    let maxX = 15;
    let maxY = 5;
    if (testerKeys.length > 0) {
        maxX = Math.max(...testerKeys.map(k => k.x + (k.w || 1)));
        maxY = Math.max(...testerKeys.map(k => k.y + (k.h || 1)));
    }

    // Dynamic sizing to eliminate whitespace
    const containerWidth = elTesterCanvasContainer.clientWidth > 0 ? elTesterCanvasContainer.clientWidth - 48 : 900;
    let computedUnit = Math.floor(containerWidth / maxX);
    const TESTER_UNIT = Math.min(Math.max(computedUnit, 30), 65);

    elTesterCanvas.style.width = Math.ceil(maxX * TESTER_UNIT) + 'px';
    elTesterCanvas.style.height = Math.ceil(maxY * TESTER_UNIT) + 'px';
    elTesterCanvasContainer.style.minHeight = 'auto'; // Remove 500px minimum

    testerKeys.forEach(k => {
        const div = document.createElement('div');
        div.className = 'layout-key tester-key'; 
        
        div.style.left = (k.x * TESTER_UNIT) + 'px';
        div.style.top = (k.y * TESTER_UNIT) + 'px';
        div.style.width = (k.w * TESTER_UNIT) + 'px';
        div.style.height = (k.h * TESTER_UNIT) + 'px';
        
        if (k.r) {
            div.style.transform = `rotate(${k.r}deg)`;
            let originX = ((k.rx || 0) - k.x) * TESTER_UNIT;
            let originY = ((k.ry || 0) - k.y) * TESTER_UNIT;
            div.style.transformOrigin = `${originX}px ${originY}px`;
        }
        
        // --- 特殊形状 (ISO Enter等) 対応 ---
        if (k.w2 > 0 || k.h2 > 0) {
            const shp = document.createElement('div');
            shp.className = 'key-shape-extension';
            shp.style.left = ((k.x2 || 0) * TESTER_UNIT) + 'px';
            shp.style.top = ((k.y2 || 0) * TESTER_UNIT) + 'px';
            shp.style.width = ((k.w2 || k.w) * TESTER_UNIT) + 'px';
            shp.style.height = ((k.h2 || k.h) * TESTER_UNIT) + 'px';
            div.appendChild(shp);
        }

        // Determine matching event.code
        let expectedCode = mode === 'custom' ? mapLabelToEventCode(k.label) : k.label;
        if (expectedCode) {
            div.dataset.code = expectedCode;
            if (!codeToDomMap[expectedCode]) codeToDomMap[expectedCode] = [];
            codeToDomMap[expectedCode].push(div);
        }

        // Use innerHTML instead of textContent to allow <br> for multi-line labels
        let labelText = formatKeyCode(k.label);
        labelText = labelText.replace(/\n/g, '<br>');
        let fontSize = (k.w || 1) < 1.25 && labelText.length > 5 ? '0.65rem' : '0.8rem';
        div.innerHTML = `<span style="text-align: center; line-height: 1.1; overflow: hidden; display:inline-block; font-size:${fontSize}; padding:0 2px;">${labelText}</span>`;
        
        elTesterCanvas.appendChild(div);
    });
}

// --- Event Handlers ---
window.addEventListener('keydown', (e) => {
    // Tester view active check (depends on top nav classes)
    const testerView = document.getElementById('view-tester');
    if (!testerView || testerView.classList.contains('hidden')) return;

    // Prevent default browser shortcuts (Ctrl+R, F5, etc.) ONLY IF they are typical tester issues
    // Actually, to fully test, we prevent default so we don't accidentally navigate or open DevTools unnecessarily.
    // Allow F12 for dev tools maybe? Let's just prevent default in Tester.
    if (e.code !== 'F12' && !e.ctrlKey) {
        e.preventDefault(); 
    }

    if (e.repeat) return; // Ignore key repetition for visuals
    
    pressedCodes.add(e.code);
    highlightKey(e.code, true);
    logEvent('↓ DOWN', e.code, e.key);
});

window.addEventListener('keyup', (e) => {
    const testerView = document.getElementById('view-tester');
    if (!testerView || testerView.classList.contains('hidden')) return;
    
    e.preventDefault();
    pressedCodes.delete(e.code);
    highlightKey(e.code, false);
    logEvent('↑ UP', e.code, e.key);
});

function highlightKey(code, isDown) {
    if (codeToDomMap[code]) {
        codeToDomMap[code].forEach(el => {
            if (isDown) {
                el.classList.add('active'); // CSS active (glow)
            } else {
                el.classList.remove('active');
                el.classList.add('tested'); // Indicates history
            }
        });
    }
}

function logEvent(action, code, key) {
    const time = new Date().toLocaleTimeString('ja-JP', { hour12: false, fractionalSecondDigits: 2 });
    const logDiv = document.createElement('div');
    if (action.includes('DOWN')) logDiv.style.color = '#00ffaa';
    else logDiv.style.color = '#aaa';
    
    // Formatting width
    const actionPad = action.padEnd(6, ' ');
    const codePad = code.padEnd(16, ' ');
    
    logDiv.textContent = `[${time}] ${actionPad} | Code: ${codePad} | Key: ${key}`;
    elTesterLogContainer.appendChild(logDiv);
    
    // Auto scroll bottom
    elTesterLogContainer.scrollTop = elTesterLogContainer.scrollHeight;
}

btnTesterReset.addEventListener('click', () => {
    elTesterLogContainer.innerHTML = '';
    document.querySelectorAll('.tester-key').forEach(el => {
        el.classList.remove('active', 'tested');
    });
});

testerLayoutSelect.addEventListener('change', () => {
    renderTesterCanvas();
});

// Initialization hook
const testerNavBtn = document.querySelector('.nav-item[data-target="tester"]');
if (testerNavBtn) {
    testerNavBtn.addEventListener('click', () => {
        // Render when activated
        setTimeout(() => renderTesterCanvas(), 50); 
    });
}

// Upload local JSON specifically for tester
if (elTesterUploadJson) {
    elTesterUploadJson.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if(!file) return;
        const reader = new FileReader();
        reader.onload = (evt) => {
            try {
                const def = JSON.parse(evt.target.result);
                // Simple parsing mimicking hid_app/builder_app logic
                const keymap = def.layouts && def.layouts.keymap ? def.layouts.keymap : def;
                if (Array.isArray(keymap)) {
                    let parsedKeys = [];
                    let curX = 0, curY = 0;
                    let cProps = { w: 1, h: 1, x: 0, y: 0, r: 0, rx: 0, ry: 0, w2: 0, h2: 0, x2: 0, y2: 0 };
                    
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
                    testerLocalLayoutDef = parsedKeys;
                    testerLayoutSelect.value = 'custom';
                    renderTesterCanvas();
                    alert("動作テスター専用にカスタムレイアウトを読み込みました。");
                } else {
                    alert("無効なJSONフォーマットです");
                }
            } catch(err) {
                alert("JSON読み込みエラー");
            }
            e.target.value = '';
        };
        reader.readAsText(file);
    });
}

window.addEventListener('resize', () => {
    const testerView = document.getElementById('view-tester');
    if (testerView && !testerView.classList.contains('hidden') && testerKeys.length > 0) {
        renderTesterCanvas();
    }
});
