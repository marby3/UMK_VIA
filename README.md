# UMK_VIA — ファームウェア

CH32V003 (RISC-V / Flash 16KB / RAM 2KB) 上で動作する自作キーボード
ファームウェアです。キーマップ編集は VIA プロトコル経由で
[Remap](https://remap-keys.app) に委任します。

設計の根拠はすべて [仕様書 (`docs/specification.md`)](docs/specification.md) にあります。
本文中の「仕様書 §N」はこの文書の節番号です。開発ルール (ブランチ運用・バージョン規則) は
[CONTRIBUTING.md](CONTRIBUTING.md) を参照してください。

---

## 開発状況

2026-09-23 時点。ファームウェアとビルドシステムは書き終えていてビルドも通りますが、
**実機ではまだ一度も動かしていません**。Web UI と `build_server.py` は未着手です。
まだリリース (バージョン番号) はありません。

仕様書 §7「実装対象」の各項目:

| 項目 | 実装 | 実機確認 |
| --- | --- | --- |
| マトリクス / ダイレクトピン | 実装済み | 未確認 |
| レイヤー (`MO` / `TO` / `TG` / `DF`) | 実装済み | 未確認 |
| Mod-Tap / Layer-Tap | 実装済み | 未確認 |
| VIA 連携 (プロトコル `0x000C`) | 実装済み | 未確認 |
| Flash 永続化 | 実装済み | 未確認 |
| Split | 実装済み | 未確認 |
| RGB (4 モード) | 実装済み | 未確認 |
| `umk` CLI | 実装済み | 未確認 (`flash` 以外は実機不要で、PC 上で動作確認済み) |
| Web UI: ヘッダーナビゲーション | 未着手 | — |
| Web UI: Hardware Builder | 未着手 | — |
| Web UI: Layout Editor (KeyboardDefinition 入出力) | 未着手 | — |
| Web UI: ローカルビルド API 連携 (`build_server.py`) | 未着手 | — |
| Web UI: Web Flasher | 未着手 | — |
| Web UI: Key Tester (104 / 109 + カスタムレイアウト) | 未着手 | — |

次の一歩は、下の §1 の手順を実機の `ch32v003_keyboard` で最後まで通すことです。通れば
`v0.1.0` を付けます (条件は CONTRIBUTING.md のバージョン規則)。

### キーボード定義

| 定義 | 基板 | 備考 |
| --- | --- | --- |
| `keyboards/ch32v003_keyboard` | 自作の 6 キー (2 行 × 3 列)。UIAPduino Pro Micro CH32V003 V1.4 を搭載 | 実機確認に使う基板。LED (SK6812MINI-E × 6) は PD5 につながっていて、今の WS2812B ドライバ (PC6 固定) では**点灯しない** (§4 の RGB ピンを参照) |
| `keyboards/uiapduino` | なし (4 × 6 のサンプル定義) | 全 `CUSTOM_*` の見本。対応する実物の基板はない |

書き込み用の `.bin` と定義 JSON は、`dist/<キーボード名>-<コミット>/` にセットで置きます
(git 管理外)。

---

## 1. 最短の動作確認手順

UIAPduino のように **rv003usb の USB ブートローダーが載っている基板なら、
書き込みは USB だけで完結します** (プログラマ不要)。ブートローダーは
CH32V003 の専用 BOOT 領域 (1916 バイト) にあり、16KB のユーザー Flash とは
別なので、**キーマップ保存領域とは衝突しません**。

ブートローダーが載っていない素の CH32V003 に初めて書き込む場合だけ、
SWD プログラマ (WCH-LinkE 等: SWIO→PD1 / GND / 3V3) が必要です。
本ファームウェアは PD1 を GPIO に転用しない (`AFIO_PCFR1_SWCFG` を触らない) ので、
書き込み後も SWD は生きたままです。

### 1-1. 配線 (USB)

rv003usb の要件:

| 信号 | ピン | 備考 |
| --- | --- | --- |
| D+ | PD3 | `CUSTOM_USB_PIN_DP` |
| D- | PD4 | `CUSTOM_USB_PIN_DM` |

- **D- と 3V3 の間に 1.5kΩ のプルアップが必須**です。これが Low-Speed
  デバイスの存在を示す信号なので、無いと一切列挙されません。
  GPIO で制御したい場合は `CUSTOM_USB_PIN_DPU` を使います。
- D+/D- に直列 33〜47Ω を入れるのは任意 (ノイズ対策)。
- VDD は 3.3V。USB 動作時は 3.6V を超えないこと。
- D+/D- に使えるのは **ピン番号 0〜4 のみ**です (rv003usb が `c.andi` の
  5bit 即値にピン番号を埋め込むため)。PC5-PC7 / PD5-PD7 は使えません。
  違反すると `usb_config.h` がビルド時に `#error` を出します。

### 1-2. ビルドと書き込み

基板をブートローダーモードにしてから (多くの構成では挿し直し ──
ユーザーコード起動まで約 5 秒待ちます ── か、ブートボタンを押しながら挿す)、

```bash
python tools/umk.py flash -kb ch32v003_keyboard
```

`minichlink` は USB ブートローダー (VID `0x1209` / PID `0xB003`) と WCH-LinkE の
どちらも自動検出します。取り違える場合は明示できます。

```bash
python tools/umk.py flash -kb ch32v003_keyboard --programmer b003boot   # USB
python tools/umk.py flash -kb ch32v003_keyboard --programmer linke      # WCH-LinkE
```

`minichlink.exe` はビルド済みのものが vendored されているので追加ビルドは不要です。
何が見えているかの確認は:

```bash
firmware/lib/ch32v003fun/minichlink/minichlink -i
```

別のツールで書く場合は `build/main.bin` を **0x08000000** に書き込んでください
(ブートローダー領域ではなくユーザー Flash 側です)。

> **再列挙について。** ブートローダーはソフトリセットでユーザーコードに移るため、
> そのままではホストがブートローダー側のデバイス (`1209:B003`) を掴んだままになり、
> キーボードとして現れません。起動直後に D- を 10ms Low に落として切断を
> 認識させています (`usb_force_reenumerate()`)。`CUSTOM_USB_PIN_DPU` を使う構成では
> rv003usb 側がプルアップを制御するので、そちらでも成立します。

### 1-3. 列挙の確認

書き込み後にユーザーコードが走ると、HID デバイスが 2 つ
(Boot Keyboard と Raw HID) 見えます。Windows なら:

```powershell
Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -like "*VID_1209&PID_B803*" }
```

- `B803` が見えれば正常です。
- `B003` のまま変わらない場合はブートローダーがユーザーコードに移っていません
  (タイムアウト待ちか、ブートボタンが押されたままか、書き込みが未完了)。
- 何も出ない場合はファームウェアではなく配線、特に **D- の 1.5k プルアップ**を
  疑ってください。これが無いと一切列挙されません。

### 1-4. VIA の疎通確認 — Remap を開く前に

Remap は不正なレポートを受け取ると**黙って切断する**ため、先にこちらで
プロトコルを直接確認してください。失敗箇所がそのまま出ます。

```bash
pip install hidapi
python tools/via_probe.py --keyboard ch32v003_keyboard
```

キーマップ書き込みと即時 Flash 保存まで含めて試す場合 (元の値は書き戻します):

```bash
python tools/via_probe.py --keyboard ch32v003_keyboard --write
```

確認内容:

- Usage Page `0xFF60` / Usage `0x61` のインターフェースが見えるか
  (Remap が認識する条件そのもの)
- `get_protocol_version` が `0x000C` を返すか
  → **32 バイトレポートの 8 バイト×4 分割・再組み立てが動いている証拠**
- uptime が進むか、1ms ループのレートが正常か
- `switch_matrix_state` (Remap の Test Matrix が使う)
- マクロが個数 0 を返すか (Remap 側でマクロ UI が無効化される条件)
- エンコーダコマンドが `id_unhandled` を返すか
- `get_buffer` と `get_keycode` の内容が一致するか
- `--write` 時: キーコード書き込み → 読み戻し → 復元 → 即時保存後も応答するか

### 1-5. Remap で開く

1. `keyboards/ch32v003_keyboard/ch32v003_keyboard.remap.json` を Remap に登録する
   (自分のアカウントでキーボード定義として登録するか、
   ローカル起動した remap-keys/remap に読み込ませる)。登録時の製品名は
   `config.h` の `STR_PRODUCT` (`CH32V003_keyboard`) と同じにする。VID/PID が
   `uiapduino` と共通なので、Remap は製品名で定義を区別する
2. 定義の `vendorId` / `productId` が `config.h` の `CUSTOM_VID` / `CUSTOM_PID`
   と一致していること
3. 定義の `matrix.rows` / `matrix.cols` は **Split 結合後の論理サイズ**であること
4. キーの左上レジェンド `"row,col"` がそのままマトリクス位置として解釈される

キーマップを変更すると VIA 通信が 750ms 途切れた時点で自動的に Flash へ
保存されます (保存後に再起動はしません)。

> **Flash 保存中は USB が数 ms 止まります。** ソフトウェア USB なので消去/書き込み
> 中の CPU ストールを避けられません。1 キー変更なら 64 バイトページ 1 枚しか
> 書き換わらないよう差分比較しているので、実質 1 ページ分 (数 ms) です。

---

## 2. ディレクトリ構成

```text
docs/
  specification.md         仕様書 (唯一の設計文書)
firmware/
  Makefile
  funconfig.h              ch32fun/rv003usb 向けビルド設定 (両者が名前で探すので直下)
  core/                    キーボード論理
    board_config.h         CUSTOM_* → MATRIX_*/LOGICAL_*、matrix_row_t、timer_ms
    main.c                 起動シーケンスと 1ms メインループ
    keymap.c/.h            キーコード評価・レイヤー・Tap/Hold
    via.c/.h               VIA プロトコル
    flash_store.c/.h       内蔵 Flash への永続化
  drivers/                 GPIO / USART / SPI / DMA を触る層
    matrix.c/.h            GPIO スキャン
    usb_config.c/.h        USB ディスクリプタと rv003usb コールバック
    split.c/.h             USART1 + DMA
    rgb_led.c/.h           SPI + DMA (WS2812B)
  lib/                     vendored (編集しない)
    ch32v003fun/  rv003usb/
keyboards/                 キーボード固有設定 (構成は §4)
  ch32v003_keyboard/       実機確認に使う 6 キー基板
    config.h  rules.mk  ch32v003_keyboard.remap.json
    keymaps/default/keymap.c
  uiapduino/               4x6 のサンプル定義
    config.h  rules.mk  uiapduino.remap.json
    keymaps/default/keymap.c 既定キーマップ
    keymaps/gaming/keymap.c  追加キーマップの例 (-km gaming)
tools/
  umk.py                   umk CLI 本体
  flash_guard.py           リンク後の Flash 残量チェック
  stamp.py                 build/ がどのキーボード/キーマップのものかを記録
  via_probe.py             VIA 疎通確認
umk  umk.cmd               umk.py のラッパー (POSIX / Windows)
build/                     生成物すべて (gitignore)
dist/                      書き込み用 .bin と定義 JSON のセット (gitignore)
```

core はキーボード論理で、GPIO・USART・SPI・DMA には drivers 経由でしか触れません。
ただし例外が 2 つあります。`flash_store.c` は Flash コントローラを、`main.c` は
SysTick を直接操作します。

依存は **core → drivers → lib** が基本ですが、drivers から core への参照が 2 種類
(3 ファイル) あります。

- `matrix.h` / `rgb_led.h` が `core/board_config.h` を include して、マトリクスの寸法
  (`MATRIX_*` / `LOGICAL_*`) を読む。`board_config.h` を core 側に置いているのは、
  キーマップのサイズを知るために core がドライバのヘッダを include しなくて済むようにするため
- `usb_config.c` が `core/via.h` を include して、USB 割り込みから
  `via_receive_packet()` / `via_handle_in()` を呼ぶ

---

## 3. ビルド

必要なもの: `riscv-none-elf-gcc`、GNU Make、Python 3。

```bash
python tools/umk.py compile -kb uiapduino -km default
./umk compile -kb uiapduino              # ラッパー経由 (Windows は umk.cmd)
make -C firmware KEYBOARD=uiapduino KEYMAP=default main.bin
```

生成物はすべて `build/` に出ます (`firmware/` 以下には何も書きません)。

```bash
python tools/umk.py list                 # キーボードとキーマップの一覧
python tools/umk.py clean -kb uiapduino  # build/ を削除
```

### フラッシュ残量チェック

キーマップは内蔵 Flash 末尾に保存されますが、この領域はリンカスクリプトで
予約されていません。`.text` がここまで伸びると最初の保存でファームウェア本体が
壊れるため、リンク後に `tools/flash_guard.py` が毎回検証してビルドを止めます。

```text
Flash: 5856 / 16128 bytes used (256 B reserved for the keymap store at 0x08003F00)
```

RAM 超過は `current_keymap` が `.bss` に載るためリンカが検出します
(`region 'RAM' overflowed`)。

---

## 4. 新しいキーボードを追加する

```
keyboards/<name>/
  config.h                  ピン・機能フラグ (CUSTOM_* マクロ)
  rules.mk                  ビルドフラグ
  keymaps/default/keymap.c  既定キーマップ
  <name>.remap.json         Remap 登録用の定義
```

`keyboards/uiapduino/config.h` に全 `CUSTOM_*` の一覧とコメントがあります。

### CH32V003 で使えないピン

| ピン | 用途 |
| --- | --- |
| PD3 / PD4 | USB D+/D- |
| PD1 | SWIO デバッグ |
| PD5 / PD6 | Split の USART1 Tx/Rx (`CUSTOM_SPLIT_ENABLE` 時、固定) |
| PC6 | WS2812B データ (`CUSTOM_RGB_ENABLE` 時、固定) |
| PD7 | NRST (オプションバイトで解放しない限り) |

標準構成でマトリクスに使えるのは PA1, PA2, PC0–PC5, PC7, PD0, PD2 です。
RGB を使わないなら PC6 も使えます (`ch32v003_keyboard` は列に使っています)。

### RGB ピンについて (仕様書 §8 の未確定事項)

採用した `ws2812b_dma_spi_led_driver.h` は DMA+SPI 駆動で、CH32V003 では
SPI1 MOSI = **PC6 固定**です。`CUSTOM_RGB_PIN` は設定可能なままにしてありますが、
PC6 以外を指定するとビルド時に `#error` になります (無音で光らない状態を避けるため)。
Web UI 側の RGB ピン選択肢も PC6 のみに絞ってください。

`ch32v003_keyboard` の LED はデータ線が PD5 なので、このドライバでは点灯しません。
そのため RGB を無効にしてビルドしています。PD5 で光らせるには、任意のピンを駆動できる
ドライバ (ソフトウェアのビット操作など) が必要で、これは §8 の決定と合わせて別の作業にします。

---

## 5. 実装済み / 未実装

**実装済み**

- マトリクススキャン / ダイレクトピンモード (キーごとの独立デバウンス)
- レイヤー: `MO` / `TO` / `TG` / `DF`、`KC_TRNS` フォールスルー、透過自動補完
- Mod-Tap / Layer-Tap (同時保留 1 キーの簡略実装)
- VIA プロトコル `0x000C`
- Flash 永続化 (`VKM1` ヘッダによる構成不一致ガード、自動保存 / 即時保存、
  ページ差分比較)
- Split (USART1 115200、Slave→Master 片方向、行/列いずれかの方向で結合)
- RGB LED 4 モード (Rainbow / Static / Breathing / Reactive)
- `umk` CLI (`compile` / `flash` / `clean` / `list`)

**スコープ外 (仕様書 §7)**

ロータリーエンコーダ (VIA `0x14`/`0x15` は `id_unhandled`)、マクロ /
One Shot / Tap Dance / Unicode、圧電スピーカー、ディスプレイ。

リソースの実測値 (2026-09-23、クリーンビルド時のリンカ出力):

| 構成 | Flash | RAM |
| --- | --- | --- |
| `uiapduino` 既定 (4x6、4 レイヤー、Split/RGB なし) | 5856 B / 16 KB | 512 B / 2 KB |
| Split + RGB 32 LED + 8 レイヤー + 論理 4x16 (片手 4x8) | 8588 B / 16 KB | 1732 B / 2 KB |
| Split + RGB 32 LED + 8 レイヤー + 論理 8x16 (片手 8x8) | — | **収まらない** (2964 B 必要、リンクエラー) |

既定構成なら、スコープ外の項目に回せる余地があります。論理 4x16・8 レイヤーまで
積むと RAM は 1732 B になり、スタックの余裕はほとんど残りません。論理 8x16・8 レイヤーは
キーマップだけで 8 × 128 × 2 = 2048 B となり、RAM の全量を超えます。キーマップ保存領域も
Flash 末尾から確保されるので、レイヤーやキー数を増やすと使えるプログラム領域が減ります
(論理 4x16・8 レイヤーで 1088 B を予約)。

---

## 6. サードパーティ

`firmware/lib/ch32v003fun/` ([cnlohr/ch32fun](https://github.com/cnlohr/ch32fun)) と
`firmware/lib/rv003usb/` ([cnlohr/rv003usb](https://github.com/cnlohr/rv003usb)) は
vendored です。原則編集しません。
