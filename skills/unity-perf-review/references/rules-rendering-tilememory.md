# レンダリングパイプライン パフォーマンスルール: タイルメモリ最適化

> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
> 本ファイルの数値は原稿の「解像度×バイト数」による試算値（1080p基準）であり、実機計測のGPU時間ではない。
> RenderGraph全般のルール（RP01-RP14）は rules-rendering.md を参照。

## ルール一覧

### RP15 全画面上書きパスのAccessFlags.Write申告（WriteAll未使用）
> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: 全画面Blit等、必ず全ピクセルを上書きするRasterRenderPassで`builder.SetRenderAttachment(..., AccessFlags.Write)`と申告している（`WriteAll`でない）
- **grep_pattern**: `SetRenderAttachment\([^)]*AccessFlags\.Write\b`（`Write\b`は`WriteAll`にマッチしない。全ピクセル上書きかは手動確認）
- **理由**: `Write`は「一部のピクセルにしか書かないかもしれない」という申告のため、Render Graphは未書き込みピクセル保全のためロードアクションをLoadにせざるを得ない。`WriteAll`ならDontCareになる。1080pのRGBA8ではこの指定の違いだけで約8MB/フレーム（60FPSで約475MB/s）のロードが発生するかどうかが変わる（原稿試算）
- **対処**: 全ピクセルを上書きするパスは`AccessFlags.WriteAll`を指定。以前の内容が不要なだけなら`Discard`。描画結果は変わらないためRender Graph ViewerでロードアクションがDontCareになったことを確認する

### RP16 同一ピクセル読みへのUseTexture使用（SetInputAttachment/FBF未活用）
> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
- **レベル**: Warning
- **対象**: コードレビュー / シェーダー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: 前のパスの結果を「処理中のフラグメントと同じピクセル位置」でしか読まない処理（トーンマッピング、カラーグレーディング、合成、ディファードライティング等）を`builder.UseTexture` + `SAMPLE_TEXTURE2D`で実装している。`SetInputAttachment` + `LOAD_FRAMEBUFFER_X_INPUT`ならタイルメモリ内で完結できる
- **grep_pattern**: `\.UseTexture\(`（同一ピクセル読みで済む処理かは手動確認。オフセットサンプリング・フィルタリング・ミップが必要な処理〈ブラー、歪み、Bloom〉は正当なUseTexture）
- **理由**: UseTextureは前パス出力のStoreを強制し、パスマージも妨げる（ストア＋サンプリング読み込みの往復）。SetInputAttachmentはマージを妨げず、マージ成立時はストアをDontCareにできタイルメモリ内で完結する。ディファードのG-Buffer 3枚（RGBA8、1080p）ならストアだけで約24MB/フレーム、60FPSで約1.4GB/sの帯域削減（原稿試算）
- **対処**: C#側で`builder.SetInputAttachment(handle, n)`を申告し、シェーダー側は`FRAMEBUFFER_INPUT_X_HALF(n)`＋`LOAD_FRAMEBUFFER_X_INPUT(n, input.positionCS.xy)`で読む。1サブパスの入力を含むアタッチメント合計は8枚上限のため、G-Bufferはチャンネルパッキングで枚数を節約。深度が必要なら線形化深度をカラーアタッチメント（R16_Float等）に書いてフェッチする代替案が確実。非対応環境では通常テクスチャ読みにfallbackするため、効果はRender Graph Viewer（入力側バッファのStoreがDontCareか）と実機帯域計測で確認する。iOS/MetalではSubPass内シェーダーのGSCウォームアップ問題（SH06）に注意

### RP17 SetGlobalTextureAfterPassによるRender Graph追跡の切断
> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: `SetGlobalTextureAfterPass`等でRender Graph管理テクスチャをグローバルテクスチャ化している（「とりあえずグローバル」なコード）
- **grep_pattern**: `SetGlobalTextureAfterPass\s*\(`
- **理由**: グローバル化されたテクスチャはRender Graphから利用状況を追跡できず、常にStoreされ、メモリーレス化の対象からも外れる（1080p RGBA8のStore 1回は約7.9MB、60FPSで約475MB/s。原稿試算）。パスマージの妨げにもなる
- **対処**: グローバルテクスチャ化は最後の手段に限定。利用パスが特定できるなら`UseTexture`（同一ピクセル読みなら`SetInputAttachment`）で明示的に依存を張る

### RP18 ImportTextureのImportResourceParams未指定
> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: 毎フレーム内容を作り直すRTHandleを`renderGraph.ImportTexture(handle)`のように`ImportResourceParams`なしでインポートしている
- **grep_pattern**: `ImportTexture\(\s*[^,)]+\s*\)`（第2引数なしの呼び出し。以前の内容を持ち越す正当なケースかは手動確認）
- **理由**: インポートしたテクスチャはグラフ外の利用が不明なため保守的に扱われ、初回利用でもロードされ最後の利用後もストアされる。1080p RGBA8で往復約16MB/フレーム、60FPSで約950MB/s相当が無駄になり得る（原稿試算）
- **対処**: 毎フレーム作り直すなら`ImportResourceParams { clearOnFirstUse = true, discardOnLastUse = true }`を渡す。そもそもライフサイクル管理が本当に必要なテクスチャ以外は`renderGraph.CreateTexture`（Render Graph管理）に任せるのが基本（自動で判断され、メモリーレス化の対象にもなる）

### RP19 中間バッファTextureDescの手組みによるマージ条件不一致
> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: カメラカラーとマージさせたい中間バッファの`TextureDesc`を`new TextureDesc(...)`で手組みし、解像度やMSAAサンプル数が元と食い違っている（MSAA有効カメラで特に発生しやすい）
- **grep_pattern**: `new\s+TextureDesc\s*\(`（マージ対象とする元アタッチメントと解像度・サンプル数が一致するかは手動確認）
- **理由**: パスマージには出力アタッチメントの解像度一致・MSAAサンプル数一致が必要。MSAAが有効なカメラカラーとMSAAなしの中間バッファはマージできず、境界にStore→Loadの往復が発生する（1080p RGBA8で往復約16MB/フレーム、60FPSで約950MB/s。原稿試算）。FBFも同一Render Pass内でしか使えないため無効化される
- **対処**: `renderGraph.GetTextureDesc(cameraColor)`のように元のDescから複製してサンプル数・解像度を引き継ぎ、必要なフィールドだけ変更する。ダウンサンプリング等で解像度を意図的に変える場合はその境界でマージが切れることを了解の上で設計する

### RP20 Depth Texture有効だが_CameraDepthTexture未使用
> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
- **レベル**: Warning
- **対象**: URP設定 / シェーダー
- **適用条件**: urp
- **検出パターン**: URPアセットのDepth Textureがオンだが、プロジェクトのシェーダー・ポストプロセスが`_CameraDepthTexture`を要求していない
- **grep_pattern**: `m_RequireDepthTexture:\s*1`（ヒットした場合、シェーダー/C#側で`_CameraDepthTexture`参照が存在するかを二次grep。参照ゼロなら違反）
- **理由**: Depth Textureをオンにするとカメラ深度がStoreされるようになる。誰も要求していないのに設定だけオンになっているのは実プロジェクトでよくある無駄（原稿）。深度（D24S8/D32）のStore 1回は1080pで約7.9MB、60FPSで約475MB/s（原稿試算）
- **対処**: `_CameraDepthTexture`の利用実態がなければURPアセット（および各カメラのoverride）のDepth Textureをオフにする。各アタッチメントのStoreに「後続の利用」という正当な理由があるかを1つずつ確認するのがタイルメモリ最適化の基本

### RP21 MSAAのResolve/メモリーレス構成の破壊
> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
- **レベル**: Warning
- **対象**: コードレビュー / URP設定 / プロファイル解析
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: MSAA有効プロジェクトで、MSAAバッファ（4サンプル）そのものを後続パスがテクスチャとして要求している（カスタムパスの誤った申告、Opaque Textureの構成ミス等）。Render Graph ViewerでMSAAカラーのStore ActionがResolveでない／Memorylessがyesでない状態
- **grep_pattern**: (手動確認 — `m_MSAA:\s*[48]`ヒットのプロジェクトを対象に、Render Graph ViewerでMSAAカラーバッファのStore=Resolve・Memoryless=yesを確認)
- **理由**: 4xMSAAカラーをResolveせずStoreするとデータ量は4倍になる。正しく構成されたMSAA（カラー・深度ともメモリーレス、Resolve後1サンプルのみストア）が壊れると、1080pで4xMSAAカラー約31.7MB＋4xMSAA深度約31.7MBの計60MB超のメモリと4サンプル分のストア帯域が発生する（原稿試算）
- **対処**: URPのMSAA設定を有効にすればRender Graphがこの構成を自動的に組む。壊している後続パスの申告（UseTexture対象がリゾルブ先でなくMSAAバッファになっていないか）を修正し、Render Graph Viewerで再確認する

### RP22 タイルメモリ内完結リソースのmemoryless明示指定の欠如
> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
- **レベル**: Info
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: `SetInputAttachment`（FBF）でサブパス間受け渡しのみを行う設計の中間バッファに、`TextureDesc.memoryless`の明示指定がない（自動メモリーレス化に暗黙依存）
- **grep_pattern**: (不在検出) `\.memoryless\s*=|RenderTextureMemoryless`（`SetInputAttachment`使用プロジェクトでヒット無しなら提案対象）
- **理由**: 自動メモリーレス化はリファクタリングでどこかに`UseTexture`が紛れ込むと黙って外れ、メモリ使用量と帯域が増えたことに気づけない。明示指定なら矛盾がその時点で顕在化する（原稿）。削減量の目安は1080pでRGBA8中間バッファ1枚約7.9MB、RGBA16F約15.9MB、G-Buffer 3枚＋深度で約31.7MB（原稿試算）
- **対処**: 「このリソースはタイルメモリ内で完結させる」という設計意図があるなら`desc.memoryless = RenderTextureMemoryless.Color`等を明示してコードに残す。成立条件はロードがClear/DontCare・ストアがDontCare・利用が1つのNative Render Pass内で完結の3つ

### RP23 Rasterパス列へのComputePass/UnsafePass挿入によるマージ分断
> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
- **レベル**: Info
- **対象**: コードレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: マージさせたいRasterRenderPassの列の途中に`AddComputePass`や`AddUnsafePass`が記録され、Native Render Passがそこで切れている
- **grep_pattern**: `AddComputePass\s*[<(]`（Rasterパス列の間に挟まる記録順かは手動確認。Render Graph ViewerのPass Break Reasoningで裏取り）
- **理由**: マージされるのはRaster Render Pass同士かつ連続して記録されたパスのみで、間にComputeパスが挟まるとそこでNative Render Passが切れる（原稿）。分断点では中間バッファのStore→Load往復が発生する（1080p RGBA8で往復約16MB/フレーム。原稿試算）
- **対処**: Compute処理をRasterパス列の前後（フレームの先頭・末尾側）に移動して記録順を入れ替え、マージさせたい区間を連続させる。マージ成否はFrame Debuggerの`(RP n:m)`表記とRender Graph Viewerで確認

### RP24 フレームバッファフェッチマクロのXR非対応バリアント使用
> 出典: UnityPerformanceTuningBible 2026 - Render Graph タイルメモリ最適化
- **レベル**: Info
- **対象**: シェーダーレビュー
- **適用条件**: urp, unity>=6000.0
- **検出パターン**: XR対応の可能性があるプロジェクトで`FRAMEBUFFER_INPUT_HALF`/`FRAMEBUFFER_INPUT_FLOAT`（`_X`なし）を使用している
- **grep_pattern**: `FRAMEBUFFER_INPUT_(HALF|FLOAT)\s*\(|LOAD_FRAMEBUFFER_INPUT\s*\(`（`_X`付きは別マクロ名のため非該当）
- **理由**: `_X`付きバリエーションは`TEXTURE2D_X`系と同じくXR（シングルパスインスタンシング）のレンダーターゲット配列に対応したもので、原稿はXRの可能性があるなら`_X`付きで統一しておくと安全としている（性能数値の言及はなし。移植安全性のルール）
- **対処**: `FRAMEBUFFER_INPUT_X_HALF(n)` / `LOAD_FRAMEBUFFER_X_INPUT(n, pos)`へ統一する
