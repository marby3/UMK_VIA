# UMK_VIA 開発ルール

このリポジトリでの作業は、人も AI もこのルールに従います。
何を作るかは [仕様書 (`docs/specification.md`)](docs/specification.md)、
今どこまでできているかは [README の「開発状況」](README.md#開発状況) に書いてあります。

---

## ブランチ規則

```text
main          リリースだけが入る。タグはここにだけ付く
 └─ dev       開発の本線。GitHub のデフォルトブランチ
     ├─ feat/N    新機能        (Issue #N)
     ├─ fix/N     不具合の修正  (Issue #N)
     └─ chore/N   ビルド・ツール・文書・リファクタ (Issue #N)
```

この 3 層のほかに使うブランチは、保存専用の `archive/*` と試作用の `spike/*` だけです (6・7 を参照)。

1. **作業は Issue から始めます。** Issue にはラベルを必ず 2 つ付けます。種類を表す
   `type:feat` / `type:fix` / `type:chore` / `type:docs` から 1 つ、優先度を表す
   `priority: high` / `priority: medium` / `priority: low` から 1 つです。
2. **作業ブランチは `dev` から切ります。** 名前は `feat/12` のように Issue 番号を付け、
   `#` は入れません (PowerShell では `#` 以降がコメントになるため)。ブランチの種類は
   Issue の種類に合わせます。`type:feat` なら `feat/N`、`type:fix` なら `fix/N`、
   `type:chore` と `type:docs` なら `chore/N` です。
   Issue との紐付けは、コミット本文の `Refs: #12` で行います。

   ```bash
   git fetch origin
   git switch -c feat/12 origin/dev
   ```

3. **PR の base は `dev` です。** 作業ブランチから `dev` へは **squash merge** で取り込みます。
4. **`dev` から `main` へはリリースのときだけ**、リリース PR を **マージコミット** で取り込みます。
   squash しないのは、`dev` と `main` の祖先関係を保つためです。squash すると、
   リリースのたびに `main` を `dev` へ戻しマージしなければ、次のリリース PR が衝突します。
5. **`main` と `dev` には直接 push しません。** GitHub のルールセットで拒否されます。
6. **`archive/*` は保存専用で、どこにもマージしません。** `archive/pre-rewrite-wip` には、
   書き直し前の旧コード (UIAPduino_VIA) のうち未コミットだった変更が入っています。
7. **`spike/*` は試作用で、マージしません。** 本採用するときは Issue を立て、
   `feat/N` として作り直します。

コミットメッセージは Conventional Commits (`feat:` `fix:` `docs:` `chore:` `test:`) に従います。

### 機能を入れたら README を更新する

仕様書 §7 の項目を実装した PR や、実機で確認できた PR では、同じ PR の中で
README の「開発状況」表を更新します。開発が止まっても、表を見れば再開できるようにするためです。

---

## バージョン規則

[Semantic Versioning](https://semver.org/lang/ja/) に従い、`vMAJOR.MINOR.PATCH` の形で付けます。
番号は git のタグと GitHub Release だけで管理し、ソースコードには書きません。

- **タグと GitHub Release は `main` にだけ付けます。**
- **動作するまではタグを付けません。** 最初のリリースは `v0.1.0` で、条件は
  実機の uiapduino で次の 3 つを確認できたことです (「ある程度動く」の定義)。
  1. `1209:B803` のキーボードとして認識される
  2. `tools/via_probe.py --write` の全項目が通る
  3. Remap で変えたキーマップが、USB の挿し直し後も残っている
- **`0.x` の間**は、機能を追加したら MINOR (`0.1.0` → `0.2.0`)、修正だけなら
  PATCH (`0.1.0` → `0.1.1`) を上げます。
- **`v1.0.0`** は、仕様書 §7「実装対象」のファームウェアと Web UI がすべて実機で
  確認できたときに付けます。
- **1.0 以降に MAJOR を上げる変更**は、利用者に作業を求めるものです。
  - キーマップ保存形式 (`VKM1`) の非互換変更 (書き込み後にキーマップが初期化される)
  - `keyboards/<name>/config.h` の `CUSTOM_*` の非互換変更 (キーボード定義の書き換えが必要)
  - `umk` CLI の非互換変更 (コマンドやオプションの削除・意味の変更)

修正を急ぐ場合でも専用の hotfix ブランチは作らず、`fix/N` を `dev` に入れてから
PATCH リリースを出します。

### リリース手順

`v0.1.0` を例にします。

1. `dev` で README の「開発状況」表が実態と合っていることを確かめます。
2. `dev` から `main` へのリリース PR を作ります。タイトルは `chore(release): v0.1.0` とし、
   本文には前回のリリースから入った PR を並べます。

   ```bash
   gh pr create --base main --head dev --title "chore(release): v0.1.0"
   ```

3. PR を **マージコミット** でマージします。

   ```bash
   gh pr merge --merge
   ```

4. `main` のマージコミットにタグを付けて push します。ローカルの `main` は更新せず、
   fetch した `origin/main` (GitHub 上のマージコミット) に直接付けます。

   ```bash
   git fetch origin
   git tag -a v0.1.0 origin/main -m "v0.1.0"
   git push origin v0.1.0
   ```

5. タグの状態でビルドし、`.bin` を添付して GitHub Release を作ります。

   ```bash
   git switch --detach v0.1.0
   python tools/umk.py compile -kb uiapduino -km default
   cp build/main.bin umk_via-uiapduino-default-v0.1.0.bin
   gh release create v0.1.0 umk_via-uiapduino-default-v0.1.0.bin --verify-tag --generate-notes
   ```
