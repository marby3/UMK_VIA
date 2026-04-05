// Firmware Builder Logic
const CH32V003_PINS = [
    'PA1', 'PA2',
    'PC0', 'PC1', 'PC2', 'PC3', 'PC4', 'PC5', 'PC6', 'PC7',
    'PD0', 'PD1', 'PD2', 'PD3', 'PD4', 'PD5', 'PD6', 'PD7'
];

const elDevRows = document.getElementById('devRows');
const elDevCols = document.getElementById('devCols');
const elMatrixPinsContainer = document.getElementById('matrixPinsContainer');

const elEnableRGB = document.getElementById('enableRGB');
const elRgbConfig = document.getElementById('rgbConfig');
const elPinRgbDin = document.getElementById('pinRgbDin');

const elBtnAddEncoder = document.getElementById('btnAddEncoder');
const elEncoderList = document.getElementById('encoderList');
let encoderCount = 0;

const btnBuildFirmware = document.getElementById('btnBuildFirmware');
const buildLog = document.getElementById('buildLog');

function createPinSelect(id) {
    let sel = document.createElement('select');
    sel.className = 'modern-select pin-select';
    if(id) sel.id = id;
    
    let opt = document.createElement('option');
    opt.value = "";
    opt.textContent = "未割当";
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
    
    // Rows
    for(let r=0; r<rows; r++){
        const rowDiv = document.createElement('div');
        rowDiv.className = 'pin-row';
        rowDiv.innerHTML = `<span class="pin-label">Row${r}</span>`;
        rowDiv.appendChild(createPinSelect(`pin_row_${r}`));
        elMatrixPinsContainer.appendChild(rowDiv);
    }
    
    // Cols
    for(let c=0; c<cols; c++){
        const colDiv = document.createElement('div');
        colDiv.className = 'pin-row';
        colDiv.innerHTML = `<span class="pin-label">Col${c}</span>`;
        colDiv.appendChild(createPinSelect(`pin_col_${c}`));
        elMatrixPinsContainer.appendChild(colDiv);
    }
}

elDevRows.addEventListener('change', updateMatrixPinsUI);
elDevCols.addEventListener('change', updateMatrixPinsUI);

// RGB
elEnableRGB.addEventListener('change', (e) => {
    if(e.target.checked) elRgbConfig.classList.remove('hidden');
    else elRgbConfig.classList.add('hidden');
});
elPinRgbDin.replaceWith(createPinSelect('pinRgbDin'));

// Encoders
elBtnAddEncoder.addEventListener('click', () => {
    encoderCount++;
    const div = document.createElement('div');
    div.className = 'encoder-item';
    div.innerHTML = `<span>#${encoderCount} A:</span>`;
    div.appendChild(createPinSelect(`pin_enc_${encoderCount}_A`));
    div.innerHTML += `<span>B:</span>`;
    div.appendChild(createPinSelect(`pin_enc_${encoderCount}_B`));
    elEncoderList.appendChild(div);
});

// Build Dummy Action
function appendLog(str) {
    buildLog.textContent += '\n' + str;
    buildLog.parentElement.scrollTop = buildLog.parentElement.scrollHeight;
}

btnBuildFirmware.addEventListener('click', () => {
    buildLog.textContent = '> Starting Cloud Build Process...';
    btnBuildFirmware.disabled = true;
    appendLog('Initializing ch32v003fun environment...');
    setTimeout(() => appendLog('Generating matrix configuration... OK'), 500);
    setTimeout(() => appendLog('Compiling core... OK'), 1000);
    setTimeout(() => {
        const hasRGB = elEnableRGB.checked;
        if(hasRGB) appendLog('Compiling RGB LED (WS2812) subsystem... OK');
    }, 1500);
    setTimeout(() => {
        appendLog('Linking firmware...');
        appendLog('Size check: 10KB / 16KB (Valid)');
    }, 2000);
    setTimeout(() => {
        appendLog('Build SUCCESS 🎉');
        appendLog('Output size is within 16KB CH32V003 limit.');
        btnBuildFirmware.disabled = false;
    }, 2500);
});

// Init
updateMatrixPinsUI();
