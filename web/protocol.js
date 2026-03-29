const COMMAND_WRITE_KEY = 0x01;
const COMMAND_SAVE_FLASH = 0x99;

class UIAPduinoProtocol {
    constructor() {
        this.device = null;
        this.onDisconnect = null;
    }

    async connect() {
        if (!("hid" in navigator)) {
            alert("WebHID API is not supported in your browser. Please use Google Chrome or Microsoft Edge.");
            return false;
        }

        try {
            // UIAPduino_VIAの VID: 0x1209, PID: 0xC003 を指定 (usb_config.hの定義依存)
            const filters = [{ vendorId: 0x1209, productId: 0xc003 }];
            const devices = await navigator.hid.requestDevice({ filters });
            
            if (devices.length > 0) {
                // 複合デバイスの場合、キーボード(0x01)ではなくカスタムHID(0xFF00)の口を探す
                let targetDevice = devices.find(d => 
                    d.collections.some(c => c.usagePage === 0xFF00)
                );
                
                // 見つからなければ最初のものをフォールバック
                if (!targetDevice) targetDevice = devices[0];

                this.device = targetDevice;
                await this.device.open();
                
                // デバイスの物理的切断イベントを購読
                navigator.hid.addEventListener('disconnect', (event) => {
                    if (this.device && event.device.vendorId === this.device.vendorId && event.device.productId === this.device.productId) {
                        this.device = null;
                        if (this.onDisconnect) this.onDisconnect();
                    }
                });

                // デバイスからの入力レポートを受信
                this.device.addEventListener('inputreport', (event) => {
                    const data = new Uint8Array(event.data.buffer);
                    if (this.onInputReport) this.onInputReport(data);
                });
                
                return true;
            }
        } catch (e) {
            console.error("HID Connection Error:", e);
        }
        return false;
    }

    async writeKey(layer, row, col, keycode) {
        if (!this.isConnected) return false;
        
        // 8バイトの送信ペイロードを構築
        const payload = new Uint8Array(8);
        payload[0] = COMMAND_WRITE_KEY;
        payload[1] = layer;
        payload[2] = row;
        payload[3] = col;
        payload[4] = (keycode >> 8) & 0xFF; // Keycode High-byte (Modifiers / Layers flags)
        payload[5] = keycode & 0xFF;        // Keycode Low-byte
        payload[6] = 0x00;
        payload[7] = 0x00;
        
        try {
            // reportId=0 に対して書き込み (カスタムHIDの送信)
            await this.device.sendReport(0, payload);
            return true;
        } catch (e) {
            console.error("WebHID writeKey failed:", e);
            return false;
        }
    }

    async readKeyRequest(layer, row, col) {
        if (!this.isConnected) return false;
        
        const payload = new Uint8Array(8);
        payload[0] = 0x02; // read command
        payload[1] = layer;
        payload[2] = row;
        payload[3] = col;
        payload[4] = 0;
        payload[5] = 0;
        payload[6] = 0;
        payload[7] = 0;
        
        try {
            await this.device.sendReport(0, payload);
            return true;
        } catch (e) {
            return false;
        }
    }

    async saveToFlash() {
        if (!this.isConnected) return false;
        
        const payload = new Uint8Array(8);
        payload[0] = COMMAND_SAVE_FLASH;
        
        try {
            await this.device.sendReport(0, payload);
            return true;
        } catch (e) {
            console.error("WebHID saveToFlash failed:", e);
            return false;
        }
    }
    
    get isConnected() {
        return this.device !== null && this.device.opened;
    }
}
