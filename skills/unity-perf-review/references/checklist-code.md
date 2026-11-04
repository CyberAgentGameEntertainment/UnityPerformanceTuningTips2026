# レビューチェックリスト: コード（C#/シェーダーコード）

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
| AN02 | Critical | - | `SetTimeUpdateMode\s*\(\s*DirectorUpdateMode\.Manual\s*\)` | 手動更新Playable Graphの1体1グラフ構成 | rules-animation.md |
| AN03 | Critical | - | `[Aa]nimator\w*\.Update\s*\(` | Animator.Update()の明示呼び出しによる間引き | rules-animation.md |
| AN04 | Critical | - | `[Aa]nimator\w*\.enabled\s*=` | Animator無効化間引きでkeepAnimatorStateOnDisable未設定 | rules-animation.md |
| AN05 | Warning | - | `%\s*\w*[Ii]nterval` | 間引き実装に位相ずらし（Slot）がない | rules-animation.md |
| AN06 | Warning | - | `void\s+LateUpdate\s*\(` | 大量キャラクターのプロシージャル処理（IK等）をLateUpdateで実装 | rules-animation.md |
| AN07 | Info | - | `\[BurstCompile\]` | [BurstCompile]を付けただけのSIMD高速化期待 | rules-animation.md |
| AN08 | Warning | - | `extraExposedTransformPaths: \[\]` | Optimize Game Objects適用対象でボーン階層に依存するコード | rules-animation.md |
| AN09 | Warning | unity>=6000.3 | `(ObjectPool<\|IObjectPool<)` | プール返却時のAnimator状態リセット漏れ | rules-animation.md |
| AN10 | Warning | unity>=6000.3 | `ResetControllerState\(` | ResetControllerState対象外プロパティの戻し漏れ | rules-animation.md |
| AN11 | Info | unity>=6000.3 | MANUAL | リセット直後のStateInfo即時参照 | rules-animation.md |
| AN14 | Warning | vat | MANUAL | VAT Shader Graphテンプレートの直接利用・Sub Graph参照の挿げ替え漏れ | rules-animation.md |
| BD12 | Warning | il2cpp | `Type\.GetType\(\|Activator\.CreateInstance\(\|\.GetMethod\(` | リフレクション経由でのみ参照される型・メンバーの保持指定漏れ | rules-build-stripping.md |
| BD13 | Warning | il2cpp | `\.Register<\|\.Bind<\|\.Resolve<` | DIコンテナー登録型のデフォルトコンストラクタ削除 | rules-build-stripping.md |
| BD14 | Critical | il2cpp | `AssetBundle\.LoadFrom\w*\(` | AssetBundle専用コンテンツが参照する型・アセンブリの保持漏れ | rules-build-stripping.md |
| BD15 | Info | il2cpp | `<assembly[^>]*preserve="all"` | link.xmlのアセンブリ全体preserve="all"の恒久運用 | rules-build-stripping.md |
| BD16 | Warning | il2cpp | `\[Preserve\]` | [Preserve]属性の付与位置と保持範囲の不一致 | rules-build-stripping.md |
| BD03 | Critical | il2cpp | `SetAdditionalIl2CppArgs` | SetAdditionalIl2CppArgsの上書き（退避・復元なし） | rules-build.md |
| BD05 | Critical | il2cpp | `-flto(?!=thin)` | Full LTOの指定 | rules-build.md |
| BD06 | Warning | il2cpp, android | `compiler-flags.*-O3` | -O3 + ThinLTOの併用 | rules-build.md |
| BD07 | Info | il2cpp | `-f(whole-program-vtables\|split-lto-unit)` | -fwhole-program-vtables（whole-program devirtualization）の指定 | rules-build.md |
| BD08 | Info | il2cpp | `import-instr-limit=\d+` | -import-instr-limitのチューニング | rules-build.md |
| CS01 | Critical | - | `using\s+System\.Linq\|\.Where\(\|\.Select\(\|\.ToList\(\|\.ToArray\(\|\.OrderBy\(\|\.Any\(\|\.First\(\|\.Count\(` | ホットパスでのLINQ使用 | rules-csharp.md |
| CS02 | Critical | - | `\$"\|string\.Format\(\|string\.Concat\(\|\w\s*\+\s*"\|"\s*\+\s*\w` | 文字列連結・補間 | rules-csharp.md |
| CS03 | Warning | - | `async\s+(Task\|ValueTask)` | async/awaitのGC | rules-csharp.md |
| CS04 | Info | - | `\bclass\s+\w+[^{]*\{` | sealed修飾子の欠如 | rules-csharp.md |
| CS05 | Critical | - | `\(object\)\s*\w+` | 構造体のボクシング | rules-csharp.md |
| CS06 | Critical | - | `=>\s*\{\|=>\s*[^{]` | クロージャキャプチャ | rules-csharp.md |
| CS07 | Warning | - | `\bparams\s+\w+\[\]` | params配列 | rules-csharp.md |
| CS08 | Warning | - | `new\s+(Dictionary\|List\|HashSet)<[^>]+>\(\s*\)` | Dictionary/Listの初期容量未指定 | rules-csharp.md |
| CS09 | Warning | - | `\bforeach\s*\(` | foreachによるアロケーション | rules-csharp.md |
| CS10 | Info | - | `new\s+(byte\|int\|float\|char\|double)\[\s*\d+\s*\]` | stackalloc未活用 | rules-csharp.md |
| CS11 | Warning | - | `static\s+(readonly\s+)?(List\|Dictionary\|HashSet)<` | 静的フィールドの不適切な使用 | rules-csharp.md |
| CS12 | Warning | - | `throw\s+new\s+\w+Exception` | 例外スローの多用 | rules-csharp.md |
| CS13 | Warning | - | `\.ToString\(\)` | 不要なToString() | rules-csharp.md |
| CS14 | Warning | - | `\benum\b[^{]*\.ToString\(\|\.HasFlag\(` | Enum操作のボクシング | rules-csharp.md |
| CS15 | Info | - | `\bstruct\s+\w+` | 構造体のコピーコスト | rules-csharp.md |
| GR09 | Warning | - | `[Cc]amera\w*\.enabled\s*=\s*true` | 不要なカメラ描画 | rules-graphics.md |
| GR14 | Warning | - | `new\s+RenderTexture\(\|RenderTexture\.GetTemporary\(` | RenderTexture解像度過大 | rules-graphics.md |
| GR15 | Warning | - | `tex2D\(\|SAMPLE_TEXTURE2D\(\|SAMPLE_TEXTURE2D_LOD\(` | シェーダーのフラグメント処理過剰 | rules-graphics.md |
| LT01 | Warning | - | `new CancellationTokenSource\(` | CancellationTokenSourceのDispose漏れ | rules-lifecycle.md |
| LT02 | Warning | - | `CreateLinkedTokenSource\(` | CreateLinkedTokenSourceの戻り値のDispose漏れ | rules-lifecycle.md |
| LT03 | Warning | - | `[Tt]oken\.Register\(` | CancellationToken.Register登録の解除漏れ | rules-lifecycle.md |
| LT04 | Warning | - | `(AssetBundle\.LoadFrom\|Addressables\.(LoadAssetAsync\|InstantiateAsync\|LoadSceneAsync))` | 動的ロードの窓口未集約 | rules-lifecycle.md |
| LT05 | Warning | - | MANUAL | ロード時のライフタイム未決定 | rules-lifecycle.md |
| LT06 | Warning | - | MANUAL | 非同期ロード完了前のライフタイム終了への未対応 | rules-lifecycle.md |
| LT07 | Warning | - | MANUAL | GameObjectスコープの破棄後Add未対応 | rules-lifecycle.md |
| PH04 | Warning | - | `Physics\.Raycast\(\|Physics\.SphereCast\(\|Physics\.BoxCast\(` | Physics.Raycast毎フレーム多発 | rules-physics.md |
| PH05 | Critical | - | `Physics\.RaycastAll\(\|Physics\.OverlapSphere\(\|Physics\.OverlapBox\(\|Physics\.SphereCastAll\(` | RaycastのGCアロケーション | rules-physics.md |
| PH06 | Warning | - | `Physics\.(Raycast\|SphereCast\|BoxCast\|OverlapSphere\|OverlapBox)\(` | レイヤーマスク未指定 | rules-physics.md |
| PH08 | Info | - | `m_IsKinematic:\s*0\|isKinematic\s*=\s*false` | Rigidbody.isKinematic未活用 | rules-physics.md |
| PH11 | Warning | - | `void\s+FixedUpdate\s*\(` | FixedUpdate内の重い処理 | rules-physics.md |
| PH12 | Warning | - | `\.MovePosition\(\|\.MoveRotation\(` | Rigidbody.MovePositionの誤用 | rules-physics.md |
| PH13 | Info | - | `Collider2D\|BoxCollider2D\|CircleCollider2D\|PolygonCollider2D` | Physics2Dと3Dの混在 | rules-physics.md |
| PH14 | Info | - | `\.contacts\b\|collision\.contacts` | ContactPoint取得の無駄 | rules-physics.md |
| RP15 | Warning | urp, unity>=6000.0 | `SetRenderAttachment\([^)]*AccessFlags\.Write\b` | 全画面上書きパスのAccessFlags.Write申告（WriteAll未使用） | rules-rendering-tilememory.md |
| RP16 | Warning | urp, unity>=6000.0 | `\.UseTexture\(` | 同一ピクセル読みへのUseTexture使用（SetInputAttachment/FBF未活用） | rules-rendering-tilememory.md |
| RP17 | Warning | urp, unity>=6000.0 | `SetGlobalTextureAfterPass\s*\(` | SetGlobalTextureAfterPassによるRender Graph追跡の切断 | rules-rendering-tilememory.md |
| RP18 | Warning | urp, unity>=6000.0 | `ImportTexture\(\s*[^,)]+\s*\)` | ImportTextureのImportResourceParams未指定 | rules-rendering-tilememory.md |
| RP19 | Warning | urp, unity>=6000.0 | `new\s+TextureDesc\s*\(` | 中間バッファTextureDescの手組みによるマージ条件不一致 | rules-rendering-tilememory.md |
| RP21 | Warning | urp, unity>=6000.0 | MANUAL | MSAAのResolve/メモリーレス構成の破壊 | rules-rendering-tilememory.md |
| RP22 | Info | urp, unity>=6000.0 | ABSENT `\.memoryless\s*=\|RenderTextureMemoryless` | タイルメモリ内完結リソースのmemoryless明示指定の欠如 | rules-rendering-tilememory.md |
| RP23 | Info | urp, unity>=6000.0 | `AddComputePass\s*[<(]` | Rasterパス列へのComputePass/UnsafePass挿入によるマージ分断 | rules-rendering-tilememory.md |
| RP24 | Info | urp, unity>=6000.0 | `FRAMEBUFFER_INPUT_(HALF\|FLOAT)\s*\(\|LOAD_FRAMEBUFFER_INPUT\s*\(` | フレームバッファフェッチマクロのXR非対応バリアント使用 | rules-rendering-tilememory.md |
| RP01 | Warning | urp, unity>=6000.0 | `SetRenderTarget\(\s*[^,)]+\s*\)\s*;` | UnsafePass内SetRenderTargetのLoad/Store Action未指定 | rules-rendering.md |
| RP02 | Critical | urp, unity>=6000.0 | `AddUnsafePass` | UnsafePassでのUseTexture依存関係宣言漏れ | rules-rendering.md |
| RP03 | Info | urp, unity>=6000.0 | `AddRasterRenderPass` | ループ内でのRasterRenderPass大量登録（マージ不能Passチェーン） | rules-rendering.md |
| RP04 | Info | urp, unity>=6000.0 | `AddUnsafePass` | 宣言的APIで表現できる処理へのUnsafePass使用 | rules-rendering.md |
| RP05 | Info | urp | `RenderPassEvent\.(AfterRenderingPostProcessing\|AfterRendering)\b` | AfterRenderingPostProcessing以降のカスタムパスによる冗長FinalBlitPass | rules-rendering.md |
| RP06 | Critical | urp, unity>=6000.3 | `SwitchActiveTexturesToBackbuffer` | SwitchActiveTexturesToBackbufferの呼び出し位置ガード欠如 | rules-rendering.md |
| RP07 | Critical | urp, unity>=6000.3 | `SwitchActiveTexturesToBackbuffer` | FinalBlit省略時のY反転判定の欠如・誤実装 | rules-rendering.md |
| RP08 | Critical | urp, unity>=6000.3 | ABSENT `LinearToSRGBConversion\|_LINEAR_TO_SRGB_CONVERSION` | FinalBlit省略時のsRGB変換の実装漏れ | rules-rendering.md |
| RP09 | Warning | unity>=6000.3 | `new Material\s*\(\|\.material\s*[=.]` | 大量描画でのRendererごとのマテリアル複製 | rules-rendering.md |
| RP10 | Warning | srp | `MaterialPropertyBlock\|SetPropertyBlock` | MaterialPropertyBlockによるSRP Batcher / GPU Resident Drawer非互換 | rules-rendering.md |
| RP11 | Info | - | `new GraphicsBuffer\(` | GraphicsBufferの更新頻度・解放管理 | rules-rendering.md |
| RP12 | Warning | ios\|android | ABSENT `QualitySettings\.SetQualityLevel` | 描画品質切り替え機構の欠如（全端末一律設定） | rules-rendering.md |
| RP13 | Warning | android | `SystemInfo\.(deviceModel\|graphicsDeviceName)` | Android端末のスペック判定を機種名文字列比較のみで実施 | rules-rendering.md |
| RP14 | Info | urp | `class\s+\w+\s*:\s*ScriptableRendererFeature` | 自作RendererFeatureに品質切り替えAPIがない | rules-rendering.md |
| SH01 | Warning | unity>=6000.0 | `ShaderVariantCollection` | ShaderVariantCollectionによるウォームアップ（モダンAPI環境） | rules-shader.md |
| SH02 | Warning | unity>=6000.0 | `GraphicsStateCollection` | GSCの一括WarmUpによるスパイク | rules-shader.md |
| SH03 | Warning | unity>=6000.0 | `isWarmedUp` | isWarmedUpによる完了待ちループ | rules-shader.md |
| SH06 | Warning | unity>=6000.0, ios | `BeginSubPass\s*\(` | SubPass内シェーダーのウォームアップ失敗（iOS/Metal） | rules-shader.md |
| SH07 | Info | unity>=6000.0 | `UnityEngine\.Experimental\.Rendering` | Experimental APIへの依存（GSC） | rules-shader.md |
| SH08 | Warning | unity>=6000.0 | `QualitySettings\.SetQualityLevel` | Quality Settings非対応のGSC収集 | rules-shader.md |
| SH09 | Warning | - | `\b(sin\|cos\|tan\|asin\|acos\|atan\|exp\|exp2\|pow\|log\|log2\|log10)\s*\(` | フラグメントシェーダー内の超越関数多用 | rules-shader.md |
| SH10 | Info | - | `\bsin\s*\(` | sin関数の放物線近似への置き換え余地 | rules-shader.md |
| SH11 | Warning | - | `\b(tan\|atan\|atan2)\s*\(` | tan/atan関数の使用（超越関数中で最重量級） | rules-shader.md |
| SH12 | Info | - | `\bpow\s*\(.+,\s*_\w+` | 定数指数のpowをuniform経由で書いている | rules-shader.md |
| SH13 | Warning | srp | `CBUFFER_START\(UnityPerMaterial\)` | グローバル値のUnityPerMaterial混入 | rules-shader.md |
| SH14 | Info | - | `UNITY_LOOP\|\[loop\]` | 動的ループ使用シェーダーのシェーダーモデル指定漏れ | rules-shader.md |
| UI01 | Critical | - | `^Canvas:` | Canvas未分割 | rules-ui.md |
| UI02 | Warning | - | `m_RaycastTarget:\s*1` | Raycast Target不要な有効化 | rules-ui.md |
| UI03 | Critical | - | `HorizontalLayoutGroup\|VerticalLayoutGroup\|GridLayoutGroup` | Layout Group毎フレーム再計算 | rules-ui.md |
| UI04 | Warning | - | `UnityEngine\.UI\.Text\b\|\bText\s+\w+` | Legacy Text使用 | rules-ui.md |
| UI05 | Critical | - | `ScrollRect` | ScrollViewの大量要素 | rules-ui.md |
| UI07 | Warning | - | `\.SetActive\(\|\.gameObject\.SetActive\(` | SetActive頻繁切り替え | rules-ui.md |
| UI08 | Warning | - | `void\s+OnGUI\s*\(` | ランタイムゲームUIでのIMGUI（OnGUI）使用 | rules-ui.md |
| UI09 | Info | - | `class\s+\w+\s*:\s*EditorWindow` | 常時再描画エディターウィンドウでのIMGUI多用 | rules-ui.md |
| UI10 | Critical | - | `UIDocument\|PanelRenderer` | uGUIとUI Toolkitの重ね合わせ設計 | rules-ui.md |
| UI11 | Warning | - | `override\s+VisualElement\s+CreatePropertyGUI` | UI Toolkit版のみのPropertyDrawer | rules-ui.md |
| US01 | Critical | - | `void\s+(Update\|LateUpdate\|FixedUpdate\|OnGUI)\s*\(\s*\)\s*\{\s*\}` | 空のUnityイベント関数 | rules-unity-script.md |
| US02 | Critical | - | `GetComponent<\|GetComponentInChildren<\|GetComponentInParent<\|GetComponents<` | GetComponent毎フレーム呼び出し | rules-unity-script.md |
| US03 | Critical | - | `GameObject\.Find\(\|Find(ObjectOfType\|ObjectsOfType\|AnyObjectByType\|ObjectsByType)\s*[<(]` | Find系メソッドのランタイム使用 | rules-unity-script.md |
| US04 | Warning | - | `\.tag\s*[!=]=` | CompareTag未使用 | rules-unity-script.md |
| US05 | Critical | - | `Debug\.Log\(\|Debug\.LogWarning\(\|Debug\.LogError\(\|Debug\.LogException\(` | Debug.Logのリリースビルド残留 | rules-unity-script.md |
| US06 | Warning | - | `\bInstantiate\(\|\bDestroy\(\|\bDestroyImmediate\(` | Instantiate/Destroyの頻繁な使用 | rules-unity-script.md |
| US07 | Warning | - | `StartCoroutine\(\|yield\s+return\s+new` | Coroutineの多用 | rules-unity-script.md |
| US09 | Critical | - | `SendMessage\(\|BroadcastMessage\(\|SendMessageUpwards\(` | SendMessageの使用 | rules-unity-script.md |
| US10 | Warning | - | `\.(SetFloat\|SetBool\|SetTrigger\|SetInteger\|GetFloat\|GetBool\|GetInteger\|ResetTrigger\|Play\|CrossFade)\(\s*"` | アニメーション文字列パラメータ | rules-unity-script.md |
| US11 | Warning+ | - | `\.Simulate\(` | ParticleSystem.Simulateへの大きな秒数指定 | rules-unity-script.md |
