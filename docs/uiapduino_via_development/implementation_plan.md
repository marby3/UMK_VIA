# 目標 (Goal)

ブラウザのWebHIDを用いて動的にキーマップを変更・保存できる、CH32V003F4P6ベースのミニマム（Flash: 16KB / RAM: 2KB）カスタムキーボードファームウェアおよびWebフロントエンドの開発を行います。
指定された5つのフェーズに従って、軽量化と最適化に徹底的に配慮したC言語ベースのファームウェア、及び機能ファーストでのWebフロントエンド（UIAPduino_VIA）を構築します。

## 合意された仕様・制約事項
- **Flashメモリへの書き込み**: CH32V003は**合計16KBのFlash**を持ち、ページ（書き込み消去の最小単位）は64バイトです。キーマップが192バイトの場合でも、最後尾から3ページ分を消費するだけであり、ファームウェア用の領域は15KB以上残るため**極めて安全に実現可能**です。
- **GPIOピン割り当て**: ユーザーが後から設定しやすいよう、ハードコーディングは避け `#define MATRIX_ROW_PINS { ... }` などのマクロでのみ定義し、汎用的なスキャンロジックを実装します。
- **依存ライブラリ**: `ch32v003fun` と `rv003usb` は `git submodule` を用いて取得する構成に `Makefile` および連携スクリプトを構築します。
- **Web フロントエンド**: 初期実装はCore機能（HID通信・パケット生成）をメインに構築し、動作が確立した段階で洗練されたUI/UXを適用する方針とします。

## 提案される変更 (Proposed Changes)

フェーズごとに以下のファイル群を構築・変更します。

### Phase 1: USB通信基盤の構築
*複合デバイス（キーボード＋カスタムHID）として認識される基盤作成*
- `[NEW]` [usb_config.h](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/usb_config.h) : rv003usbのディスクリプタ定義（1キーボード＋1カスタムHID）。複合デバイス構成。
- `[NEW]` [usb_config.c](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/usb_config.c) : ディスクリプタのデータ配列実装。
- `[NEW]` [main.c](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/main.c) : `SystemInit()` と `usb_setup()` を呼び出し、PCと正しく通信できるかのベースを構築。
- `[NEW]` [Makefile](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/Makefile) : ch32v003funを利用し、submoduleの利用と`-Os` 等の最適化フラグを含んだビルド設定。

---

### Phase 2: キーボード機能の実装
*マトリックススキャンとキーコード生成*
- `[NEW]` [matrix.h](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/matrix.h)
- `[NEW]` [matrix.c](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/matrix.c) : ユーザー定義可能なGPIO初期化、マトリックススキャン、チャタリング除去（デバウンス）。
- `[NEW]` [keymap.h](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/keymap.h)
- `[NEW]` [keymap.c](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/keymap.c) : RAM上のキーマップ多次元配列とキー取得関数。

---

### Phase 3: WebHID通信とキーマップ動的変更機能
*PCからのコマンド受信とRAM書き換え*
- `[MODIFY]` [main.c](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/main.c) : `usb_handle_custom_hid_out()` などのコールバックを追加。受信した8バイトコマンド (`id = 0x01`) 解析とRAM書き換え。

---

### Phase 4: Flashメモリへの保存機能
*不揮発性ストレージへの永続化*
- `[NEW]` [flash_store.h](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/flash_store.h)
- `[NEW]` [flash_store.c](file:///c:/Users/Marby/Documents/UIAPduino_VIA/firmware/flash_store.c) : `id = 0x99` のコマンド受信時にFlashロック解除、最終ページ軍消去、RAM書き込み、`NVIC_SystemReset()`の呼び出し。

---

### Phase 5: Webフロントエンド開発
*機能ファーストでのWebHIDアプリケーション構築*
- `[NEW]` [index.html](file:///c:/Users/Marby/Documents/UIAPduino_VIA/web/index.html), `[NEW]` [hid_app.js](file:///c:/Users/Marby/Documents/UIAPduino_VIA/web/hid_app.js), `[NEW]` [protocol.js](file:///c:/Users/Marby/Documents/UIAPduino_VIA/web/protocol.js) : WebHID接続、8バイトパケット生成・送信などのコア処理。
- `[NEW]` [styles.css](file:///c:/Users/Marby/Documents/UIAPduino_VIA/web/styles.css) : コア機能の実装完了後、デザインを適用。

## 検証計画 (Verification Plan)
- 各機能ディレクトリおよびビルド環境の検証
- デバイスの列挙・複合USB接続の動作確認 (Phase 1)
- マトリックスの動作およびPCへのキーコード送信テスト (Phase 2)
- WebHIDツールを用いたバルク転送/OUTパケットのパース確認 (Phase 3)
- デバイス再起動後のキーマップ保持テスト (Phase 4)
- Web UIからのフルサイクル動作確認 (Phase 5)
