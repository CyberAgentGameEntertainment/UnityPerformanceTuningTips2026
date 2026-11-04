# レビューチェックリスト: プロファイル解析

> このファイルは gen-checklists.py（配布パッケージでは scripts/ に同梱）により rules-*.md から自動生成される。
> 直接編集せず、ルールを修正・追加したら再生成すること（例: python3 scripts/gen-checklists.py references/）。
> 用途: レビュー一次スキャン。全行の grep を機械実行し、ヒットしたルールのみ元ファイルの詳細（検出パターン・理由・対処）を読む。
> `MANUAL` は grep 不能ルール。対象ファイル種別に該当する場合は精読で確認する。
> `ABSENT` は不在検出ルール。grep でパターンが**見つからない場合**に該当を疑い、詳細を読んで適用条件を確認する（ヒットがあれば通常は非該当）。
> 適用条件列: tuning-env.md の環境プロファイルと突き合わせ、明確に条件を満たさない行はスキップしてよい
> （スキップ数はCoverageの「環境対象外」に計上）。カンマ=AND、`|`=OR。tuning-env.md が無い/unknown の場合はスキップしない。
> 注意: `-flto` など `-` で始まるパターンをシェルの grep に渡すときは `grep -e 'パターン'` を使う（引数と誤解釈される）。

| ID | レベル | 適用条件 | grep_pattern | ルール名 | 詳細 |
|----|--------|----------|--------------|----------|------|
| AS16 | Critical | - | MANUAL | Shader Warmup未実施 | rules-asset.md |
| GR01 | Warning | - | ABSENT `Screen\.SetResolution\(\|renderScale` | 解像度スケーリング未設定 | rules-graphics.md |
| GR02 | Info | - | MANUAL | 動的バッチング対象外メッシュ | rules-graphics.md |
| GR04 | Warning | - | MANUAL | Overdraw過多 | rules-graphics.md |
| GR05 | Warning | - | `shadowResolution:\s*3\|m_MainLightShadowmapResolution:\s*4096\|m_ShadowDistance:\s*([5-9]\d\|\d{3,})\|shadowDistance:\s*([5-9]\d\|\d{3,})` | 影の解像度・距離過大 | rules-graphics.md |
| GR06 | Critical | - | `m_Type:\s*[02]` | リアルタイムライト過多 | rules-graphics.md |
| GR07 | Warning | - | `m_Name:\s*(Bloom\|DepthOfField\|Vignette\|MotionBlur)` | ポストプロセス重複・過剰 | rules-graphics.md |
| GR08 | Info | - | `m_MSAA:\s*[48]\|antiAliasing:\s*[48]` | カメラのMSAA過剰 | rules-graphics.md |
| GR09 | Warning | - | `[Cc]amera\w*\.enabled\s*=\s*true` | 不要なカメラ描画 | rules-graphics.md |
| GR12 | Info | - | MANUAL | GPU Skinning未使用 | rules-graphics.md |
| GR13 | Info | - | MANUAL | Occlusion Culling未設定 | rules-graphics.md |
| PH03 | Warning | - | `Fixed Timestep:\s*0\.00` | Fixed Timestep不適切 | rules-physics.md |
| PH07 | Warning | - | `m_IsTrigger:\s*1` | トリガーコライダー過多 | rules-physics.md |
| PH15 | Info | - | `m_SimulationMode:\s*[01]\|m_AutoSimulation:\s*1` | Auto Simulation不要時の有効化 | rules-physics.md |
| RP21 | Warning | urp, unity>=6000.0 | MANUAL | MSAAのResolve/メモリーレス構成の破壊 | rules-rendering-tilememory.md |
