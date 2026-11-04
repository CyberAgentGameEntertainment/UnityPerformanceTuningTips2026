# レビューチェックリスト: アセット・設定（.meta/Prefab/ProjectSettings/ビルド）

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
| AN01 | Warning | - | `optimizeGameObjects: 0` | キャラクターFBXのOptimize Game Objects未適用 | rules-animation.md |
| AN08 | Warning | - | `extraExposedTransformPaths: \[\]` | Optimize Game Objects適用対象でボーン階層に依存するコード | rules-animation.md |
| AN12 | Critical | - | `m_Script:.*Animator` | 破片・部品ごとの個別MeshRenderer + Animator構成 | rules-animation.md |
| AN13 | Critical | vat | MANUAL | VATテクスチャの圧縮設定違反 | rules-animation.md |
| AN14 | Warning | vat | MANUAL | VAT Shader Graphテンプレートの直接利用・Sub Graph参照の挿げ替え漏れ | rules-animation.md |
| AS01 | Warning | - | `maxTextureSize:\s*(4096\|8192\|16384)` | テクスチャサイズ過大 | rules-asset.md |
| AS02 | Critical | - | `textureCompression:\s*0` | テクスチャ圧縮未設定 | rules-asset.md |
| AS03 | Warning | - | `isReadable:\s*1` | Read/Write Enabled不要な有効化 | rules-asset.md |
| AS04 | Warning | - | `textureType: 8` | Mipmap不要な有効化 | rules-asset.md |
| AS05 | Warning | - | MANUAL | メッシュ頂点数過大 | rules-asset.md |
| AS06 | Info | - | MANUAL | メッシュの不要な属性 | rules-asset.md |
| AS07 | Warning | - | `loadType:\s*0` | AudioClipのLoad Type不適切 | rules-asset.md |
| AS08 | Warning | - | `compressionFormat:\s*0` | AudioClipの圧縮設定 | rules-asset.md |
| AS09 | Warning | - | `maxNumParticles:\s*([2-9]\d{3}\|\d{5,})` | Particleの過剰なMax Particles | rules-asset.md |
| AS10 | Warning | - | `grep -A2 'CollisionModule:' \| grep 'enabled: 1'` | Particleの物理衝突有効 | rules-asset.md |
| AS11 | Info | - | `animationCompression:\s*0` | AnimationClipの精度過剰 | rules-asset.md |
| AS12 | Warning | - | MANUAL | 不要なアセット依存 | rules-asset.md |
| AS13 | Warning | - | MANUAL | Sprite Atlas未使用 | rules-asset.md |
| AS14 | Info | - | `m_AtlasWidth:\s*(8192\|16384)\|m_AtlasHeight:\s*(8192\|16384)` | Font Atlasサイズ過大 | rules-asset.md |
| AS15 | Warning | - | `#pragma\s+multi_compile\b` | Shaderバリアント過多 | rules-asset.md |
| AS17 | Critical | addressables | MANUAL | Addressables未登録の共有アセット（暗黙の依存の複製） | rules-asset.md |
| AS18 | Info | - | ABSENT `class\s+\w+\s*:\s*AssetPostprocessor` | インポート設定の自動適用の欠如 | rules-asset.md |
| AS19 | Warning | - | MANUAL | アセット規約のビルド前検査の欠如 | rules-asset.md |
| BD14 | Critical | il2cpp | `AssetBundle\.LoadFrom\w*\(` | AssetBundle専用コンテンツが参照する型・アセンブリの保持漏れ | rules-build-stripping.md |
| BD17 | Info | il2cpp | `managedStrippingLevel` | Managed Stripping Level Lowの使用 | rules-build-stripping.md |
| BD18 | Warning | il2cpp | MANUAL | Stripping Level引き上げ構成のビルド検証欠如 | rules-build-stripping.md |
| BD01 | Warning | il2cpp, android, unity>=6000.5 | `LinkTimeOptimization"[: ]*"Thin"` | 公式LTOトグル（engine ThinLTO）の有効化 | rules-build.md |
| BD02 | Warning | il2cpp | `stripEngineCode:\s*0` | Strip Engine Codeの無効化 | rules-build.md |
| BD04 | Info | il2cpp, android, unity>=6000.5 | `il2cppCompilerConfiguration` | 「性能のため」のMaster構成（Android 6000.5+） | rules-build.md |
| BD05 | Critical | il2cpp | `-flto(?!=thin)` | Full LTOの指定 | rules-build.md |
| BD06 | Warning | il2cpp, android | `compiler-flags.*-O3` | -O3 + ThinLTOの併用 | rules-build.md |
| BD07 | Info | il2cpp | `-f(whole-program-vtables\|split-lto-unit)` | -fwhole-program-vtables（whole-program devirtualization）の指定 | rules-build.md |
| BD08 | Info | il2cpp | `import-instr-limit=\d+` | -import-instr-limitのチューニング | rules-build.md |
| BD09 | Warning | il2cpp, ios | `-flto=thin` | iOSへのThinLTO適用 | rules-build.md |
| BD10 | Warning | il2cpp, ios | `LLVM_LTO\s*=\s*YES` | Xcode LLVM_LTOでIL2CPP生成コードにLTOを掛けようとする設定 | rules-build.md |
| BD11 | Warning | il2cpp | MANUAL | LTO適用のサイレント失敗（適用検証の欠如） | rules-build.md |
| BD19 | Warning | il2cpp, android, unity>=6000.5 | MANUAL | IL2CPP LTO Mode設定によるLTO有効化の誤解 | rules-build.md |
| GR03 | Warning | srp | ABSENT `CBUFFER_START\(UnityPerMaterial\)` | SRP Batcher非互換シェーダー | rules-graphics.md |
| GR10 | Critical | - | `m_Mode:\s*1` | Reflection Probeのリアルタイム更新 | rules-graphics.md |
| GR11 | Info | - | `aniso:\s*([5-9]\|1[0-6])` | テクスチャの異方性フィルタリング過剰 | rules-graphics.md |
| GR15 | Warning | - | `tex2D\(\|SAMPLE_TEXTURE2D\(\|SAMPLE_TEXTURE2D_LOD\(` | シェーダーのフラグメント処理過剰 | rules-graphics.md |
| PH01 | Warning | - | `MeshCollider:` | MeshCollider多用 | rules-physics.md |
| PH02 | Critical | - | `m_Convex:\s*0` | Convex未設定のMeshCollider | rules-physics.md |
| PH09 | Warning | - | `m_AutoSyncTransforms:\s*1` | Auto Sync Transforms有効化 | rules-physics.md |
| PH10 | Warning | - | `m_LayerCollisionMatrix: f{8,}` | Collision Matrixの未整理 | rules-physics.md |
| RP20 | Warning | urp | `m_RequireDepthTexture:\s*1` | Depth Texture有効だが_CameraDepthTexture未使用 | rules-rendering-tilememory.md |
| RP21 | Warning | urp, unity>=6000.0 | MANUAL | MSAAのResolve/メモリーレス構成の破壊 | rules-rendering-tilememory.md |
| RP05 | Info | urp | `RenderPassEvent\.(AfterRenderingPostProcessing\|AfterRendering)\b` | AfterRenderingPostProcessing以降のカスタムパスによる冗長FinalBlitPass | rules-rendering.md |
| RP12 | Warning | ios\|android | ABSENT `QualitySettings\.SetQualityLevel` | 描画品質切り替え機構の欠如（全端末一律設定） | rules-rendering.md |
| SH04 | Critical | unity>=6000.0 | MANUAL | GSCメタデータの実行環境不一致 | rules-shader.md |
| SH05 | Critical | unity>=6000.0, ios | MANUAL | シェーダーのAssetBundle重複パッキング（iOS/Metal） | rules-shader.md |
| SH08 | Warning | unity>=6000.0 | `QualitySettings\.SetQualityLevel` | Quality Settings非対応のGSC収集 | rules-shader.md |
| UI01 | Critical | - | `^Canvas:` | Canvas未分割 | rules-ui.md |
| UI02 | Warning | - | `m_RaycastTarget:\s*1` | Raycast Target不要な有効化 | rules-ui.md |
| UI06 | Warning | - | `a:\s*0\.0[0-9]\|a:\s*0\}` | 透明Imageのフィルレート消費 | rules-ui.md |
| UI10 | Critical | - | `UIDocument\|PanelRenderer` | uGUIとUI Toolkitの重ね合わせ設計 | rules-ui.md |
| US11 | Warning+ | - | `\.Simulate\(` | ParticleSystem.Simulateへの大きな秒数指定 | rules-unity-script.md |
