---
name: unity-perf-review
description: >
  Unity プロジェクトのコード・アセット・設定をパフォーマンス観点でレビューする。
  C# スクリプト、シェーダー、.meta / Prefab / ProjectSettings、ビルドスクリプトを
  UnityPerformanceTuningBible 由来の 163 ルール（GC アロケーション、描画、アニメーション、
  PSO ウォームアップ、LTO、ライフタイム管理等）と照合し、Critical / Warning / Info の
  指摘リストを出力する。Unity のパフォーマンスレビュー、チューニング、最適化調査、
  フレームスパイクやメモリリークの静的調査を依頼されたときに使う。
license: MIT
metadata:
  source: UnityPerformanceTuningBible / UnityPerformanceTuningBible 2026 (CyberAgent)
  version: "2.0"
---

# Unity Performance Review

Unity プロジェクトをパフォーマンス観点で静的レビューするスキル。
ルール本体は `references/` 配下にあり、対象ファイルの種類に応じて必要なカテゴリだけを読み込む。

## 実行フロー（チェックリスト2段階方式）

1. **対象の特定**: レビュー対象を決める。指定がなければ VCS の差分（例: `git diff --name-only`）を対象とし、差分が無ければユーザーに対象ディレクトリを確認する。
2. **チェックリスト読み込み**: 対象ファイルの種類に応じて `references/checklist-code.md`（.cs / .shader / .hlsl）と `references/checklist-asset.md`（.meta / .prefab / .unity / ProjectSettings / Addressables 構成）を読み込む。チェックリストは全ルールの「ID / レベル / 適用条件 / grep_pattern / ルール名 / 詳細ファイル名」を1行ずつ持つ。
3. **環境プロファイル**: プロジェクトに `tuning-env.md`（環境プロファイル）が無ければ `scripts/detect-env.py <プロジェクトルート> tuning-env.md` で生成する。適用条件タグ（`unity>=X` / `urp` / `srp` / `il2cpp` / `ios` / `android` / `addressables` / `vat`）との突き合わせは次ステップの scan.py が `--env` で決定的に行う（**明確に条件を満たさない行のみ**スキップ、環境値が unknown のタグはスキップしない=安全側）。
4. **一次スキャン**: 同梱スクリプトで全行の grep_pattern を機械実行する。

   ```bash
   python3 scripts/scan.py references/checklist-code.md references/checklist-asset.md \
       --env tuning-env.md --targets <対象ファイル/ディレクトリ...>
   ```

   対象がコードのみ/アセットのみなら該当チェックリストだけを渡す。tuning-env.md が無ければ `--env` を省略（全ルール照合）。出力にはヒット一覧（ファイル:行）、ABSENT 該当疑い、MANUAL 一覧、Coverage が含まれる。python3 が無い環境ではフォールバックとして、エージェント自身が全行の grep を1本ずつ実行する（行の飛ばし・抜粋は禁止。パターンは Markdown 表の `\|` を `|` に復元し、先読み等の PCRE 構文を含むため `grep -P` 相当で実行する）。`MANUAL` 行は対象ファイルの種別・内容が該当しうる場合のみ精読対象とする。`ABSENT` 行は不在検出ルール: パターンが**見つからない場合**に該当を疑い、詳細を読んで適用条件を確認する（ヒットがあれば通常は非該当）。
5. **二次精査**: ヒットしたルールについてのみ、チェックリストの「詳細」列に記載されたルールファイルを `references/` から読み込み、`検出パターン` の条件で誤検出を除外して指摘化する。
6. **レベル判定**: ルール記載のレベル（Critical / Warning / Info）を適用する。`references/rules-overrides.md` が存在する場合、コンテキスト（in-game / init / tooling）ごとの上書きを適用し、Skip 指定はレポートから除外する。
7. **レポート出力**: 下記フォーマットで出力する。各指摘に必ずルール ID を付け、根拠にはルールの理由欄（実測値を含む）を引用する。Coverage セクションは必須。

## レベル定義

- **Critical**: パフォーマンス劣化の確実な原因。必ず修正。
- **Warning**: 潜在的リスク。条件次第で問題化する。
- **Info**: 改善推奨。任意対応。

## 出力フォーマット

```markdown
## Unity Performance Review

### Critical
- `ファイルパス:行番号` - 指摘内容 [ルールID]
  - 根拠: なぜ問題か（ルールの実測値を引用）
  - 推奨: どう修正すべきか

### Warning
（同上）

### Info
（同上）

### Summary
- Critical: N件 / Warning: N件 / Info: N件
- 対象ファイル数: N

### Coverage
- 照合ルール数: N（使用したチェックリストの全行数 - 環境対象外）
- 環境対象外スキップ: N件（tuning-env.md 判定。スキップしたルールIDを列挙）
- grepヒット: N件 / 精査後採用: N件 / 誤検出除外: N件
- MANUAL確認実施: N件（対象外としてスキップ: N件）
```

Coverage の行数・スキップ・ヒット件数は scan.py の「Coverage（機械生成）」出力をそのまま転記し、二次精査で決めた採用・除外件数のみエージェントが追記する。

## 注意事項

- 初期化時（Awake / Start / コンストラクタ）のアロケーションは既定で許容（overrides で調整可能）
- `#if UNITY_EDITOR` 内のコード・テストコード・Editor 専用アセットは対象外（rules-build / UI09 / UI11 等の Editor 対象ルールを除く）
- 設計やコード規約の指摘はしない。パフォーマンス観点のみ
- ルール ID を必ず付与し、根拠の追跡性を確保する
- コードとアセット設定の突き合わせが必要なルール（US11 の Prefab NoiseModule 確認等）は、差分レビューで片側しか差分に含まれない場合でも、参照先の Prefab / 設定ファイルを読み込んで判定する
- ランタイム挙動の検証（プロファイリング）はこのスキルの対象外。静的レビューで判断できない疑いは「プロファイル計測推奨」として報告する。プロファイルデータの症状別解釈は `references/diagnosis-knowledge.md` を参照

## references/ と scripts/ の内容

- `references/checklist-code.md` / `checklist-asset.md` — 静的レビューの一次スキャン用チェックリスト（自動生成。常にここから照合を始める）
- `references/checklist-profile.md` — プロファイル解析用チェックリスト（自動生成。静的レビューでは使用しない付属資料）
- `references/rules-*.md` — カテゴリ別ルール集（13ファイル・163ルール。ヒット時のみ参照）
- `references/diagnosis-knowledge.md` — プロファイル症状→原因→対処の診断パターン集
- `references/rules-overrides.md` — （任意）プロジェクト固有のレベル上書き。同梱の `rules-overrides.example.md` はそのままでは読まれない。この名前にリネーム（またはコピー）して編集すると有効になる
- `scripts/scan.py` — 一次スキャンの機械実行。チェックリスト全行の grep_pattern を対象ファイルに適用し、ヒット一覧・ABSENT 該当疑い・MANUAL 一覧・Coverage を Markdown で出力する。`--env tuning-env.md` で環境フィルタも決定的に適用する
- `scripts/gen-checklists.py` — チェックリストの再生成。ルール（`references/rules-*.md`）を修正・追加した場合に `python3 scripts/gen-checklists.py references/` を実行してチェックリストを同期する（チェックリストは直接編集しない）
- `scripts/detect-env.py` — 環境プロファイル（tuning-env.md）の自動生成。Unity バージョン・レンダーパイプライン・Scripting Backend・Addressables 有無を検出する（**python3 が必要**。無い環境では tuning-env.md をこのスクリプト内のテンプレートに沿って手動作成するか、環境フィルタなし＝全ルール照合で進める）
