# グラフィックス パフォーマンスルール

> 出典: UnityPerformanceTuningBible - tuning_practice_graphics

## ルール一覧

### GR01 解像度スケーリング未設定
- **レベル**: Warning
- **対象**: プロファイル解析
- **検出パターン**: モバイルでネイティブ解像度のまま描画（逆引き検出: grepがマッチしない=スケーリング未実装の疑い）
- **grep_pattern**: (不在検出) `Screen\.SetResolution\(|renderScale`（マッチ無しで違反候補）
- **理由**: フィルレート負荷がGPUボトルネックの主因
- **対処**: URPならRender Scale（0.7-0.8）またはResolution Scaling Mode (Fixed DPI)、Built-inは`Screen.SetResolution`で70-80%スケーリング（SetResolutionはUI解像度も下がる点に注意）

### GR02 動的バッチング対象外メッシュ
- **レベル**: Info
- **対象**: プロファイル解析
- **検出パターン**: 頂点数300超の動的オブジェクト多数
- **grep_pattern**: (手動確認 — Frame Debuggerでバッチング状況と動的オブジェクトの頂点数を確認)
- **理由**: 動的バッチングの頂点数制限を超えDrawCall増加
- **対処**: SRP Batcher/GPU Instancing活用

### GR03 SRP Batcher非互換シェーダー
- **レベル**: Warning
- **対象**: アセット設定
- **適用条件**: srp
- **検出パターン**: Propertiesブロックを持つ手書き.shaderで`CBUFFER_START(UnityPerMaterial)`が無いもの（逆引き検出。プロパティ無しシェーダーやShader Graph生成物はCBUFFER無しでも互換）
- **grep_pattern**: (不在検出) `CBUFFER_START\(UnityPerMaterial\)`（マッチ無し+Propertiesありで違反候補）
- **理由**: SRP Batcherの恩恵を受けられずDrawCall削減不可
- **対処**: マテリアルプロパティをUnityPerMaterial CBUFFERに配置

### GR04 Overdraw過多
- **レベル**: Warning
- **対象**: プロファイル解析
- **検出パターン**: 半透明オブジェクト重なり、パーティクル密集
- **grep_pattern**: (手動確認 — SceneビューのOverdraw表示やGPUプロファイラで重なりを確認)
- **理由**: ピクセル毎に複数回シェーダー実行
- **対処**: 半透明削減、パーティクル間引き、描画順序最適化

### GR05 影の解像度・距離過大
- **レベル**: Warning
- **対象**: プロファイル解析
- **検出パターン**: Shadow Resolution 4096, Shadow Distance > 50
- **grep_pattern**: `shadowResolution:\s*3|m_MainLightShadowmapResolution:\s*4096|m_ShadowDistance:\s*([5-9]\d|\d{3,})|shadowDistance:\s*([5-9]\d|\d{3,})`（QualitySettings.assetはshadowResolution: 0-3、URPアセットはm_MainLightShadowmapResolution/m_ShadowDistance）
- **理由**: シャドウマップ描画がGPU負荷の大きな割合を占める
- **対処**: モバイルは1024-2048、距離20-30推奨

### GR06 リアルタイムライト過多
- **レベル**: Critical
- **対象**: プロファイル解析
- **検出パターン**: Additional Lights > 4（モバイル。シーン内のSpot/Pointライト数をカウントして判定）
- **grep_pattern**: `m_Type:\s*[02]`のヒット数カウント（Lightの実フィールドはm_Type。Spot=0/Point=2。Directional=1は通常1灯で正当なため対象外）
- **理由**: ライト毎にパス追加、フラグメントシェーダー負荷増大（Unity 6 URPのForward+ではper-object制限は緩和されるが総負荷は増える）
- **対処**: ベイク活用、Light Probe、ライト数制限

### GR07 ポストプロセス重複・過剰
- **レベル**: Warning
- **対象**: プロファイル解析
- **検出パターン**: Bloom+ColorGrading+Vignette+DOF同時使用
- **grep_pattern**: `m_Name:\s*(Bloom|DepthOfField|Vignette|MotionBlur)`（Volume Profileの各エフェクトはサブアセットのm_Nameとしてシリアライズされる。`active: 1`との複合確認）
- **理由**: フルスクリーンパス毎にRenderTexture確保+描画
- **対処**: モバイルはBloom+ColorGrading程度に限定

### GR08 カメラのMSAA過剰
- **レベル**: Info
- **対象**: プロファイル解析
- **検出パターン**: MSAA 4x以上（要文脈判断）
- **grep_pattern**: `m_MSAA:\s*[48]|antiAliasing:\s*[48]`（URPアセットはm_MSAA、Built-inのQualitySettingsはantiAliasing）
- **理由**: 非タイル型GPUやRenderTexture併用時は帯域・メモリ増。ただしタイルベース（TBDR）のモバイルGPUではMSAA 4xはタイルメモリ内で解決され比較的安価なため、一律の削減指示は古い指針
- **対処**: ターゲットGPUで実測して判断。帯域が問題になる構成では2xまたはFXAA代替。Unity 6 URP + Render Graphでは、MSAA有効時にRender Graphが「MSAAバッファはメモリーレス＋Resolveのみストア」の構成を自動で組むため、Render Graph ViewerでStore Action=Resolve・Memoryless=yesを確認し、壊れていれば後続パスの申告を修正（RP21参照）

### GR09 不要なカメラ描画
- **レベル**: Warning
- **対象**: コードレビュー / プロファイル解析
- **検出パターン**: 非表示状態でもenableなCamera（コード側の機械検出は困難。Profilerでカメラ数を確認するのが確実）
- **grep_pattern**: `[Cc]amera\w*\.enabled\s*=\s*true`（camera系変数名ヒューリスティック。`\.enabled\s*=`全般の検出は誤爆過多のため廃止）
- **理由**: カメラ毎にCulling+描画パイプラインが走る
- **対処**: 不要時はCamera.enabled = false

### GR10 Reflection Probeのリアルタイム更新
- **レベル**: Critical
- **対象**: アセット設定
- **検出パターン**: Reflection Probe Mode = Realtime かつ Refresh Mode = Every Frame
- **grep_pattern**: `m_Mode:\s*1`と`m_RefreshMode:\s*1`の複合条件（RefreshModeはOnAwake=0/EveryFrame=1/ViaScripting=2。旧パターンの`m_RefreshMode: 0`は無害なOnAwakeを検出し危険なEveryFrameを見逃す誤りだった）
- **理由**: 6面キューブマップを毎フレーム描画
- **対処**: Baked/Custom、またはVia Scriptingで頻度制御

### GR11 テクスチャの異方性フィルタリング過剰
- **レベル**: Info
- **対象**: アセット設定
- **検出パターン**: Aniso Level > 4（モバイル）
- **grep_pattern**: `aniso:\s*([5-9]|1[0-6])`（テクスチャ.metaの実キーはtextureSettings配下のaniso。m_Anisoはビルド済みアセット/RenderTexture側の名称）
- **理由**: テクスチャサンプリングコスト増加（効果は0-1/2-3/4-7/8以上の段階で変化）
- **対処**: モバイルは2-4推奨

### GR12 GPU Skinning未使用
- **レベル**: Info
- **対象**: プロファイル解析
- **検出パターン**: SkinnedMeshRendererのCPUスキニング（GPU Skinningはデフォルト有効のため該当ケースは稀）
- **grep_pattern**: (手動確認 — Player SettingsのGPU Skinning / Mesh Deformation設定を確認)
- **理由**: CPU側で頂点変換が走りボトルネック
- **対処**: Player Settings > Other Settings > GPU Skinning有効化（Unity 6ではMesh DeformationドロップダウンをGPUまたはGPU (Batched)に。「Compute Deformation」はEntities Graphics(DOTS)のShader Graphノード名で別物）

### GR13 Occlusion Culling未設定
- **レベル**: Info
- **対象**: プロファイル解析
- **検出パターン**: 遮蔽物が多いシーンでOcclusion Culling未使用
- **grep_pattern**: (手動確認 — シーンのOcclusion Culling Bake有無を確認)
- **理由**: 見えないオブジェクトも描画される
- **対処**: Occlusion Culling Bake実施

### GR14 RenderTexture解像度過大
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `new RenderTexture(`で高解像度指定（解像度はgrepで判定不能。生成箇所を列挙し目視確認するlocator）
- **grep_pattern**: `new\s+RenderTexture\(|RenderTexture\.GetTemporary\(`
- **理由**: VRAM大量消費+描画コスト
- **対処**: 用途に応じた適切な解像度

### GR15 シェーダーのフラグメント処理過剰
- **レベル**: Warning
- **対象**: コードレビュー / アセット設定
- **検出パターン**: フラグメントシェーダー内のテクスチャサンプル5回超（回数はファイル単位のヒット数集計で判定）
- **grep_pattern**: `tex2D\(|SAMPLE_TEXTURE2D\(|SAMPLE_TEXTURE2D_LOD\(`
- **理由**: モバイルGPUのテクスチャ帯域ボトルネック
- **対処**: テクスチャパッキング、LODシェーダー
