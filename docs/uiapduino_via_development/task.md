# UIAPduino WebHID カスタムキーボード開発タスクリスト

- `[x]` **Phase 1: USB通信基盤の構築**
  - `[x]` プロジェクトの `git init` とサブモジュールの追加 (`ch32v003fun`, `rv003usb`)
  - `[x]` `firmware/usb_config.h` の作成（複合デバイスディスクリプタ定義）
  - `[x]` `firmware/usb_config.c` の作成（ディスクリプタ配列の実装）
  - `[x]` `firmware/main.c` の作成（システム・USBの初期化）
  - `[x]` `firmware/Makefile` の作成（submoduleを参照するビルドスクリプト）
  - `[x]` ビルド（コンパイル）のテスト

- `[x]` **Phase 2: キーボード機能の実装**
  - `[x]` `firmware/matrix.h` / `matrix.c` の作成（汎用的なGPIO構成、ピン定義マクロ）
  - `[x]` スキャンロジック、デバウンス処理の実装
  - `[x]` `firmware/keymap.h` / `keymap.c` の作成（RAM上の二次元/三次元配列管理）
  - `[x]` `main.c` にキースキャンおよび `Endpoint 1` (IN) への送信処理を統合

- `[x]` **Phase 3: WebHID通信とキーマップ動的変更機能**
  - `[x]` `main.c` に `Endpoint 2` (OUT) の受信コールバック実装
  - `[x]` `command_id = 0x01` 8バイトパケットによる `current_keymap` RAM書き換えロジック実装

- `[x]` **Phase 4: Flashメモリへの保存機能**
  - `[x]` `firmware/flash_store.h` / `flash_store.c` の作成
  - `[x]` Flash最終ページ(群)の消去と `current_keymap` 配列の書き込み処理実装
  - `[x]` `main.c` と連携し、`command_id = 0x99` 受信時にUSB無効化、Flash書き込み、システムリセットを呼ぶ処理
  - `[x]` 起動時(`Flash`から`RAM`)へのデータロードロジック実装

- `[x]` **Phase 5: Webフロントエンド開発 (機能ファースト)**
  - `[x]` プロジェクト構成作成 (`web/index.html`, `web/hid_app.js`, `web/protocol.js`, `web/styles.css`)
  - `[x]` WebHID API (navigator.hid) でデバイスに接続するUI(ボタン)の実装
  - `[x]` WebLink_USB互換の仕様4項（8バイトプロトコル）に準拠した送受信関数の実装
  - `[x]` 基本UIの実装（キーマップ変更、表示）
  - `[x]` 機能テスト・UIリファイン（保存時にUSBが一時途絶し、即座に再接続される挙動のハンドリング）
  - `[x]` デザインの適用（Vanilla CSSによるモダン・グラスモーフィズムデザインの統合）
