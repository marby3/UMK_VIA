# UMK_VIA 仕様書

> 本書は UMK_VIA の唯一の仕様書です。実装は本書に基づいて行います。

---

## 0. 設計方針サマリー

1. **Web UI はキーマップ編集機能を持たない。** キーマップ編集は [Remap (remap-keys.app)](https://remap-keys.app) に
   完全委任する。Web UI の役割は「ハードウェア/キーボードビルダー」「Web Flasher」「Key Tester」
   「Remap用キーボード定義の生成」に限定する。→ [5. Web UI 仕様](#5-web-ui-仕様)
2. **RGB LED のピンは `CUSTOM_RGB_PIN` で指定可能にする。** → [4.8 RGB LED](#48-rgb-led)
3. **レイアウトエディタのキャンバス描画は、スケール定数を一箇所に定義して全描画パスで共有する。**
   → [5.2 Hardware / Keyboard Builder](#52-hardware--keyboard-builder)
4. **実装対象の機能一覧とスコープ外の項目は [7. 実装スコープ](#7-実装スコープ) に一本化する。**
5. **プロジェクト名は `UMK_VIA` とする。** サンプルの個別キーボード定義名 `keyboards/uiapduino/` は
   ハードウェア個体名としてそのまま使う (プロジェクト名ではなく「そのキーボード」の名前という位置づけ)。
6. **ビルドは独自 CLI `umk` でも実行できるようにする。** QMK のコマンド書式 (`compile`/`-kb`/`-km`) を
   模倣するが、実体は本プロジェクト独自のツールとする。→ [4.9 ビルドシステム](#49-ビルドシステム)

---

## 1. プロジェクト概要

UMK_VIA は、WCH **CH32V003** RISC-V マイコン上で動作する自作キーボード向けファームウェアと、
それをブラウザだけで書き込み・ビルド・検証できる Web コンフィグレータのセットです。

- **対象ユーザー**: 自作キーボードを組む一般ユーザー (書き込み・キーマップ変更のみ) と、
  基板/ピン配置から独自キーボードを定義する開発ユーザーの両方。
- **キーマップ編集の入口**: Remap (VIA プロトコル経由)。本プロジェクトの Web UI では編集させない。
- **配布形態**: OSS。ビルド・書き込み・定義生成はすべてブラウザ (+ ローカル Python サーバー) で完結する。

---

## 2. ハードウェア・ソフトウェア基盤

| 項目 | 内容 |
| --- | --- |
| マイコン | WCH **CH32V003** (RISC-V, Flash 16KB, RAM 2KB) |
| コアフレームワーク | `ch32v003fun` (`firmware/lib/` に vendored) |
| USB 実装 | `rv003usb` (ソフトウェア USB, `firmware/lib/` に vendored) |
| USB 速度 | **Low-Speed** — 1 パケット最大 8 バイト、`bInterval` 下限 10ms (規格上の制約) |
| ビルドツールチェイン | `riscv-none-elf-gcc` |
| 永続化 | EEPROM 非搭載。内蔵 Flash の末尾ページを使用 |

`ch32v003fun` と `rv003usb` はサードパーティとして扱い、原則編集しない。

---

## 3. リポジトリ構成

QMK 風に「コアファームウェア」と「キーボード固有設定」を分離した構成とします。

```text
firmware/                  コアファームウェア (キーボード非依存)
  Makefile                 KEYBOARD=<name> を受け取るビルドエントリ
  funconfig.h              ch32v003fun / rv003usb 向けビルド設定
  core/                    キーボード論理
    board_config.h         CUSTOM_* から MATRIX_* / LOGICAL_* を導出
    main.c                 メインループ・USB割り込みディスパッチ
    keymap.c/.h            QMK互換キーコード評価・レイヤー・Tap/Hold
    via.c/.h               VIAプロトコル実装
    flash_store.c/.h       内蔵Flashへのキーマップ永続化
  drivers/                 ペリフェラル (GPIO / USART / SPI / DMA) を触る層
    usb_config.c/.h        USBディスクリプタ (EP0/EP1/EP2)
    matrix.c/.h            マトリクス / ダイレクトピンスキャン
    split.c/.h             分割キーボード (USART1)
    rgb_led.c/.h           RGB LED (WS2812, DMA+SPI)
  lib/                     vendored (編集しない)
    ch32v003fun/  rv003usb/

keyboards/<name>/          キーボード固有設定 (複数キーボードを共存可能にする単位)
  config.h                 ピン/マトリクス/機能フラグ (CUSTOM_* マクロ)
  rules.mk                 キーボード固有のビルド設定
  keymaps/
    default/keymap.c       既定キーマップ (default_keymap のオーバーライド。Web UI ビルドはこれを使う)
    <keymap名>/keymap.c    任意追加のキーマップ (umk CLI の `-km` で選択)
  <name>.remap.json        Remap 登録用キーボード定義

web/                        ブラウザ側コンフィグレータ (ビルド不要, Vanilla JS) ※未着手
  index.html                 タブ切り替え型の単一ページ
  builder_app.js             Hardware Config + Layout Editor
  flasher_app.js              WebUSB ファームウェア書き込み
  tester_app.js                動作テスター (WebHID不要、標準キーボードイベント)
  protocol.js                (Flasher/Builder が VIA 経由でデバイス情報を読む用途にのみ縮小して残す)
  styles.css

build_server.py             ローカル開発サーバー + /api/build, /api/remap-definition ※未着手
```

`web/` と `build_server.py` は予定している配置で、まだ存在しません。実装の進み具合は
README の「開発状況」を参照してください。

`keyboards/<name>/config.h` と `<name>.remap.json` は `build_server.py` が Web UI からの
JSON 設定を元に自動生成する成果物であり、手編集しても次回ビルドで上書きされる。

---

## 4. ファームウェア仕様

### 4.1 全体制御フロー

`main.c` が全体を統括する。起動シーケンスと 1ms メインループは以下の通り。

**起動時**
1. `SystemInit()` → 1ms ディレイ (USB 再認識用)
2. `flash_store_load()` — 保存済みキーマップの復元 (無ければ `default_keymap` で初期化)
3. `matrix_init()` — GPIO 初期化
4. `keymap_init()` — レイヤー状態・キャッシュ初期化
5. `via_init()`
6. `usb_setup()`
7. (`CUSTOM_RGB_ENABLE` 時) `rgb_led_init()`
8. (`CUSTOM_SPLIT_ENABLE` 時) `split_init()` → 最大 500ms USB 接続確立を待ち、確立すれば Master、
   タイムアウトすれば Slave として役割を確定 (`split_set_master()`)

**メインループ (1ms ごと)**
- Slave の場合: `matrix_scan()` → `split_slave_task()` のみを実行して継続 (VIA/Flash/RGB は実行しない)
- Master (または非 Split) の場合:
  1. `via_task()` — 受信済み VIA コマンドの処理
  2. `flash_store_task()` — 自動保存タイミングの判定
  3. (RGB 有効時) `rgb_led_task()`
  4. `matrix_scan()` でローカルマトリクスを更新
  5. Split 有効時は `split_master_task()` で Slave データと結合して `global_matrix_state` を構築、
     非 Split 時は `local_matrix_state` をそのまま `global_matrix_state` にコピー
  6. `global_matrix_state` の差分を検出し、押下/離上を `keymap_process_press/release()` に通知
  7. `keymap_task()` で時間経過依存の処理 (Tap/Hold のタップ解除) を実行
  8. 何か変化があれば `keymap_generate_report()` で USB レポートを再構築

### 4.2 USB 構成

3 エンドポイント構成、`ENDPOINTS = 3`。

| EP | 用途 | サイズ | bInterval | 備考 |
| --- | --- | --- | --- | --- |
| EP0 | Control | 8 bytes | - | 標準ディスクリプタ要求など |
| EP1 (IN) | Boot Keyboard HID | 8 bytes | 10ms | 6KRO。Modifier byte + Reserved + 6 keycode |
| EP2 (IN/OUT) | VIA Raw HID | 32 bytes (論理) / 8 bytes (物理パケット) | 10ms | Usage Page `0xFF60` / Usage `0x61` |

- Low-Speed 制約により物理パケットは 8 バイト固定。EP2 の 32 バイト論理レポートは
  8 バイト ×4 トランザクションとして `via.c` が組み立て・分割する。
- デフォルト VID/PID: `0x1209` / `0xB803` (`CUSTOM_VID` / `CUSTOM_PID` で上書き可能)。
- 文字列ディスクリプタ (`STR_MANUFACTURER` / `STR_PRODUCT` / `STR_SERIAL`) はキーボード側 `config.h` から
  上書き可能。デフォルトは `"UMK"` / `"UMK_VIA"` / `"001"`。
- EP2 の Usage Page/Usage は VIA/Remap 側の必須要件 (`WebHid.ts` が `usagePage===0xff60 && usage===0x61`
  のコレクションのみをキーボードとして認識する) であり変更不可。

### 4.3 マトリクススキャン

- `CUSTOM_MATRIX_ROWS` / `CUSTOM_MATRIX_COLS` で `MATRIX_ROWS` / `MATRIX_COLS` を定義 (既定 4x6)。
- **標準マトリクスモード**: 行を順に GND へ落とし (`OUT_10Mhz_PP`, `Delay_Us(10)` の安定待ち)、
  列をプルアップ入力 (`IN_PUPD`) で読む。デバウンスは行×列ごとに独立したタイマで管理 (既定 5ms)。
- **ダイレクトピンモード** (`CUSTOM_DIRECT_PIN_MODE`, マクロパッド向け): 行が存在せず、列ピン
  (`CUSTOM_COL_PINS`) を直接プルアップ入力として読む。内部的には `MATRIX_ROWS=1` として扱う。
- **論理マトリクス** (`LOGICAL_ROWS` / `LOGICAL_COLS`): Split 結合後、VIA/Remap から見えるサイズ。
  非 Split 時は `MATRIX_ROWS`/`COLS` と同一。Split 有効時は結合方向 (後述) に応じて行または列が 2 倍になる。

### 4.4 キーマップ / レイヤーエンジン

- **データ構造**: `current_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS]` (`uint16_t`)。
  `LAYERS` は既定 4、`CUSTOM_LAYERS` で変更可。RAM 使用量 `LAYERS × LOGICAL_ROWS × LOGICAL_COLS × 2`
  バイトに注意 (CH32V003 は RAM 2KB)。
- **キーコード体系 (QMK 互換, VIA が送る 16bit キーコードをそのまま解釈)**:

  | 範囲 | 意味 | 対応状況 |
  | --- | --- | --- |
  | `0x0000` | `KC_NO` (割り当てなし、確定して打ち切り) | ✅ |
  | `0x0001` | `KC_TRNS` (透過、下位レイヤーへフォールスルー) | ✅ |
  | `0x0004`-`0x00FF` | 基本キー (USB HID Usage ID と同値) | ✅ |
  | `0x0100`-`0x1FFF` | モディファイア付き基本キー (`LSFT(KC_A)` 等) | ✅ |
  | `0x2000`-`0x3FFF` | Mod-Tap (`MT`) | ✅ (同時保留1つまでの簡略実装) |
  | `0x4000`-`0x4FFF` | Layer-Tap (`LT`) | ✅ (同上) |
  | `0x5200`-`0x521F` | `TO(n)` | ✅ |
  | `0x5220`-`0x523F` | `MO(n)` | ✅ |
  | `0x5240`-`0x525F` | `DF(n)` | ✅ |
  | `0x5260`-`0x527F` | `TG(n)` | ✅ |
  | それ以外 | One Shot / Tap Dance / マクロ / Unicode 等 | ❌ スコープ外 (何も送出しない) |

- **評価順序**: 押下の瞬間、最上位アクティブレイヤーから順に評価し、`KC_TRNS` のときだけ下位へフォールス
  ルーする。`KC_NO` は「割り当てなし」として確定する (QMK 仕様通り)。
- **初期化時の透過補完**: `keymap_reset_to_default()` はレイヤー1以降の未割当 (`KC_NO`) を自動的に
  `KC_TRNS` で埋める。これをしないとレイヤー1以降が全キー無反応になる。`CUSTOM_NO_TRNS_DEFAULT` を
  定義すると明示的に `KC_NO` のままにできる。
- **キーコードキャッシュ**: 押下確定時のキーコードを `keycode_cache[row][col]` に記憶し、離上時は
  このキャッシュを見て解除処理する。キー押下中のレイヤー切り替えでも正しく解放できるための設計。
- **Tap/Hold**: 同時に保留できるのは1キーのみ。2つ目の Tap/Hold キーが押されると先行キーはホールド
  確定として扱う。`TAPPING_TERM_MS` (既定 200ms) 未満・割り込みなしでタップ確定、`TAP_REPORT_MS`
  (既定 20ms) の間だけホストへ送出する。
- **レポート生成**: `keycode_cache` の全マスを走査して HID Boot Keyboard レポート (Modifier byte +
  6 キー配列) を構築。6KRO を超える分は無視 (`key_idx < 8` の範囲でのみ格納)。

### 4.5 VIA プロトコル

`via.c` が **VIA プロトコルバージョン `0x000C`** を実装する。

| コマンドID | 内容 | 対応状況 |
| --- | --- | --- |
| `0x01` | `get_protocol_version` | ✅ `0x000C` |
| `0x02` | `get_keyboard_value` (uptime/layout options/switch matrix state/firmware version) | ✅ |
| `0x03` | `set_keyboard_value` (layout options/device indication/**独自拡張 0xFF=即時Flash保存**) | ✅ |
| `0x04`/`0x05` | `dynamic_keymap_get/set_keycode` | ✅ |
| `0x06`/`0x0A` | `dynamic_keymap_reset`/`eeprom_reset` | ✅ |
| `0x0C`-`0x10` | マクロ関連 | ⚠️ 個数0・バッファ0を返すのみ (Remap 上でマクロ UI が無効化される) |
| `0x11` | `dynamic_keymap_get_layer_count` | ✅ |
| `0x12`/`0x13` | `dynamic_keymap_get/set_buffer` (1回最大28バイト) | ✅ |
| `0x14`/`0x15` | エンコーダ | ❌ `id_unhandled` |
| `0x07`-`0x09` | ライティング (Custom UI) | ❌ `id_unhandled` |

**割り込み/メインループの分担 (rv003usb の制約)**

rv003usb は `usb_handle_user_data()` から**戻った直後**にホストへ ACK を返すため、割り込み内で重い処理
をすると Low-Speed のターンアラウンド時間 (数マイクロ秒) を守れず通信が壊れる。そのため:

| コンテキスト | 処理 |
| --- | --- |
| EP2 OUT 割り込み (`via_receive_packet`) | 8 バイトずつ `via_buf` へコピーし、32 バイト揃うか短いパケットを受けたらフラグを立てるだけ |
| メインループ (`via_task`) | コマンド解釈・応答組み立て・キーマップ初期化・Flash保存予約 |
| EP2 IN 割り込み (`via_handle_in`) | 用意済み応答を 8 バイトずつ送出。送信済みチャンク数は `e->count` (ホストACKごとに増分) から算出 |

**応答ルール**: Remap は「要求していないレポートを受け取ったら切断する」実装 (`WebHid.ts` の
`handleInputReport`) のため、`via_handle_in()` は応答待ちが無いとき必ず**空パケット**を返す。
また VIA はコマンドバッファをその場で書き換えて返す「エコーバック」方式のため、送受信で同じ
`via_buf` を使い回す。

**switch_matrix_state**: `LOGICAL_COLS` に応じて 1 行あたり 1/2/4 バイト (`VIA_MATRIX_ROW_BYTES`) で
`global_matrix_state` をビットパックして返す。Remap の Test Matrix 機能で使用。

**dynamic_keymap バッファアクセス**: `current_keymap` はメモリ上リトルエンディアンの `uint16_t` 配列
だが、VIA のバッファ表現はキーコードをビッグエンディアンで並べたバイト列のため、バイト単位アクセス時に
上位/下位を入れ替える。

### 4.6 Flash 永続化

- 内蔵 Flash (16KB, `0x08000000`-`0x08003FFF`) の**末尾ページ**を保存領域として使用。
  `FLASH_OFFSET = 0x08004000 - (FLASH_PAGES * 64)`。ファームウェア本体 (`.text`) が大きくなると
  この領域と衝突するため、リンク後のサイズに注意する。
- **保存フォーマット**: `magic("VKM1", 4B)` + `layers(1B)` + `logical_rows(1B)` + `logical_cols(1B)` +
  `reserved(1B)` + キーマップ本体 (`uint16` リトルエンディアン配列)。
- **起動時ロード**: magic とレイヤー/行/列数が一致した場合のみ復元。不一致 (ビルド設定変更後など) の
  場合は `default_keymap` で初期化する — 古い構成のデータを誤って読み込むことを防ぐガード。
- **自動保存**: VIA でキーマップが変更されると dirty フラグが立ち、USB 通信が `FLASH_AUTOSAVE_DELAY_MS`
  (既定 750ms) 途切れた時点でメインループが保存する。**保存後は再起動しない** (Remap との接続維持のため)。
- **即時保存**: VIA `set_keyboard_value` サブID `0xFF` (本プロジェクト独自拡張) で `flash_store_request_save()`
  を呼び、次の `flash_store_task()` で即座に保存する。
- **書き込み単位**: 64 バイトページ単位。消去・書き込み中はコアがストールしてソフトウェア USB の割り込み
  が遅延するため、ページごとに `__enable_irq()` を挟んで影響を最小化する。

### 4.7 分割キーボード (Split)

`CUSTOM_SPLIT_ENABLE` で有効化。

- **役割判定**: 起動時、USB ホストへの接続 (`usb_configured_flag`) が 500ms 以内に確立すれば **Master**、
  タイムアウトすれば **Slave**。判定は `main.c` が行い `split_set_master()` に反映する。
- **左右判定**: `CUSTOM_HANDEDNESS_PIN` をプルアップ入力にし、GND に落ちていれば左手側 (`is_left_hand=1`)。
  ピン未定義時は安全側フォールバックとして常に左 (Master=左, Slave=右という前提の配線を想定)。
- **通信**: USART1、Tx=`PD5` / Rx=`PD6` 固定、**115200bps**、Slave→Master の一方向送信のみ。
- **プロトコル**: 同期バイト `0xAA 0x55` に続けて、行ごとに列状態を 8bit 単位でビットパックしたバイト列
  (`ceil(MATRIX_COLS/8)` バイト/行) を送る。Slave 側はマトリクスに変化があった時、または前回送信から
  20ms 経過した時に送信する。Master 側は 2 状態の簡易ステートマシンで同期を取り、1 フレーム分
  (全行) 受信し終えると `received_slave_state` に格納する。
- **結合**: `local_matrix` (Master 自身のマトリクス) と `received_slave_state` (Slave から受信した
  マトリクス) を、`CUSTOM_SPLIT_COMBINE_COLS` (列方向に結合、`LOGICAL_COLS = MATRIX_COLS*2`) または
  `CUSTOM_SPLIT_COMBINE_ROWS` (行方向、`LOGICAL_ROWS = MATRIX_ROWS*2`) の設定に従って `global_matrix_state`
  へマージする。`is_left_hand` により、自分がオフセット無し側かオフセット側かを決める。

### 4.8 RGB LED

`CUSTOM_RGB_ENABLE` で有効化。WS2812B を DMA+SPI 駆動 (`ch32v003fun/extralibs/ws2812b_dma_spi_led_driver.h`)。

- **ピン**: `CUSTOM_RGB_PIN` で指定する。CH32V003 の WS2812B ドライバ (DMA+SPI 駆動) が SPI MOSI 固定
  ピンでしか出力できない場合は、Web UI 側の RGB ピン選択肢をそのピンのみに制限し、矛盾する組み合わせを
  生成させない (→ [8. 未確定事項](#8-未確定事項))。
- **LED 数**: `CUSTOM_RGB_NUM_LEDS` (ドライバ側バッファ上限 `DMALEDS=32`)。
- **モード** (`CUSTOM_RGB_MODE`、`WS2812BLEDCallback()` 内で毎フレーム色を計算):
  | モード値 | 名称 | 挙動 |
  | --- | --- | --- |
  | 0 | Rainbow | 全 LED を HSV で走査、10ms ごとに色相 +1、LED ごとに位相をずらす |
  | 1 | Static | 固定色 (シアン系, HSV(128,255,60)) |
  | 2 | Breathing | 2000ms 周期の三角波で明度を 0↔100 に往復 |
  | 3 | Reactive | 打鍵時に輝度を最大 (150) にし、5ms ごとに 1 ずつ減衰 |
- **更新レート**: 約 33Hz (30ms ごとに `WS2812BDMAStart()`)。
- **打鍵通知**: `keymap_process_press` 相当のイベントから `rgb_led_notify_keypress(row, col)` を呼び、
  Reactive モードの輝度をリセットする (現行実装では row/col は未使用、キー位置非依存の全体反応)。

### 4.9 ビルドシステム

#### 4.9.1 Makefile (低レベルビルドエントリ)

```bash
make -C firmware KEYBOARD=<name> KEYMAP=<keymap> main.bin   # ビルドのみ
make -C firmware KEYBOARD=<name> KEYMAP=<keymap>             # デフォルトターゲットは cv_flash も試みる (要 WCH-LinkE)
make -C firmware KEYBOARD=<name> KEYMAP=<keymap> clean
```

- `KEYBOARD` 未指定時は `$(error ...)` で即エラー。`KEYMAP` 未指定時は `default` を使う。
- `keyboards/$(KEYBOARD)/rules.mk` を `-include`、`EXTRA_CFLAGS` に `-I../keyboards/$(KEYBOARD)` を追加。
- `keyboards/$(KEYBOARD)/keymaps/$(KEYMAP)/keymap.c` が存在すれば `ADDITIONAL_C_FILES` に追加
  (`default_keymap` の `weak` オーバーライド)。存在しない場合はコア側の空 `weak` 定義のままビルドする。
- コアの `.c` ファイル一覧 (`rv003usb.S`, `rv003usb.c`, `usb_config.c`, `matrix.c`, `split.c`, `rgb_led.c`,
  `keymap.c`, `flash_store.c`, `via.c`) は固定でリンクする。

#### 4.9.2 umk CLI

本物の QMK CLI (`pip install qmk`) はそのまま使わない。QMK Firmware 本体のビルド構造・キーボード
リポジトリ構成を前提にしており、CH32V003 非対応の本プロジェクトでは動作しない。代わりに、
**`compile`/`-kb`/`-km` といったコマンド書式・引数ルールだけを模倣した、`umk` という名前の独自 CLI**
をリポジトリ直下に自作する (例: `tools/umk.py`, Python 製。シェルスクリプトでラップして `./umk ...`
としてもよい)。

```bash
umk compile -kb <keyboard_name> -km <keymap_name>
```

- `-kb` は `keyboards/<keyboard_name>/` の存在を検証し、`-km` は `keyboards/<keyboard_name>/keymaps/<keymap_name>/`
  の存在を検証してから、内部で `make -C firmware KEYBOARD=<keyboard_name> KEYMAP=<keymap_name> main.bin`
  を実行する薄いラッパーとする (ビルドロジックの実体は Makefile 側に置き、CLI は引数検証と呼び出しに徹する)。
- `-km` 省略時は `default` を使う (Makefile の既定と揃える)。
- 将来的に `umk flash` (書き込みまで実行) や `umk clean` など、QMK の他のサブコマンドと同名のショートハンド
  を同じ CLI に追加できるよう、サブコマンドディスパッチ構造で設計する。

#### 4.9.3 Web UI からのビルド (build_server.py)

Web UI の Hardware Builder は複数キーマップの概念を持たない (キーマップは常に VIA/Remap 経由で
動的に書き換える前提のため)。`build_server.py` は常に `keyboards/<name>/keymaps/default/` を対象に
`make -C firmware KEYBOARD=<name> KEYMAP=default main.bin` を実行する。→ [5.5](#55-build_serverpy-ローカル開発-api) 参照。

---

## 5. Web UI 仕様

### 5.1 役割定義

**キーマップ編集機能は持たない。** Web UI の役割は次の 4 つに限定する。

1. **Hardware / Keyboard Builder** — ピン配置・マトリクス/ダイレクトピン・Split・RGB・VID/PID などの
   ハードウェア定義と、視覚的なキー配置 (Layout Editor) を GUI で作成し、ファームウェアと
   Remap 用キーボード定義 JSON の両方を生成する。
2. **Web Flasher** — WebUSB で CH32V003 純正ブートローダーに直接书き込む (ドライバ不要)。
3. **Key Tester** — 完成したキーボードの打鍵動作を視覚的に確認する (WebHID 不要、標準の
   `keydown`/`keyup` イベントで動作するため市販キーボードのテストにも使える)。
4. キーマップの実際の編集・書き込みは **Remap (remap-keys.app) へ誘導する** (VID/PID とキーボード
   定義 JSON さえ揃っていれば Remap 側で完結する)。

### 5.2 Hardware / Keyboard Builder

- **Hardware Config タブ**: Row/Col ピン数に応じた動的なピン選択 UI、ダイレクトピンモード切替、
  Split 設定 (有効化/結合方向/Handedness ピン)、RGB 設定 (ピン・LED数・モード)、VID/PID 入力
  (プレースホルダ `0x1209`/`0xb803`)、修飾キー同時押し設定 UI。
- **Layout Editor タブ**: キャンバス上でのドラッグ&ドロップ配置、矩形選択/Ctrl+Click複数選択、
  Row/Col 自動連番補完、Rotation (角度 + 回転中心 Rx/Ry の可視化)。キャンバス描画のスケール定数は
  一箇所に定義し、全描画パスで共有する。
- **入出力データ形式**: `KeyboardDefinition` 統合 JSON (ハードウェア設定 + KLE 風レイアウト配列 +
  各キーの `row`/`col` メタデータ) でエクスポート/インポートし、往復変換で情報が失われないようにする。
- **ビルド実行**: `/api/build` に設定 JSON を POST し、`build_server.py` が `keyboards/<name>/config.h`
  を生成 → `make clean && make KEYMAP=default main.bin` → 生成された `.bin` をブラウザへ返す
  (常に `keyboards/<name>/keymaps/default/` を対象とする。複数キーマップの切り替えは
  [umk CLI](#492-umk-cli) の役割)。
- **Remap 用定義生成**: `/api/remap-definition` (サーバー生成、マトリクスそのままの格子レイアウト) と
  クライアント側 `generateRemapDefinition()` (Layout Editor の実際の配置を反映) の両方を提供する。

### 5.3 Web Flasher

- rv003usb-webflasher を参考に、CH32V003 のブートローダー ROM (USB HID feature report 経由) に対して
  halt → flash unlock → 64 バイト単位で write/verify → run app の手順で `.bin` を書き込む。
- 追加のドライバ・WCH-LinkE 等の書き込み機は不要。ブラウザの WebUSB のみで完結する。
- 書き込み成功後、Remap (または本体の Key Tester) へ誘導する導線を用意する (自動遷移はしない)。

### 5.4 Key Tester

- 標準 104 (US) / 109 (JIS) レイアウトを内蔵し、切り替えて使用可能。
- Layout Editor で作成したカスタムレイアウトを読み込んでの動作確認にも対応する。
- 押下中/押下済みキーの視覚フィードバックと、ミリ秒付きのイベントログ表示を行う。

### 5.5 build_server.py (ローカル開発 API)

- `POST /api/build`: Hardware Config の JSON を受け取り、`core/board_config.h` の `LOGICAL_ROWS`/`LOGICAL_COLS`
  計算ロジックと同じ規則 (`logical_matrix_size()`) で論理マトリクスサイズを算出し、
  `keyboards/<name>/config.h` と `<name>.remap.json` を同時生成してからビルドする。
  **`core/board_config.h` の論理サイズ計算ロジックを変更する場合は、この関数を必ず同期させる。**
- `POST /api/remap-definition`: ビルドを伴わない、定義 JSON のみの生成 (ダウンロード用)。
- VID/PID は Remap のスキーマ (`^0x[0-9a-fA-F]{1,4}$`) に正規化してから出力する。

### 5.6 Remap 連携

- Remap は VID/PID に紐づくキーボード定義 JSON をカタログまたは個人登録から読み込む方式のため、
  本プロジェクトで生成した定義 JSON を Remap 側 (自分のアカウントでの登録、またはローカル起動した
  remap-keys/remap) に読み込ませる必要がある。
- 定義 JSON の `matrix.rows`/`cols` は **Split 結合後の論理サイズ**でなければならない。
- キーの左上レジェンド `"row,col"` がそのままマトリクス位置として解釈される (VIA の慣習)。

---

## 6. 用語・定数早見表

| 定数/シンボル | 既定値 | 上書き方法 |
| --- | --- | --- |
| `MATRIX_ROWS`/`MATRIX_COLS` | 4 / 6 | `CUSTOM_MATRIX_ROWS`/`CUSTOM_MATRIX_COLS` |
| `LAYERS` | 4 | `CUSTOM_LAYERS` |
| `TAPPING_TERM_MS` | 200 | `config.h` で直接定義 |
| `TAP_REPORT_MS` | 20 | 同上 |
| `FLASH_AUTOSAVE_DELAY_MS` | 750 | 同上 |
| `CUSTOM_VID`/`CUSTOM_PID` | `0x1209`/`0xB803` | `config.h` |
| VIA プロトコルバージョン | `0x000C` | 固定 (Remap 側実装依存) |
| Split UART ボーレート | 115200 | 固定 |
| RGB 更新レート | 約33Hz (30ms) | 固定 |

---

## 7. 実装スコープ

### 実装対象

- ファームウェア: マトリクス/ダイレクトピン、レイヤー (MO/TO/TG/DF)、Mod-Tap/Layer-Tap、VIA連携、
  Flash永続化、Split、RGB (4モード)
- Web UI: ヘッダーナビゲーション、Hardware Builder、Layout Editor (KeyboardDefinition入出力)、
  ローカルビルドAPI連携、Web Flasher、Key Tester (104/109 + カスタムレイアウト)、umk CLI

### スコープ外

- ロータリーエンコーダー (VIAコマンド `0x14`/`0x15` は `id_unhandled` を返す)
- マクロ / One Shot / Tap Dance / Unicode 入力
- 圧電スピーカー、ディスプレイ表示
- Web UI 上でのキーマップ編集機能 (Remap に委任する方針のため、意図的にスコープ外とする)

---

## 8. 未確定事項

- RGB ピン: CH32V003 の WS2812B ドライバが SPI MOSI 固定ピンしか使えない制約のまま、Web UI 側の
  選択肢をそのピンのみに制限するか、ソフトウェア Bit-bang 等でピンを任意化するかは、採用するドライバの
  実装方式が決まってから確定する。
