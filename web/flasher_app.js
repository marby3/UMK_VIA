// Web Flasher (フェーズ5)
// rv003usb-webflasher (MIT License) を基盤にUIAPduino向けにリファクタリング・統合

// --- 書き込みコアアルゴリズム ---
const USB_VID = 0x1209;
const USB_PID = 0xB003;
const FLASH_BASE = 0x08000000;
const FLASH_SIZE = 16384; 
const SECTOR_SIZE = 64;

function dataview_to_uint8array(dataview) {
    let ret = new Uint8Array(dataview.byteLength);
    for(let i=0; i<dataview.byteLength; i++){
        ret[i] = dataview.getUint8(i);
    }
    return ret;
}

function payload_obtain_uint8array(content) {
    const payload_size = 128;
    const blob_prefix = [0xaa, 0x00, 0x00, 0x00];
    const blob_suffix = [0xcd, 0xab, 0x34, 0x12];
    if(blob_prefix.length + blob_suffix.length + content.length > payload_size) {
        throw new Error("Payload content too long!");
    }

    let ret = new Uint8Array(payload_size);
    for(let i=0; i<blob_prefix.length; i++) {
        ret[i] = blob_prefix[i];
    }
    for(let i=0; i<blob_suffix.length; i++) {
        ret[ret.length-blob_suffix.length+i] = blob_suffix[i];
    }
    for(let i=0; i<content.length; i++) {
        ret[blob_prefix.length+i] = content[i]
    }
    return ret;
}

function uint32value_to_array(uint32value) {
    if(uint32value < 0 || uint32value > 2**32-1) {
        throw new Error("uint32value out of range");
    }
    let ret = [];
    for(let i=0; i<4; i++) {
        ret.push(uint32value & 0xFF);
        uint32value = Math.floor(uint32value / 256);
    }
    return ret;
}

function build_halt_wait_payload() {
    return payload_obtain_uint8array([0x81, 0x46, 0x94, 0xc1, 0xfd, 0x56, 0x14, 0xc1, 0x82, 0x80]);
}

function build_read_payload(address, size) {
    let payload = [
        0x23, 0xa0, 0x05, 0x00, 0x13, 0x07, 0x45, 0x03, 0x0c, 0x43, 0x50, 0x43,
        0x2e, 0x96, 0x21, 0x07, 0x94, 0x41, 0x14, 0xc3, 0x91, 0x05, 0x11, 0x07,
        0xe3, 0xcc, 0xc5, 0xfe, 0x93, 0x06, 0xf0, 0xff, 0x14, 0xc1, 0x82, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    payload = payload.concat(uint32value_to_array(address));
    payload = payload.concat(uint32value_to_array(size));
    return payload_obtain_uint8array(payload);
}

function build_write_payload(address, size, content) {
    let payload = [
        0x23, 0xa0, 0x05, 0x00, 0x13, 0x07, 0x45, 0x03, 0x0c, 0x43, 0x50, 0x43,
        0x2e, 0x96, 0x21, 0x07, 0x14, 0x43, 0x94, 0xc1, 0x91, 0x05, 0x11, 0x07,
        0xe3, 0xcc, 0xc5, 0xfe, 0x93, 0x06, 0xf0, 0xff, 0x14, 0xc1, 0x82, 0x80, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    payload = payload.concat(uint32value_to_array(address));
    payload = payload.concat(uint32value_to_array(size));
    payload = payload.concat(content);
    return payload_obtain_uint8array(payload);
}

function build_write64_flash_payload(address, content) {
    let payload = [
        0x13, 0x07, 0x45, 0x03, 0x0c, 0x43, 0x13, 0x86, 0x05, 0x04, 0x5c, 0x43,
        0x8c, 0xc7, 0x14, 0x47, 0x94, 0xc1, 0xb7, 0x06, 0x05, 0x00, 0xd4, 0xc3,
        0x94, 0x41, 0x91, 0x05, 0x11, 0x07, 0xe3, 0xc8, 0xc5, 0xfe, 0xc1, 0x66,
        0x93, 0x86, 0x06, 0x04, 0xd4, 0xc3, 0xfd, 0x56, 0x14, 0xc1, 0x82, 0x80];
    payload = payload.concat(uint32value_to_array(address));
    payload = payload.concat(uint32value_to_array(0x4002200C)); 
    for(let i=0; i<content.length; i++) payload.push(content[i]); // unpack
    return payload_obtain_uint8array(payload);
}

function build_run_app_payload() {
    let payload = [
        0xb7,0xf5,0xff,0x1f, 0x93,0x87,0xc5,0x77, 0x03,0xa7,0x07,0x00, 0x13,0x57,0x07,0x01,
        0x83,0x96,0x07,0x00, 0x93,0xc7,0xc6,0x77, 0x63,0x16,0xf7,0x00, 0x33,0x87,0xb6,0x00,
        0x67,0x00,0x07,0x00,
        0xb7,0x27,0x02,0x40, 0x93,0x87,0x87,0x02, 0x37,0x07,0x67,0x45, 0x13,0x07,0x37,0x12,
        0x23,0xa0,0xe7,0x00, 0xb7,0x27,0x02,0x40, 0x93,0x87,0x87,0x02, 0x37,0x97,0xef,0xcd,
        0x13,0x07,0xb7,0x9a, 0x23,0xa0,0xe7,0x00, 0xb7,0x27,0x02,0x40, 0x93,0x87,0xc7,0x00,
        0x23,0xa0,0x07,0x00, 0xb7,0x27,0x02,0x40, 0x93,0x87,0x07,0x01, 0x13,0x07,0x00,0x08,
        0x23,0xa0,0xe7,0x00, 0xb7,0xf7,0x00,0xe0, 0x93,0x87,0x07,0xd1, 0x37,0x07,0x00,0x80,
        0x23,0xa0,0xe7,0x00,
    ];
    return payload_obtain_uint8array(payload);
}

async function communicate_usb(device, command, readback=true, logCb) {
    let retries = 0;
    while(1) {
        try {
            await device.sendFeatureReport(command[0], command.slice(1));
            break;
        } catch (error) {
            if(retries++ > 10) {
                if(logCb) logCb("ERROR: sendFeatureReport retries exceeded!");
                return null;
            }
        }
    }

    if(!readback) return 0;

    retries = 0;
    let timeout = 0;
    let response = null;
    while(1) {
        try {
            let dataview = await device.receiveFeatureReport(command[0]);
            response = dataview_to_uint8array(dataview);
            if(response.byteLength === command.byteLength && response[1] === 0xff) {
                break;
            } else if(timeout++ > 20) {
                if(logCb) logCb("ERROR: receiveFeatureReport timeout!");
                return null;
            }
        } catch (error) {
            if(retries++ > 10) {
                if(logCb) logCb("ERROR: receiveFeatureReport retries exceeded!");
                return null;
            }
        }
    }
    return response;
}

async function communicate_halt_wait(device, logCb) {
    return await communicate_usb(device, build_halt_wait_payload(), true, logCb);
}

async function communicate_read_word(device, address, logCb) {
    let result = await communicate_usb(device, build_read_payload(address, 4), true, logCb);
    if(result === null || result.length < 64) return null;
    let ret = 0;
    for(let i=0; i<4; i++) ret += result[60+i] * (2**(i*8));
    return ret;
}

async function communicate_write_word(device, address, data, logCb) {
    let result = await communicate_usb(device, build_write_payload(address, 4, uint32value_to_array(data)), true, logCb);
    if(result === null || result.length < 64) return null;
    let ret = 0;
    for(let i=0; i<4; i++) ret += result[60+i] * (2**(i*8));
    return ret;
}

async function communicate_verify64(device, address, expected_data, logCb) {
    let result = await communicate_usb(device, build_read_payload(address, 64), true, logCb);
    if(result === null || result.length < 60+64) return null;
    for(let i=0; i<64; i++) {
        if(expected_data[i] !== result[60+i]) return false;
    }
    return true;
}

async function communicate_flash64(device, address, data, logCb) {
    if(await communicate_write_word(device, 0x40022010, 0x00020000, logCb) === null) return null;
    if(await communicate_write_word(device, 0x40022014, address, logCb) === null) return null;
    if(await communicate_write_word(device, 0x40022010, 0x00020040, logCb) === null) return null;
    
    let result = 0x03;
    let timeout = 0;
    do {
        result = await communicate_read_word(device, 0x4002200C, logCb);
        if(result === null) {
            if(logCb) logCb("ERROR: Flash wait communication error!");
            return null;
        } else if(timeout++ > 1000) {
            if(logCb) logCb("WARNING: Flash erase timed out. STATR = " + result);
            return null;
        }
    } while(result & 0x03);

    if(result & 0x00000010) {
        if(logCb) logCb("ERROR: Memory Protection Error");
        return null;
    }
    
    if(await communicate_write_word(device, 0x40022010, 0x00010000, logCb) === null) return null;
    if(await communicate_write_word(device, 0x40022010, 0x00090000, logCb) === null) return null;

    if(await communicate_usb(device, build_write64_flash_payload(address, data), true, logCb) === null) {
        if(logCb) logCb("ERROR: flash write64 error");
        return null;
    }
    return 0;
}

async function communicate_run_app(device, logCb) {
    return await communicate_usb(device, build_run_app_payload(), false, logCb);
}

async function communicate_flash_unlock(device, logCb) {
    let rw = await communicate_read_word(device, 0x40022010, logCb);
    if (rw === null) return null;

    if(rw & 0x8080) {
        if(await communicate_write_word(device, 0x40022004, 0x45670123, logCb) === null) return null;
        if(await communicate_write_word(device, 0x40022004, 0xCDEF89AB, logCb) === null) return null;
        if(await communicate_write_word(device, 0x40022008, 0x45670123, logCb) === null) return null;
        if(await communicate_write_word(device, 0x40022008, 0xCDEF89AB, logCb) === null) return null;
        if(await communicate_write_word(device, 0x40022024, 0x45670123, logCb) === null) return null;
        if(await communicate_write_word(device, 0x40022024, 0xCDEF89AB, logCb) === null) return null;

        rw = await communicate_read_word(device, 0x40022010, logCb);
        if (rw === null || rw & 0x8080) {
            if(logCb) logCb("ERROR: Flash unlock failure " + rw);
            return null;
        }
    }
    
    rw = await communicate_read_word(device, 0x4002201c, logCb);
    if(rw === null) return null;
    if(rw & 2) {
        if(logCb) logCb("WARNING: Flash [read] locked. Cannot program unless unlocked.");
        return null;
    }
    return 0;
}

function pad_rom_to_64(content) {
    let ret = new Uint8Array(Math.floor((content.byteLength+63)/64)*64);
    ret.fill(0xFF);
    ret.set(content, 0);
    return ret;
}

async function flash_ch32v003_firmware(uint8arraycontent, status_callback, logCb) {
    let image_content = pad_rom_to_64(uint8arraycontent);
    if(image_content.byteLength > FLASH_SIZE) {
        if(logCb) logCb("ERROR: ROM size too large! (" + image_content.byteLength + " bytes)");
        return null;
    }

    status_callback({step: 1, text: "Requsting USB Device..."});
    let device = null;
    try {
        // CH32系およびコミュニティ用VIDのみフィルタリング (ProductIdは可変対応)
        let devs = await navigator.hid.requestDevice({ filters: [{ vendorId: 0x1209 }, { vendorId: 0x1A86 }] });
        if(devs.length === 0) throw new Error("No device selected");
        device = devs[0];
        await device.open();
        if(logCb) logCb("Opened device: " + device.productName);
    } catch (error) {
        if(logCb) logCb("ERROR: device.open() failed. Please make sure the device is plugged in while holding the Boot button (or it's an empty MCU).");
        return null;
    }

    status_callback({step: 2, text: "Halting MCU via WebHID..."});
    if(await communicate_halt_wait(device, logCb) === null) {
        if(logCb) logCb("ERROR: communicate_halt_wait() failed");
        return null;
    }

    status_callback({step: 3, text: "Unlocking Flash Memory..."});
    if(await communicate_flash_unlock(device, logCb) === null) {
        if(logCb) logCb("ERROR: communicate_flash_unlock() failed");
        return null;
    }

    let difference_found = true;
    let retries = 0;
    while(difference_found && retries < 5) {
        difference_found = false;
        retries++;
        for(let i=0; i<image_content.byteLength; i+=SECTOR_SIZE) {
            let address = FLASH_BASE+i;
            let image_chunk = image_content.slice(i, i+SECTOR_SIZE);
            status_callback({
                step: difference_found ? 4 : 5, 
                text: difference_found ? "Retrying Flash Write..." : "Flashing Sector...",
                offset: i, 
                size: image_content.byteLength
            });
            
            if(!(await communicate_verify64(device, address, image_chunk, logCb))) {
                if(!difference_found) difference_found = true;
                if(await communicate_flash64(device, address, image_chunk, logCb) === null) {
                    if(logCb) logCb("ERROR: Unable to write flash at offset 0x" + (FLASH_BASE+i).toString(16));
                    return null;
                }
            }
        }
    }

    if(difference_found) {
        if(logCb) logCb("ERROR: Unable to write flash with correct content after multiple retries");
        return null;
    }

    status_callback({step: 6, text: "Rebooting into App...", offset: image_content.byteLength, size: image_content.byteLength});
    await communicate_run_app(device, logCb); // Ignore failure here, device will disconnect
    status_callback({step: 7, text: "Completed! Successfully Flashed.", offset: image_content.byteLength, size: image_content.byteLength});
    return true;
}

// --- UI Logic ---
document.addEventListener('DOMContentLoaded', () => {
    const elFlasherLog = document.getElementById('flasherLog');
    const elFlasherProgress = document.getElementById('flasherProgress');
    const elFlasherProgressBar = document.getElementById('flasherProgressBar');
    const elFlasherProgressText = document.getElementById('flasherProgressText');
    const elFlasherStatusText = document.getElementById('flasherStatusText');
    const btnFlashAction = document.getElementById('btnFlashAction');
    const flasherFileInput = document.getElementById('flasherFileInput');
    const flasherFileWrapper = document.getElementById('flasherFileWrapper');
    const flasherCacheWarning = document.getElementById('flasherCacheWarning');
    const flasherToKeymapModal = document.getElementById('flasherToKeymapModal');

    let currentFirmwareBytes = null; // Uint8Array

    function appendLog(str) {
        elFlasherLog.textContent += str + '\n';
        elFlasherLog.scrollTop = elFlasherLog.scrollHeight;
    }

    function updateProgressBar(progress) {
        elFlasherProgress.classList.remove('hidden');
        elFlasherProgressBar.style.width = progress + '%';
        elFlasherProgressText.textContent = Math.floor(progress) + '%';
    }

    // Builder側でキャッシュされたデータを読み込む
    function attachCachedBuild() {
        if (window.UIAPduinoCache && window.UIAPduinoCache.firmwareBin) {
            currentFirmwareBytes = window.UIAPduinoCache.firmwareBin;
            btnFlashAction.disabled = false;
            flasherCacheWarning.classList.remove('hidden');
            flasherFileWrapper.querySelector('.help-text').textContent = "（アップロードするとキャッシュより優先されます）";
            appendLog('> Local build cache found. Ready to flash ' + currentFirmwareBytes.byteLength + ' bytes.');
        } else {
            flasherCacheWarning.classList.add('hidden');
            if(!flasherFileInput.files[0]) {
                btnFlashAction.disabled = true;
            }
        }
    }

    // タブが切り替わったときにキャッシュを再確認
    const navItems = document.querySelectorAll('.nav-items .nav-item');
    navItems.forEach(item => {
        item.addEventListener('click', () => {
            if (item.getAttribute('data-target') === 'view-flash') {
                attachCachedBuild();
            }
        });
    });
    
    // 手動ファイルアップロード（常にキャッシュより優先）
    flasherFileInput.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (evt) => {
            const arrayBuffer = evt.target.result;
            currentFirmwareBytes = new Uint8Array(arrayBuffer);
            flasherCacheWarning.classList.add('hidden'); // 手動アップロード時は警告を消す
            btnFlashAction.disabled = false;
            appendLog(`> External file loaded: ${file.name} (${currentFirmwareBytes.byteLength} bytes). Ready to flash.`);
        };
        reader.onerror = () => {
            appendLog('> ERROR: FileReader failed reading file.');
        };
        reader.readAsArrayBuffer(file);
    });

    btnFlashAction.addEventListener('click', async () => {
        if (!currentFirmwareBytes) {
            alert("ファームウェア（.bin）を選択してください。");
            return;
        }

        const isBrowserSupported = ("hid" in navigator);
        if (!isBrowserSupported) {
            alert("このブラウザはWebHIDに対応していません。Chromeベースのブラウザを使用してください。");
            return;
        }

        btnFlashAction.disabled = true;
        elFlasherLog.textContent = '> Starting Web Flasher process...\n';
        appendLog("> Requesting target device...");

        updateProgressBar(0);
        elFlasherStatusText.textContent = "待機中...";
        elFlasherStatusText.classList.remove('hidden');

        try {
            const success = await flash_ch32v003_firmware(
                currentFirmwareBytes,
                (status) => {
                    const { step, text, offset, size } = status;
                    if(text) elFlasherStatusText.textContent = text;
                    if(offset !== undefined && size && size > 0) {
                        updateProgressBar(Math.min(100, Math.floor((offset / size) * 100)));
                    } else if (step === 1) {
                         elFlasherStatusText.textContent = "デバイス待機中... (Bootモードで接続してください)";
                    }
                },
                (msg) => {
                    appendLog(msg);
                }
            );

            if (success) {
                updateProgressBar(100);
                elFlasherStatusText.textContent = "完了！再起動しました。";
                appendLog("\n--- FLASHING SUCCESSFUL ---");
                flasherToKeymapModal.classList.remove('hidden');
                
                // キャッシュを消費済みとしてクリアするかどうか。今回はあえて残しておく方針だが、
                // 誤書き込み防止のためにクリアしておく。
                if(window.UIAPduinoCache) window.UIAPduinoCache.firmwareBin = null;
                currentFirmwareBytes = null; // リセット
                flasherFileInput.value = ''; // Inputリセット
                attachCachedBuild(); // ボタンの無効化など
            } else {
                appendLog("\n--- FLASHING FAILED ---");
                elFlasherStatusText.textContent = "エラーが発生しました";
                btnFlashAction.disabled = false;
            }

        } catch(err) {
            appendLog(`\nERROR: ${err.message}`);
            elFlasherStatusText.textContent = "中断しました";
            btnFlashAction.disabled = false;
        }
    });

    // Modal behavior
    document.getElementById('btnNavToKeymap').addEventListener('click', () => {
        flasherToKeymapModal.classList.add('hidden');
        document.querySelector('.nav-item[data-target="view-keymap"]').click();
    });
    document.getElementById('btnStayOnFlasher').addEventListener('click', () => {
        flasherToKeymapModal.classList.add('hidden');
    });
});
