# アセット設定 パフォーマンスルール

> 出典: UnityPerformanceTuningBible - tuning_practice_asset

## ルール一覧

### AS01 テクスチャサイズ過大
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: モバイル向けで2048x2048超のテクスチャ。maxTextureSizeは「上限キャップ」であり実サイズではない点に注意（小さい画像でもキャップが緩ければヒットする。実サイズ超過は目視/Editorスクリプトで確認）
- **grep_pattern**: `maxTextureSize:\s*(4096|8192|16384)`
- **理由**: VRAM消費増大、帯域圧迫
- **対処**: 用途に応じた適切なサイズ（UIは1024以下推奨）

### AS02 テクスチャ圧縮未設定
- **レベル**: Critical
- **対象**: アセット設定
- **検出パターン**: TextureImporterの非圧縮設定（textureCompression=0）
- **grep_pattern**: `textureCompression:\s*0`（`textureFormat: -1`はAutomatic=推奨状態のため検出対象にしない）
- **理由**: 非圧縮テクスチャはメモリ4-8倍消費（実測: 2048x2048でCompressed 2.7MB / Uncompressed 16.0MB、約6倍差）
- **対処**: ASTC(モバイル)、DXT/BC(PC)使用

### AS03 Read/Write Enabled不要な有効化
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: `isReadable: 1`（テクスチャ/メッシュ）
- **grep_pattern**: `isReadable:\s*1`
- **理由**: CPU側にもコピーを保持しメモリ2倍
- **対処**: ランタイムで読み書き不要なら無効化

### AS04 Mipmap不要な有効化
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: UI/2DスプライトでMipmap有効。3Dテクスチャ（Defaultタイプ）はMipmap有効が標準のため、`textureType: 8`（Sprite）との共起で判定する
- **grep_pattern**: `textureType: 8`を含む.metaで`enableMipMap:\s*1`（単独grepでは3Dテクスチャに全数誤爆する。UIをDefaultタイプで運用しているケースは手動確認）
- **理由**: メモリ約1.3倍（33%増加）、UIでは不要
- **対処**: UI/2DスプライトはMipmap無効

### AS05 メッシュ頂点数過大
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: モバイル向けで10万頂点超のメッシュ
- **grep_pattern**: (手動確認 — Model ImporterインスペクタやProject検索でメッシュの頂点数を確認)
- **理由**: 描画負荷・メモリ圧迫
- **対処**: LOD設定、頂点削減

### AS06 メッシュの不要な属性
- **レベル**: Info
- **対象**: アセット設定
- **検出パターン**: 使用しないNormal/Tangent/Color/UV2の同梱（「使用有無」はgrepで判定不能。手動確認）
- **grep_pattern**: (手動確認)（ModelImporterの実キーはtangentSpace配下のnormalImportMode/tangentImportMode。参考locator: `tangentImportMode:\s*[^2]`＝Tangent import有効）
- **理由**: 頂点あたりのデータ量増加
- **対処**: Player Settings > Optimize Mesh Data を有効化（使用シェーダーが参照しない属性をビルド時に自動除去）。個別にはModelImporterでNormals/Tangents=Noneを設定

### AS07 AudioClipのLoad Type不適切
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: 長いBGMが`Decompress On Load`（loadType: 0はデフォルト値のため全AudioClipにヒットするlocator。BGM/長尺ボイスのみ問題視する）
- **grep_pattern**: `loadType:\s*0`
- **理由**: 展開後データが全てメモリに載る
- **対処**: BGMは`Streaming`、短音SEは`Decompress On Load`、ボイスは`Compressed In Memory`

### AS08 AudioClipの圧縮設定
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: 非圧縮(PCM)のまま
- **grep_pattern**: `compressionFormat:\s*0`
- **理由**: メモリ大量消費
- **対処**: Vorbis(品質70%は一般的推奨値)/AAC。即時・大量再生するSEはADPCMも選択肢

### AS09 Particleの過剰なMax Particles
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: MaxParticles >= 2000（デフォルト値1000は全ParticleSystemに書き出されるため対象外）
- **grep_pattern**: `maxNumParticles:\s*([2-9]\d{3}|\d{5,})`
- **理由**: パーティクルバッファが大量メモリ確保
- **対処**: 視覚効果を維持しつつ最大数を削減。実際の放出数はEmission（Rate over Time/Bursts）の確認が本質

### AS10 Particleの物理衝突有効
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: ParticleSystem Collision有効（CollisionModuleブロックはenabled: 0でも必ずシリアライズされるため複数行マッチが必要）
- **grep_pattern**: `grep -A2 'CollisionModule:' | grep 'enabled: 1'` 相当の複数行検査
- **理由**: パーティクル毎に物理演算が走る
- **対処**: 必要最小限に限定

### AS11 AnimationClipの精度過剰
- **レベル**: Info
- **対象**: アセット設定
- **検出パターン**: Anim. Compression = Off
- **grep_pattern**: `animationCompression:\s*0`
- **理由**: キーフレームデータが非圧縮で肥大化
- **対処**: Keyframe Reduction使用

### AS12 不要なアセット依存
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: Addressables/AssetBundle間の循環参照、巨大な単一グループ
- **grep_pattern**: (手動確認 — Addressables Analyzeでグループ間依存・循環参照を確認)
- **理由**: ロード時間増大、メモリ断片化
- **対処**: 依存関係整理、適切な粒度分割

### AS13 Sprite Atlas未使用
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: 同一Canvas内で複数の個別スプライト
- **grep_pattern**: (手動確認 — Frame DebuggerのDrawCallとSpriteAtlas登録状況を確認)
- **理由**: DrawCall増加。SpriteAtlasでバッチング可能
- **対処**: SpriteAtlas使用

### AS14 Font Atlasサイズ過大
- **レベル**: Info
- **対象**: アセット設定
- **検出パターン**: TMP Font Atlasが4096x4096超
- **grep_pattern**: `m_AtlasWidth:\s*(8192|16384)|m_AtlasHeight:\s*(8192|16384)`
- **理由**: VRAM消費過剰
- **対処**: 使用文字セット限定、2048x2048推奨

### AS15 Shaderバリアント過多
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: multi_compileの多用（フォグ等の正当な用途やURP同梱シェーダーにも普遍的に存在するため、自作シェーダーでの乱用を手動精査）
- **grep_pattern**: `#pragma\s+multi_compile\b`
- **理由**: ビルドサイズ肥大化、初回シェーダーコンパイルのスパイク
- **対処**: Shader Feature使用、未使用バリアント除外。Project Settings > Graphics のShader Stripping / Strip Unused Variantsも併用

### AS16 Shader Warmup未実施
- **レベル**: Critical
- **対象**: プロファイル解析
- **検出パターン**: ゲーム中にProfilerで`Shader.CreateGPUProgram`スパイクが発生
- **grep_pattern**: (手動確認 — Profilerで`Shader.CreateGPUProgram`スパイクの有無を確認)
- **理由**: 初回描画時にシェーダーのGPUプログラムがコンパイルされ、数十〜数百msのスパイクが発生する
- **対処**: ロード画面中に`ShaderVariantCollection.WarmUp()`で事前コンパイル。GraphicsStateCollection(Unity 6+)も利用可能（モダンAPIでのPSOウォームアップ詳細はSH01-SH08参照）

### AS17 Addressables未登録の共有アセット（暗黙の依存の複製）
> 出典: UnityPerformanceTuningBible 2026 - 運用フェーズのアセット肥大化
- **レベル**: Critical
- **対象**: アセット構成（Addressablesグループ設計）
- **適用条件**: addressables
- **検出パターン**: Addressable未登録アセットを複数バンドルのアセットが参照している
- **grep_pattern**: (手動確認 — Addressables Analyzeで検出。2.0系で一時削除、2.3.1で復活)
- **理由**: 未登録アセットは参照元バンドルごとに複製される。原稿の図解例（非圧縮換算の目安）: 4MBテクスチャを2つのPrefabが参照しただけでバンドル合計10MB、専用グループ登録で6MBに削減。同時ロード時はメモリにも同数展開される
- **対処**: 共有アセットを専用グループに登録して明示的な依存にする。Analyze自動解消はグループ構成が書き換わるためビルド結果まで確認

### AS18 インポート設定の自動適用の欠如
> 出典: UnityPerformanceTuningBible 2026 - 運用フェーズのアセット肥大化
- **レベル**: Info
- **対象**: プロジェクト構成（Editorコード検査）
- **検出パターン**: AssetPostprocessorもPresetも存在せず、インポート設定が手動運用
- **grep_pattern**: (不在検出) `class\s+\w+\s*:\s*AssetPostprocessor`（ヒットが無ければフラグ。Presetアセットの有無も併せて確認）
- **理由**: Import時の自動適用が無いと圧縮設定の誤り・解像度の過剰・付随データ・形式ミスが再発し続ける
- **対処**: 条件で値を変えるならAssetPostprocessor、型ごとの初期値固定ならPresetを導入

### AS19 アセット規約のビルド前検査の欠如
> 出典: UnityPerformanceTuningBible 2026 - 運用フェーズのアセット肥大化
- **レベル**: Warning
- **対象**: プロジェクト構成 / CI設定
- **検出パターン**: 規約をテストとして検査する仕組みが無く、文書周知のみの運用
- **grep_pattern**: (手動確認 — CIスクリプト・テストコードの有無を確認)
- **理由**: .metaごと持ち込まれたアセットは設定を保持したまま入り、Import時適用をすり抜ける。「まとめ方の不一致」「バンドル間の重複」はImport時には判定できずビルド前検査でしか捕捉できない
- **対処**: ビルド前・自動ビルド段階で規約違反アセットを検査（Project Auditor CLI等）。実測値をログ/レポートで全員が見える場所に残す
