# レンダリングパイプライン パフォーマンスルール

> 出典: UnityPerformanceTuningBible 2026 - RenderGraph UnsafePass / FinalBlitPass省略 / Renderer Shader User Value / 端末別描画設定
> タイルメモリ/FBF系のルール（RP15-RP24）は rules-rendering-tilememory.md を参照。

## ルール一覧

### RP01 UnsafePass内SetRenderTargetのLoad/Store Action未指定
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: `AddUnsafePass`を使うコード内で`SetRenderTarget(target)`のようにRenderBufferLoadAction/StoreActionを省略して呼んでいる
- **grep_pattern**: `SetRenderTarget\(\s*[^,)]+\s*\)\s*;`（`AddUnsafePass`を含むファイルに絞って適用）
- **理由**: Action省略時は既定のLoad/Storeが適用され、Tile-Based GPUではタイルメモリとメインメモリ間の不要な転送が発生し性能低下する
- **対処**: Loadは全画面上書きなら`DontCare`・既存内容を読む場合のみ`Load`、Storeは後続で読むなら`Store`・使わないなら`DontCare`を明示

### RP02 UnsafePassでのUseTexture依存関係宣言漏れ
- **レベル**: Critical
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: `AddUnsafePass`のbuilderスコープ内で、SetRenderTargetやBlitTextureで使用するテクスチャに対する`builder.UseTexture(...)`宣言が無い
- **grep_pattern**: `AddUnsafePass`ヒットファイルのうち`UseTexture`不在を一次候補（突合は手動確認）
- **理由**: UnsafePassでは`SetRenderTarget`の呼び出しだけでは依存関係が登録されず、リソースのライフタイム管理や同期が正しく行われない
- **対処**: レンダーターゲット含む使用テクスチャすべてを`UseTexture`で依存関係として宣言

### RP03 ループ内でのRasterRenderPass大量登録（マージ不能Passチェーン）
- **レベル**: Info
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: ループ内で`AddRasterRenderPass`を呼び、Ping-pong Blur・Mip Pyramid生成などSubPassマージが成立しないPassチェーンをPass単位で登録
- **grep_pattern**: `AddRasterRenderPass`（ループ内か、マージ不能かは手動確認）
- **理由**: RasterRenderPass 1つあたり約1.8µsの追加固定コスト（Editor測定）。Blur100回でPass単位1080.7µs対UnsafePass統合898.5µs
- **対処**: マージできないPassチェーンは1つのUnsafePassにまとめSetRenderFunc内でターゲットを切り替え。GPU負荷は変わらない点に注意

### RP04 宣言的APIで表現できる処理へのUnsafePass使用
- **レベル**: Info
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: RasterRenderPass/ComputePassで表現可能な処理をUnsafePassで実装（Compatibility Modeからの暫定移行の恒久化を含む）
- **grep_pattern**: `AddUnsafePass`（表現可能かは手動確認）
- **理由**: UnsafePassはRender Graphの依存関係解析・SubPassマージ等の最適化と検証の一部を手放す。またUnsafePass/ComputePassはRasterパス列のNative Render Passマージを分断し、境界にStore→Load往復を生む（RP23参照）
- **対処**: 非公開APIが必要な場合・外部コードへCommandBufferを渡す場合・マージ不能Passチェーンの整理に限定

### RP05 AfterRenderingPostProcessing以降のカスタムパスによる冗長FinalBlitPass
- **レベル**: Info
- **対象**: コードレビュー / URP設定
- **適用条件**: urp
- **検出パターン**: 最終出力相当のカスタムパスを`RenderPassEvent.AfterRenderingPostProcessing`以降に注入し、`SwitchActiveTexturesToBackbuffer()`を使っていない
- **grep_pattern**: `RenderPassEvent\.(AfterRenderingPostProcessing|AfterRendering)\b`
- **理由**: FinalBlitPassがほぼ必ず追加され余計なフルスクリーンBlitが1回挟まる。高解像度・モバイルでは無視できないコスト
- **対処**: Unity 6.3以降なら最終出力カスタムパスで`resourceData.SwitchActiveTexturesToBackbuffer()`でBackBufferへ直接出力（カスタムFXAA例でBlit 3回→1回）

### RP06 SwitchActiveTexturesToBackbufferの呼び出し位置ガード欠如
- **レベル**: Critical
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.3
- **検出パターン**: `SwitchActiveTexturesToBackbuffer()`を呼んでいるが`cameraData.resolveFinalTarget`判定が無い、または呼び出し後に`activeColorTexture`等を読む後続パスがある
- **grep_pattern**: `SwitchActiveTexturesToBackbuffer`（`resolveFinalTarget`併存を二次grep）
- **理由**: BackBufferはReadできないため以降に`activeColorTexture`を読む処理は破綻。後続カメラがある場合に呼ぶとOverlayカメラの積み重ね設計と衝突し描画結果が混ざる・消える
- **対処**: 最後のカメラの、最終出力を行うカスタムパスの直前に限定して呼ぶ

### RP07 FinalBlit省略時のY反転判定の欠如・誤実装
- **レベル**: Critical
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.3
- **検出パターン**: `SwitchActiveTexturesToBackbuffer`使用パスで`GetTextureUVOrigin()`によるSource/Target比較が無い、`SystemInfo.graphicsUVStartsAtTop`で代用、またはSetRenderFunc外で呼んでいる
- **grep_pattern**: 一次検出は「`SwitchActiveTexturesToBackbuffer`ヒットファイルで`GetTextureUVOrigin`不在」（判定自体の欠如を捕捉）。`SystemInfo\.graphicsUVStartsAtTop`は同ファイル内に併存する場合のみ誤用フラグ
- **理由**: UV原点不一致時に反転しないと画面が上下逆になる。`graphicsUVStartsAtTop`はAPIのUV規約を示すだけで判定に使えない。`GetTextureUVOrigin()`は実行時GraphContext専用
- **対処**: SetRenderFunc内で`context.GetTextureUVOrigin(source) != context.GetTextureUVOrigin(target)`を判定しscaleBiasを切り替え

### RP08 FinalBlit省略時のsRGB変換の実装漏れ
- **レベル**: Critical
- **対象**: コードレビュー / シェーダー
- **適用条件**: urp, unity>=6000.3
- **検出パターン**: `SwitchActiveTexturesToBackbuffer`使用パスで`LinearToSRGBConversion`キーワード設定、またはシェーダー側`_LINEAR_TO_SRGB_CONVERSION`分岐が無い
- **grep_pattern**: (不在検出) `LinearToSRGBConversion|_LINEAR_TO_SRGB_CONVERSION`（C#/シェーダーいずれにも無ければフラグ。`SwitchActiveTexturesToBackbuffer`使用パスがある場合のみ該当）
- **理由**: Linear→sRGB変換はFinalBlitPassが担っていた最終処理で、省略時はカスタムパス側で肩代わりしないと色が正しく出ない（判定: `cameraData.targetTexture == null && Display.main.requiresSrgbBlitToBackbuffer`）
- **対処**: キーワード設定＋シェーダーで`LinearToSRGB(color)`実装。HDR出力タイトルはHDROutputUtils対応、Viewport非全画面なら`cmd.SetViewport`も必要

### RP09 大量描画でのRendererごとのマテリアル複製
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: unity>=6000.3
- **検出パターン**: 大量のRendererに個別バリエーションを与えるためRendererごとに`new Material`や`renderer.material`（暗黙複製）でマテリアルを量産
- **grep_pattern**: `new Material\s*\(|\.material\s*[=.]`（大量描画文脈かは手動確認）
- **理由**: 実測（10000 Sphere、iPhone 12）でマテリアル複製はメモリ+15.79MB（RSUVは1.66KB）。毎フレームパラメータ更新時11.1 FPS（GPU Resident Drawerありで8.1）に対しRSUVは48.0/60.0を維持
- **対処**: Unity 6.3以降、オブジェクトごとの差が32ビットに収まるなら1マテリアル＋`Renderer.SetShaderUserValue()`（シェーダー側`unity_RendererUserValue`）。32ビット超はGraphicsBufferにデータを置きRSUVにインデックスを渡す

### RP10 MaterialPropertyBlockによるSRP Batcher / GPU Resident Drawer非互換
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: srp
- **検出パターン**: SRP Batcher / GPU Resident Drawer利用プロジェクトでRendererごとのパラメータ設定に`MaterialPropertyBlock`を使用（Built-in RPではMPBはマテリアル複製を避ける標準手法であり対象外）
- **grep_pattern**: `MaterialPropertyBlock|SetPropertyBlock`
- **理由**: MaterialPropertyBlockはSRP BatcherおよびGPU Resident Drawerと互換性がなく対象外になる（バッチングの機会損失。MPB自体はマテリアル複製より更新時は高速）。実測28.9 FPS（GRDの有無で変化なし）に対しRSUVは49.9/59.8
- **対処**: Unity 6.3以降でパラメータが32ビットに収まるならRSUVへ移行。32ビット超はGraphicsBufferにデータを置きRSUVでインデックス参照

### RP11 GraphicsBufferの更新頻度・解放管理
- **レベル**: Info
- **対象**: コードレビュー
- **検出パターン**: GraphicsBufferの高頻度な`SetData`更新や`Dispose()`漏れ
- **grep_pattern**: `new GraphicsBuffer\(`
- **理由**: GraphicsBufferのアップロードには追加コストがかかるため更新頻度・サイズに配慮が必要
- **対処**: 更新頻度・サイズを抑え、不要になったら確実にDispose

### RP12 描画品質切り替え機構の欠如（全端末一律設定）
- **レベル**: Warning
- **対象**: コードレビュー / ProjectSettings
- **適用条件**: ios|android
- **検出パターン**: `QualitySettings.SetQualityLevel`呼び出しやRenderPipelineAsset切り替えが存在せず、全端末が単一の描画設定
- **grep_pattern**: (不在検出) `QualitySettings\.SetQualityLevel`（ヒット無し＋Quality Levelが実質1つならフラグ）
- **理由**: 一律設定は低スペック端末でクラッシュ・処理落ち・発熱の原因になり、高スペック端末では品質向上の機会を逃す
- **対処**: QualityでAdd Quality Levelによりプリセット追加し各プリセットにRenderPipelineAssetをアタッチ、`SetQualityLevel`で切り替え。直接操作型なら`(UniversalRenderPipelineAsset)GraphicsSettings.currentRenderPipeline`のrenderScale等を操作（このキャスト方式はURP前提）

### RP13 Android端末のスペック判定を機種名文字列比較のみで実施
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: android
- **検出パターン**: `SystemInfo.deviceModel`/`graphicsDeviceName`の文字列比較だけでAndroidの品質を自動判定
- **grep_pattern**: `SystemInfo\.(deviceModel|graphicsDeviceName)`（Android向け判定ロジックか手動確認）
- **理由**: Androidは機種によって返る値がまばらで一概に比較できない（iOSは命名規則が一貫し、同系列ならA/M後の数字大＝高スペック）
- **対処**: Androidは (1) systemMemorySize（RAM）で割り切る、(2) マスタデータに逃がして比較、(3) 起動時ベンチマーク、のいずれかで切り分け

### RP14 自作RendererFeatureに品質切り替えAPIがない
- **レベル**: Info
- **対象**: コードレビュー
- **適用条件**: urp
- **検出パターン**: ScriptableRendererFeature派生クラスに品質段階に応じた設定変更手段が無い
- **grep_pattern**: `class\s+\w+\s*:\s*ScriptableRendererFeature`（切り替えAPIの有無は手動確認）
- **理由**: 自作RendererFeatureも端末スペックに応じて負荷を落とせると低スペック端末対応の選択肢が増える
- **対処**: 一時テクスチャのフォーマット/解像度を下げる、Blit数を減らす、簡易シェーダー置き換え等の切り替えAPIを用意
