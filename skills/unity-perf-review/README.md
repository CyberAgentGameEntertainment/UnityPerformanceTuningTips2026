# unity-perf-review

Unity プロジェクトをパフォーマンス観点で静的レビューする Agent Skills 準拠のスキルです。
『Unity Performance Tuning Bible』シリーズの知見を 163 の評価ルール（Critical / Warning / Info）
として収録し、実測値つきの根拠とルール ID を添えた指摘を出力します。

SKILL.md 形式に対応したエージェント（Claude Code、OpenAI Codex、GitHub Copilot、
Gemini CLI など）で動作します。

## 同梱物

- `SKILL.md` — エージェント向けの実行フロー
- `references/` — チェックリスト（一次スキャン用）、カテゴリ別ルール163件、
  診断ナレッジ、rules-overrides.example.md
- `scripts/detect-env.py` — 環境プロファイル（tuning-env.md）の生成
- `scripts/scan.py` — 一次スキャンの機械実行（ヒット一覧と Coverage を出力）
- `scripts/gen-checklists.py` — ルール修正時のチェックリスト再生成

## 導入（3ステップ）

1. このディレクトリごと、使っているエージェントのスキルディレクトリへコピーします。

   ```bash
   cp -r unity-perf-review /path/to/your-repo/.claude/skills/   # Claude Code（Copilot も読む）
   cp -r unity-perf-review /path/to/your-repo/.agents/skills/   # Codex / Copilot / Gemini CLI
   ```

2. 環境プロファイルを生成します（python3 が必要）。Unity プロジェクトがリポジトリの
   サブディレクトリにあっても、リポジトリルートを渡せば検出されます。

   ```bash
   python3 unity-perf-review/scripts/detect-env.py /path/to/your-repo /path/to/your-repo/tuning-env.md
   ```

   生成された tuning-env.md を開き、出荷プラットフォーム（platforms）と SideFX VAT の
   使用有無（vat）を手動で記入してください。unknown のままでも安全側（全ルール照合）で動きます。

3. エージェントにレビューを依頼します。

   ```
   この差分をパフォーマンス観点でレビューして
   Assets/Scripts/Battle/ 配下をチューニングバイブルのルールでチェックして
   ```

   自動発動しない場合は、依頼文にスキル名 unity-perf-review を明記してください。

## 運用のヒント

- 初回はリポジトリ全体ではなく、差分か特定ディレクトリから始めてください。運用歴のある
  コードベース全体では指摘が数十ルール規模になり、精査が追いつかなくなります
- 意図的にルールから外れている箇所は、references/rules-overrides.example.md を
  rules-overrides.md にリネームしてレベルを調整してください（そのままの名前では読まれません）
- ルールを追加・修正したい場合は references/rules-*.md を編集し、
  `python3 scripts/gen-checklists.py references/` でチェックリストを再生成してください
  （checklist-*.md は自動生成物のため直接編集しないでください）
- python3 が無い環境でも動きますが、一次スキャンはエージェントが grep を直接実行する
  フォールバックになり、実行の確実性が下がります

## 導入前の確認（セキュリティ）

スキルは「エージェントへの指示と実行可能スクリプト」を含む配布物です。導入前に
SKILL.md と scripts/ の中身を確認し、信頼できる配布元のバージョンを固定して使ってください。

## ライセンス / 出典

- ライセンス: MIT
- 出典: UnityPerformanceTuningBible / UnityPerformanceTuningBible 2026（CyberAgent）
