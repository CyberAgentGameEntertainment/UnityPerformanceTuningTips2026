# TuningBible ルールオーバーライド

> プロジェクト固有のルールレベル上書き設定。
> baseルールファイルは変更せず、このファイルでプロジェクト事情に合わせた調整を行う。
> エージェントはレビュー時にこのファイルを読み、該当ルールの適用条件を調整する。
> ファイル名が `rules-overrides.example.md` の場合は無効（読まれない）。
> `rules-overrides.md` にリネーム（またはコピー）して編集すると有効になる。

## ルール

- 未記載のルールはbase rulesのレベルがそのまま適用される
- override_levelはbaseより緩い方向のみ許可（Critical→Warning/Info/Skip、Warning→Info/Skip）
- context: `in-game`（ホットパス）、`init`（初期化）、`tooling`（Editor/テスト）
- Skip = 検出対象外（レポートに含めない）

## オーバーライド一覧

| ルールID | base | context | override_level | 備考 |
|----------|------|---------|---------------|------|
| CS01 | Critical | init | Warning | 初期化時のLINQは許容（頻度低） |
| CS01 | Critical | tooling | Skip | Editor/テストコードでは対象外 |
| CS02 | Critical | init | Warning | 初期化時の文字列操作は許容 |
| CS03 | Warning | init | Info | 初期化時のasync/awaitは低リスク |
| CS07 | Warning | init | Info | 初期化時のparams配列は許容 |
| CS12 | Warning | init | Info | 初期化時の例外スローは許容 |
| CS13 | Warning | init | Info | 初期化時のToStringは許容 |
| US05 | Critical | tooling | Skip | デバッグログはEditor用途で許容 |
| US06 | Warning | init | Info | 初期化時のInstantiate/Destroyは許容 |
| US07 | Warning | init | Info | 初期化時のCoroutineは許容 |

## 追加方法

テーブルに行を追加する。同一ルールIDで複数contextを指定可能。
プロジェクト固有の閾値変更が必要な場合は備考欄に記載し、エージェントが判断材料とする。

個別アセット・個別ファイルだけを例外にしたい場合は、備考欄にパスやglob、GUIDを明記する
（例: `AS02 | Critical | in-game | Skip | Assets/Textures/Lut/ 配下は意図的に非圧縮`）。
専用の適用範囲カラムは現バージョンでは未対応のため、エージェントは備考の記述を読んで判断する。
