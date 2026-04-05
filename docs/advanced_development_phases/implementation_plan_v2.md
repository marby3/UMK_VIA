# 画面統合とRotation/マトリクス・レイアウト定義の実装案

ご要望を受け、フェーズ3（レイアウトエディタ）とフェーズ4（ファームウェアビルダー）を統合し、`KeyboardDefinition` に準拠した包括的なデータ形式での入出力に対応するためのアーキテクチャ変更を計画しました。実装に入る前に方針の確認をお願いいたします。

## User Review Required

> [!IMPORTANT]
> 以下のアーキテクチャ統合とJSONスキーマの実装方針で問題ないかご確認ください。

### 1. 画面の統合方針（TabUI化）
ヘッダーから「レイアウト」ナビゲーションを削除し、「開発ユーザー（Firmware Builder）」の中に以下のサブタブを作成します。
1. **Hardware Config**: VID/PID、Row/Col数、各種ピン設定を行う画面。（現在のフェーズ4）
2. **Layout Editor**: キャンバス上でキーを配置する画面。（現在のフェーズ3）

これにより、同じコンテキスト（同一のRow/Col定義など）を共有しながら両画面を行き来できるようにします。

### 2. Layout Editorへの Rotation & Row/Col 追加
- **Position & Rotationパネル**:
  キャンバス脇に、画像のKLE NGを模した `Position (X, Y)`, `Size (Width, Height)`, `Rotation (degrees, Rx, Ry)`, `Matrix (Row, Col)` を入力できる共通プロパティパネルを配置します。
- **CSS TransformによるRotation**:
  KLEの仕様に基づき、`(Rx, Ry)` を `transform-origin` に指定し、`rotate(r deg)` を適用することで回転をブラウザ上で正確に再現します。
- **デフォルトラベルの削除**:
  `Add Key` 時の初期テキスト（`0,0`等）は表示せず、空欄にします。

### 3. JSON定義ファイル (KeyboardDefinition) の入出力化
個別のKLE配列のみをダウンロードするのではなく、以下のようにハードウェア設定とKLE配列を融合させた**統合JSONファイル**のエクスポートおよびインポート（パース）機能を実装します。

```json
{
  "name": "Custom Keyboard",
  "vendorId": "0x1209",
  "productId": "0xb803",
  "matrix": { "rows": 4, "cols": 6 },
  "layouts": {
    "keymap": [
      [
        { "x": 1, "r": 30, "rx": 6.5, "ry": 4.25, "row": 0, "col": 0 },
        "Esc",
        { "row": 0, "col": 1 },
        "F1"
      ]
    ]
  }
}
```
※キーごとに行(`row`)・列(`col`)のメタデータをプロパティオブジェクト内に付与して出力します。これにより、アップロード時にレイアウトとピンマトリクスのマッピングが完全に復元可能になります。

## 今後の作業手順
1. `index.html` および `styles.css` を更新し、タブ構造とヘッダーの整理を行う。
2. `firmware_builder.js` と `layout_editor.js` のスコープを連携させ（あるいは一つの `builder_app.js` に統合し）、グローバルな統合データを扱えるようにする。
3. RotationのCSSレンダリングロジックとプロパティ画面の実装。
4. 統合JSONのダウンロード／アップロードパースロジックの実装。

上記の方針で開発（コードの書き換え）を進行してよろしいでしょうか？
