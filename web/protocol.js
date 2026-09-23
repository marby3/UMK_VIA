// UIAPduino_VIA の WebHID クライアント。
// ファームウェアは VIA プロトコル (Raw HID / Usage Page 0xFF60 / 32 バイト) を
// 実装しているため、remap-keys.app と同じコマンド体系で通信します。

const VIA_USAGE_PAGE = 0xFF60;
const VIA_USAGE = 0x61;
const VIA_REPORT_SIZE = 32;

// VIA コマンドID (firmware/via.c と対応)
const VIA_CMD = {
    GET_PROTOCOL_VERSION:      0x01,
    GET_KEYBOARD_VALUE:        0x02,
    SET_KEYBOARD_VALUE:        0x03,
    DYNAMIC_KEYMAP_GET_KEYCODE: 0x04,
    DYNAMIC_KEYMAP_SET_KEYCODE: 0x05,
    DYNAMIC_KEYMAP_RESET:      0x06,
    DYNAMIC_KEYMAP_GET_LAYER_COUNT: 0x11,
    DYNAMIC_KEYMAP_GET_BUFFER: 0x12,
    DYNAMIC_KEYMAP_SET_BUFFER: 0x13,
};

// id_set_keyboard_value のサブコマンド。0xFF は「今すぐ不揮発領域へ保存」。
const VIA_STORE_KEYMAP_PERSISTENTLY = 0xFF;

// Low-Speed USB では 32 バイトのレポートが 8 バイト x 4 パケットに分割されるため、
// 往復に時間がかかります。余裕を持ったタイムアウトにしています。
const VIA_TIMEOUT_MS = 3000;

// 1コマンドで転送できるキーマップバッファのバイト数 (32 - コマンド1 - offset2 - size1)
const VIA_BUFFER_CHUNK_MAX = 28;

class UIAPduinoProtocol {
    constructor() {
        this.device = null;
        this.onDisconnect = null;
        this._pending = null;
        this._queue = Promise.resolve();
    }

    async connect() {
        if (!("hid" in navigator)) {
            alert("WebHID API is not supported in your browser. Please use Google Chrome or Microsoft Edge.");
            return false;
        }

        try {
            // VIA の Raw HID インターフェースのみを対象にする
            const filters = [{ usagePage: VIA_USAGE_PAGE, usage: VIA_USAGE }];
            const devices = await navigator.hid.requestDevice({ filters });

            if (devices.length === 0) return false;

            // 複合デバイスの場合、キーボードではなく VIA 用のコレクションを持つ口を選ぶ
            const targetDevice = devices.find(d =>
                d.collections.some(c => c.usagePage === VIA_USAGE_PAGE && c.usage === VIA_USAGE)
            ) || devices[0];

            this.device = targetDevice;
            if (!this.device.opened) await this.device.open();

            navigator.hid.addEventListener('disconnect', (event) => {
                if (this.device && event.device === this.device) {
                    this.device = null;
                    if (this.onDisconnect) this.onDisconnect();
                }
            });

            this.device.addEventListener('inputreport', (event) => this._handleInputReport(event));

            return true;
        } catch (e) {
            console.error("HID Connection Error:", e);
        }
        return false;
    }

    get isConnected() {
        return this.device !== null && this.device.opened;
    }

    _handleInputReport(event) {
        const data = new Uint8Array(event.data.buffer);
        const pending = this._pending;
        if (!pending) return;

        // ファームウェアは要求内容をエコーバックするので、先頭バイトの一致で照合する
        for (let i = 0; i < pending.match.length; i++) {
            if (data[i] !== pending.match[i]) return;
        }

        clearTimeout(pending.timer);
        this._pending = null;
        pending.resolve(data);
    }

    // 1コマンドずつ直列に実行する (デバイス側は同時に1つの応答しか保持しません)
    _command(payload, matchLength) {
        const run = async () => {
            if (!this.isConnected) throw new Error('Device is not connected.');

            const report = new Uint8Array(VIA_REPORT_SIZE);
            report.set(payload.slice(0, VIA_REPORT_SIZE));

            const response = new Promise((resolve, reject) => {
                this._pending = {
                    match: report.slice(0, matchLength),
                    resolve,
                    reject,
                    timer: setTimeout(() => {
                        this._pending = null;
                        reject(new Error(`VIA command 0x${report[0].toString(16)} timed out`));
                    }, VIA_TIMEOUT_MS)
                };
            });

            // reportId = 0 (レポートIDなしのディスクリプタ)
            await this.device.sendReport(0, report);
            return response;
        };

        this._queue = this._queue.then(run, run);
        return this._queue;
    }

    async getProtocolVersion() {
        const res = await this._command([VIA_CMD.GET_PROTOCOL_VERSION], 1);
        return (res[1] << 8) | res[2];
    }

    async getLayerCount() {
        const res = await this._command([VIA_CMD.DYNAMIC_KEYMAP_GET_LAYER_COUNT], 1);
        return res[1];
    }

    async getKeycode(layer, row, col) {
        const res = await this._command(
            [VIA_CMD.DYNAMIC_KEYMAP_GET_KEYCODE, layer, row, col], 4);
        return (res[4] << 8) | res[5];
    }

    async setKeycode(layer, row, col, keycode) {
        await this._command(
            [VIA_CMD.DYNAMIC_KEYMAP_SET_KEYCODE, layer, row, col,
             (keycode >> 8) & 0xFF, keycode & 0xFF], 6);
        return true;
    }

    // キーマップ全体をまとめて読み出す (1キーずつ読むより大幅に高速)
    async readKeymapBuffer(offset, size) {
        const chunk = Math.min(size, VIA_BUFFER_CHUNK_MAX);
        const res = await this._command(
            [VIA_CMD.DYNAMIC_KEYMAP_GET_BUFFER, (offset >> 8) & 0xFF, offset & 0xFF, chunk], 4);
        return res.slice(4, 4 + chunk);
    }

    // レイヤー数 x 行 x 列 のキーマップを一括取得する
    async readAllKeymaps(layerCount, rows, cols) {
        const layerBytes = rows * cols * 2;
        const totalBytes = layerBytes * layerCount;
        const buffer = new Uint8Array(totalBytes);

        for (let offset = 0; offset < totalBytes; offset += VIA_BUFFER_CHUNK_MAX) {
            const size = Math.min(VIA_BUFFER_CHUNK_MAX, totalBytes - offset);
            const part = await this.readKeymapBuffer(offset, size);
            buffer.set(part.slice(0, size), offset);
        }

        // ビッグエンディアンの 16bit キーコードへ復元する
        const keymap = [];
        for (let l = 0; l < layerCount; l++) {
            const layer = [];
            for (let r = 0; r < rows; r++) {
                const row = [];
                for (let c = 0; c < cols; c++) {
                    const idx = l * layerBytes + (r * cols + c) * 2;
                    row.push((buffer[idx] << 8) | buffer[idx + 1]);
                }
                layer.push(row);
            }
            keymap.push(layer);
        }
        return keymap;
    }

    // キーマップをファームウェアの初期値へ戻す
    async resetKeymap() {
        await this._command([VIA_CMD.DYNAMIC_KEYMAP_RESET], 1);
        return true;
    }

    // 変更内容を今すぐ内蔵Flashへ保存する。
    // (通常はキー変更後しばらくすると自動保存されるため、明示的な保存は任意です)
    async saveToFlash() {
        const res = await this._command(
            [VIA_CMD.SET_KEYBOARD_VALUE, VIA_STORE_KEYMAP_PERSISTENTLY], 2);
        return res[2] === 0;
    }
}
