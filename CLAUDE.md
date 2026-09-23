# CLAUDE.md

@CONTRIBUTING.md

## このリポジトリで Claude Code が守ること

上の CONTRIBUTING.md に加えて、次のことを守ります。

- **ベースブランチは `dev`。** ブランチは `origin/dev` から切り、PR の base も `dev` にします。
  ai-dev-workflow の postmerge には必ず `--base dev` を渡します。
- **`dev` → `main` のリリース PR だけはマージコミットで取り込みます。** グローバル規則の
  「Squash merge only」に対する、このリポジトリだけの例外です (2026-09-23 に合意)。
- **仕様の根拠は `docs/specification.md` だけです。** 仕様と食い違う実装をする場合は、
  先に仕様書を直す Issue を立てます。
- **自動テストはありません。** ビルドの確認は `python tools/umk.py compile -kb uiapduino`、
  実機の確認は README §1 の手順と `tools/via_probe.py` で行います。
- **重い読み込み・受け入れ基準の点検・PR レビューはサブ AI に回します。** `.review/config`
  の OpenCode Go (`opencode-go/muse-spark-1.3-contributor`) を ai-dev-workflow の
  review スクリプト (`-Task research` / `criteria` / `review`) で呼び出します。
  Claude が自分で読むのは、サブ AI の報告を確かめる範囲にとどめます。
- **`firmware/lib/` は vendored です。** 編集しません。
