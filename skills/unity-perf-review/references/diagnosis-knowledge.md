# プロファイル診断ナレッジ集

> 出典: UnityPerformanceTuningBible 2026。profile-analyzer / profile-diagnoser がプロファイルデータの解釈時に参照する「症状 → 原因 → 対処」のパターン集。

## アニメーション / スキニング

| 症状（Profilerマーカー等） | 原因 | 対処 |
|---|---|---|
| メインスレッドの`MeshSkinning.Update` / 全スレッドの`MeshSkinning.CalcMatrices`が大きい | ボーンのTransform階層が実体化しており、スキニング行列計算とTransform書き戻しが走っている | Optimize Game Objectsまたは`AnimatorUtility.OptimizeTransformHierarchy`で階層フラット化（CalcMatricesはゼロになり得る）。ただしアニメーション評価自体（クリップサンプリング・ブレンド）には効かない [AN01] |
| 手動更新Playableでメインスレッドの`Evaluate`が大きく、Jobワーカーの大半がIdle | グラフ粒度が細かすぎる（1体1グラフの直列評価） | グラフ集約。健全な状態では`Animator.ProcessGraph`が全ワーカーにほぼ均等に載る [AN02] |
| 間引きLOD導入後もフレーム時間がほぼ減らない | `Animator.Update()`明示呼び出しによる同期評価でジョブ並列化を喪失（6.7 → 15.4µs/体） | `Animator.enabled`切り替えor位相別手動グラフに変更 [AN03] |
| CPUチャートが「ほぼ0msの谷 + 約10msのスパイク」の櫛状 | 間引きの位相ずらし忘れ（全キャラ同一フレーム更新） | Slot（`index % interval`）で位相分散。判断は中央値でなくp95/p99/標準偏差で行う [AN05] |
| アニメーション数比例でCPU時間が伸び、`SRPBatcher.Flush`（ドローコール送信）が支配的 | パーツごとに個別Rendererを持つ構成でカリング・描画対象計算がパーツ数分発生 | 1メッシュ統合（実測でFlush 76.17ms → 0.02ms）。さらにVAT化でAnimator更新もGPUへ（4.54ms → 0.02ms） [AN12] |
| VAT化後にGPU時間がSkin方式より悪い | VATはシャドウ・デプス等パスのたびに頂点シェーダーで変形を再計算（GPU Skinningはフレーム1回で全パス再利用） | 頂点計算パス数が少ない構成ではVAT有利。パス数の多い構成ではSkin方式と比較検討（実測: シャドウ4カスケードでSkin 7.4ms対VAT 16.0ms） |
| プールから取り出したキャラのアニメが前回の状態から始まる | State・parameter（特にTrigger残留）の持ち越し | 返却時に`ResetControllerState(true)`（Unity 6.3+）。Layer Weight/speed/StateMachineBehaviour独自変数は別途戻す [AN09-AN10] |

**知見**:
- 「Playable APIはAnimatorより軽い」はシンプルな制御では不成立（600体で5.004ms対5.093ms、ほぼ同コスト）。Playableの価値は最適化ではなくAnimation Jobの差し込み・グラフ制御の柔軟性
- `AnimationScriptPlayable`にはノード固定コストがある（300体で約2.16ms）。演算が軽いうちはLateUpdateの方が総CPU時間は少ない
- Profiler接続時のキャプチャは非接続計測よりわずかに重く出る。合否判定の数値はProfiler非接続で取る

## 描画律速の見極め

| 症状 | 原因 | 対処 |
|---|---|---|
| CPU処理を削ったのにフレーム時間が変わらず、`Semaphore.WaitForSignal` / `Gfx.WaitForPresentOnGfxThread`が伸びる | レンダースレッド側がクリティカルパス（描画律速）の可能性。ただし待機マーカーは一般的な同期点でGPU待ち・VSync待ちも含むため、Timelineビューで待ち先を確認して確定する | 描画律速と確認できたらCPU側最適化では改善しない。描画側を先に削る。CPU削減効果は描画負荷を下げた構成で初めてフレーム時間に現れる |
| Render Graph関連CPUマーカーの合計時間がPass数に比例して増加 | マージできないPassチェーンをRasterRenderPassとして多数登録 | 1つのUnsafePassへ統合（1Passあたり約1.8µs削減、Editor測定） [RP03] |
| Frame Debugger/Render Graph Viewer末尾に単純コピーのフルスクリーンBlit（FinalBlitPass）が1回入っている | ポストプロセス無効で中間テクスチャが存在 / ポストプロセス後にカスタムパスがある / キャプチャ処理がある | Unity 6.3の`SwitchActiveTexturesToBackbuffer()`で省略を検討（Y反転・sRGB変換の肩代わりが必要） [RP05-RP08] |
| 大量描画シーンで毎フレームのパラメータ更新時にFPSが激減し、GPU Resident Drawer有効時にさらに悪化 | マテリアルパラメータの大量更新はマテリアル全体のGPU再アップロードを伴う（実測: 11.1 FPS → GRDありで8.1 FPS） | Renderer Shader User Value（Unity 6.3+）へ移行 [RP09-RP10] |

## シェーダー / PSOスタッター

初回描画スタッターの切り分け観測点:
- Main Thread `Shader.CreateGPUProgram` = シェーダープログラム生成
- Render Thread `GpuProgramMetal.GetCachedPipeline`（iOS）/ `CreateGraphicsPipelineImpl`（Android）= PSO生成（Metal/VulkanではこちらがスタッターSの主因。Vulkan実例で1回数十ms）
- 合否判定: プレイ中フレームにPSO生成マーカーが1件でも出たらウォームアップ漏れ

| 症状 | 原因 | 対処 |
|---|---|---|
| GSCウォームアップ時に`variant not found`エラー、ランタイムでも該当バリアントのコンパイルなし | Shader Strippingで該当バリアントが除外されている | Graphics > Shader Strippingを確認、バリアントに影響する変更後はGSC再収集 |
| 同じく`variant not found`だがランタイムではコンパイル発生 | GSCにバリアントは記録されているがGraphicsStateが0件 | 収集シナリオで実際に描画されるよう修正 |
| `WarmUp may be inaccurate due to ... mismatch`警告 | GSCメタデータと実行環境の不一致（別プラットフォーム収集の流用等） | 該当プラットフォームの実機で収集し直す [SH04] |
| iOSのみ: エラーなし・`Shader.CreateGPUProgram`なしで`GpuProgramMetal.GetCachedPipeline`が発生 | AssetBundle重複パッキングによるシェーダー二重インスタンス | シェーダー専用バンドル化、またはバンドルごとにGSC用意 [SH05] |
| iOSのみ: ウォームアップ完了・エラーなしなのにプレイ中PSOコンパイル | SubPass（Native Render Pass）内描画シェーダーのウォームアップ失敗 | SubPass見直しまたはロード中のプリレンダリング [SH06] |
| 超越関数を実行しないパス（分岐で通らない・ループ0回）でもGPU時間が増加 | レジスタプレッシャー（関数が存在するだけで消費レジスタが増えスレッド占有数が減る。実測: sin存在だけで旧世代端末+0.5ms） | 未使用の超越関数コードをバリアント分割で生成コードから除去 [SH09] |
| 近似計算への置き換え効果が実行回数の少ない場面で出ない | 少数回領域ではレジスタプレッシャーのオーバーヘッドが支配的（旧世代端末はsin4回まで増加ほぼなし） | 近似置き換えは呼び出し回数の多いホットスポットに適用 |

**GSC運用**: トレースは必ずビルドしたアプリを実機で実行して収集。ウォームアップはシェーダーアセットのロード後に実行。ParticleSystemは`Simulate(t, withChildren: true, restart: true)`で再生途中状態を同期再現すると収集を効率化できる。

## メモリ / ライフタイム

| 症状 | 原因 | 対処 |
|---|---|---|
| 画面/シーン遷移後もアセットが残る | ライフタイム管理の不備（解放漏れ・窓口未集約） | Memory Profilerで遷移前後のスナップショット比較。同じ遷移を複数回繰り返して増え続けないか確認。「残っている＝解放漏れ」と即断せず参照元まで確認。最終確認は実機プレイヤービルド [LT04-LT07] |
| MonoBehaviour破棄後も非同期処理が待機し続ける/参照が残る | CancellationTokenSourceのライフタイム管理不備（特にCreateLinkedTokenSourceのDispose漏れによる長寿命CTSからの参照連鎖） | CTS/linkedCtsのusing管理 [LT01-LT03] |
| パーティクル再生開始時（特に途中時点からの再生）に単発の巨大スパイク | `ParticleSystem.Simulate`の一括シミュレーションでNoiseJobが大量コール（実測: 20秒指定でNoiseJob約2000回・約50ms） | Profiler.BeginSample/EndSampleでSimulate呼び出しを囲んで切り分け。Noise使用の有無と指定秒数を確認 [US11] |

## ビルド / バイナリ

| 症状 | 原因 | 対処 |
|---|---|---|
| Player SettingsにLink Time Optimizationトグルが表示されない | Development Buildにチェックが入っている | 非Developmentビルドで確認 |
| LTOトグルを有効にしたのに効果が全く出ない | 3条件（IL2CPP / Strip Engine Code無効 / 非Development）未達でサイレントにNoneフォールバック | `Library/Bee/Player*-inputdata.json`のgrepとAPK内libunity.soのmd5照合 [BD11] |
| 手動ThinLTOが効いているか不明 | フラグがコンパイル/リンクの片方にしか渡っていない可能性 | `Player*.dag.json`で両方に`-flto=thin`を確認、IL2CPP生成.oの先頭4バイト（bitcode `4243 c0de`）で確認 |
| .soサイズ増 = 常駐メモリ増と見積もってしまう | .soマッピングの常駐増の大半はPrivate Clean（逼迫時破棄可能） | `dumpsys meminfo`でPrivate Dirty / Cleanを分けて評価 |
| 数%のCPU改善をA/B検証したいがCPU時間(ms)では差が出ない | セッション平均周波数がビルドと無関係に約5%振れ、効果が埋もれる | Android: `/proc/<pid>/task/*/time_in_state`からサイクル数で判定。再インストール有無でART/dexopt状態が変わるため毎セッション再インストールで統一 |
| interface/virtual呼び出しがホットパスを支配しLTOの効きが悪い | IL2CPPのinterface呼び出しは実行時メソッドテーブル経由の間接呼び出しで、どのフラグでも直接化できない | C#側で対処: sealed、具象型受け、ジェネリクスによる静的ディスパッチ |
| iOSで配布署名ipaのCPUプロファイルが取れない | `get-task-allow=false`のためxctraceアタッチ不可 | InstrumentsのActivity Monitorテンプレート（`sysmon-process`、アタッチ不要） |

**LTO費用対効果の見積もり**: LTOが速くするのは`libil2cpp.so`の中だけ（レンダースレッド・GPU・エンジン内部・GCは射程外）。Profilerのreleaseキャプチャで PlayerLoop 内スクリプティングmsを取り、フレーム-1%未満の見込みならビルド時間+9〜15%に見合わない。

## コードストリッピング

| 症状 | 原因 | 対処 |
|---|---|---|
| エディター・Monoビルドでは動くのに、ビルドしたアプリだけで特定機能が壊れる（Stripping Levelを上げた直後に発生） | Managed Code Strippingが静的解析でrootから到達できないコードを削除（実行時に使われるかは判定に無関係） | Mono.Cecilでストリップ後アセンブリの型・メソッド一覧をダンプしLevel違いの2ビルドで差分比較。保持指定（link.xml / [Preserve] / Require系属性）を追加 [BD12] |
| ビルドしたアプリでのみ`Type.GetType`がnullを返す | リフレクションのみで参照される型が型ごと削除された。UnityLinkerは定数文字列の`Type.GetType`は追跡するが、フィールド・設定・サーバー応答経由の文字列は追跡しない | link.xmlまたは[Preserve]で保持。恒久対応はSource Generatorで静的参照化 [BD12] |
| ビルドしたアプリでのみ`MissingMethodException: Default constructor not found`（DIのResolve時等） | 型は静的参照（型引数等）で残るが、リフレクションでしか呼ばれない暗黙のデフォルトコンストラクタがメンバー単位で削除された | コンストラクタまで保持指定するか、Source Generator注入（例: VContainer）へ切り替え [BD13, BD16] |
| AssetBundleのアセットロード時に「The referenced script (X) on this Behaviour is missing!」。Stripping Levelを変えても結果が同じ | AssetBundle側コンテンツだけが必要とする型はrootにならず、アセンブリごとビルドから消える（実測: Unity.Timeline.dllがMinimalでも存在しない）。Levelの調整では直らない | link.xmlで明示保持、Addressablesの自動生成link.xml、またはIUnityLinkerProcessorでビルド時生成 [BD14] |
| [Preserve]を付けたのにリフレクション呼び出しが動かない | 付与位置により保持範囲が異なる（型に付与→型+デフォルトコンストラクタのみ、メソッドに付与→型+そのメソッドのみ） | 生成して呼び出すには型とメソッドの両方に付与。パターン保持は[RequireImplementors]/[RequireDerived] [BD16] |
| どの型・メソッドが削られたか特定したい | - | Mono.Cecilでビルド出力のDLL（またはLibrary/Bee/artifacts/<target>/ManagedStripped/の中間生成物。非公式パスのため無ければビルド出力側）から型とメソッド（.ctor含む）一覧を書き出し、2ビルドの差分を見る |

**ストリッピングの知見**:
- 削減効果の目安（シンプルなシーン、Minimal→High実測）: マネージドアセンブリ合計 7,702,016→7,188,480 bytes（約6.7%減）、アプリ全体 89,069,307→88,548,299 bytes（約0.6%減）。System.Runtime.Serialization.dllが丸ごと削除、mscorlib.dll約272KB減、Assembly-CSharp.dll 22.5%減。削減量は参照ライブラリ量と未使用コード割合で決まり、ライブラリの多い大規模プロジェクトほど恩恵が大きい
- IL2CPPビルドではマネージド削減分だけC++変換・コンパイル対象も減り、ネイティブバイナリサイズとビルド時間にも効果が波及する
- IL2CPPビルドの既定はMinimal（Disabled選択不可＝必ずストリッピングされる）、Monoビルドの既定はDisabled
- UnityLinkerのmarkはメンバー単位: typeofで型だけ参照した場合、型は残るが未呼び出しメソッドは個別に削除される。typeofで得たTypeへ直接GetMethodを呼ぶと、その型のpublicメソッドが一括保持される
- 保持手段の使い分け: [Preserve]=自コードの少数要素 / link.xml=編集できないDLL含む決まった対象 / Require系属性=フレームワークの契約宣言 / IUnityLinkerProcessor=ターゲット・コンテンツで保持対象が変わる場合 / Source Generator=リフレクション探索自体の静的化

## タイルメモリ / メモリ帯域

| 症状 | 原因 | 対処 |
|---|---|---|
| 最初の数分は60FPSで動くが、遊んでいるうちにフレームレートが落ちてくる | メモリ帯域の消費→消費電力増→発熱→サーマルスロットリング→クロック低下、の連鎖。モバイルSoCはLPDDRをCPU/ISP等と共有し帯域が細い | ロード/ストア削減（AccessFlags申告・パスマージ・FBF・メモリーレス）で帯域を削る。まずRender Graph Viewerで各パスのアタッチメント×アクションを列挙し「解像度×バイト数」で1フレームの往復MBを試算して優先順位を付ける（1080p RGBA8で1回約7.9MB、60FPS換算約475MB/s） [RP15-RP23] |
| エディタのFrame Debuggerで`(RP n:m)`のマージがまったく観察できない | Native Render PassのマージはVulkan/Metal環境で機能する。エディタのGraphics APIがD3D等だと観察不可 | エディタのGraphics APIをVulkanに変更するか、実機ビルドにFrame Debuggerをアタッチする |
| マージされるはずのパスがマージされない | マージ条件不成立: 非Rasterパス（Compute/Unsafe）の挟み込み、記録が非連続、解像度・MSAAサンプル数不一致、前パス出力のUseTexture利用、アタッチメント合計8枚超、グローバルテクスチャ化 | Render Graph ViewerのPass Break Reasoningを最初に確認する [RP16, RP17, RP19, RP23] |
| FBFマクロを使ったのに帯域が減らない／エディタでは正しく描画されるのに実機と差が出る | 非対応環境（エディタD3D、GL ES拡張非対応GPU）ではマクロが通常のテクスチャ読み（`_UnityFBInput0`等）にfallbackし、動作はするが帯域削減効果はない | Render Graph Viewerで入力側バッファのStoreがDontCareになっているかを確認し、実機で帯域を計測する。エディタでは効果測定はできない前提でワークフローを組む |
| Adreno端末でタイルメモリ最適化の効果が出たり出なかったりする | Adrenoは描画内容によってダイレクトモード（IMR的動作）へ切り替わることがあり、タイルメモリ前提の最適化効果が変わる | プロファイラでどちらのモードで動いているかを確認する |
| TBR端末でハイポリゴンシーンが想定以上に遅い | ビニングフェーズ自体がタイルごとのプリミティブリストをメインメモリへ読み書きし、頂点数に比例して帯域を消費する | 頂点数の削減。フラグメント側だけでなくジオメトリ量も帯域要因として見積もる |
| シャドウマップ等のStoreを削ろうとして描画が壊れる | 「描いて後でサンプリングする」リソースのStoreは削れない正当なストア | Storeごとに「後続の利用」という正当な理由があるかを1つずつ確認する。理由があるStoreは残し、無いStore（未使用Depth Texture等）だけ削る [RP20] |

**タイルメモリの知見**:
- 帯域計測の基準ベンダーはARM Mali（外部帯域カウンタが充実）。FBF/メモリーレスはApple/Metalが安定して機能するため、iOSで効果を確認してからAndroid検証に進む手順も有効
- Imagination（Pixel 10のIMG DXT等）はハードウェア隠面除去（HSR）を持つTBDRで不透明オーバードローが元々少なく、最適化の効きどころが他ベンダーと異なる。新しいドライバのため挙動差の検証を推奨
- 帯域の概算式: 1回のロード/ストア = 横×縦×1ピクセルのバイト数。1080pの目安はRGBA8/B10G11R11/D32が約7.9MB（60FPSで約475MB/s）、RGBA16Fが約15.9MB（約950MB/s）。MSAAは倍率がかかり、ロードとストアは対で数える

## iOS 標準計測（Development Build 不要）

| 症状 | 原因 | 対処 |
|---|---|---|
| 長時間プレイやテスター端末でのみ発生する負荷問題が、Mac接続状態では再現できない | 接続計測はセッションを再現できる場面しか捉えられない | Performance Trace（iOS 26+、Development Build不要）でデバイス単体収集。特にLookback Collectionはシステムが常時記録している数日分のメトリクス（CPU / GPU / FPS / メモリ等）をコントロールセンターのタップ1回で遡って収集できる。`.atrc`をAirDrop等で共有してもらい、Instrumentsまたは`metalperftrace`で分析 |
| トレースでフレームレートが落ちた時刻は特定できるが、そのときゲームが何をしていたか（シーン・設定・オブジェクト数）が分からない | Metalパフォーマンスメトリクスにはアプリ固有の情報が自動では含まれない | `StateReporter` API（iOS 27 beta、StateReportingフレームワーク）で画面遷移・グラフィックス品質等を独立ドメインとして報告。InstrumentsではPoints of Interestとして表示され、`metalperftrace overview --include-state-transitions`で滞在時間・割合付きの遷移履歴、`--aggregate --domain <domain> [--state-label <label>]`で状態別メトリクス集計が可能。UnityからはMetalPerfKit-Unityの`StateReporter`クラス（プレビュー）で利用 |
| 計測処理を組み込んでいないビルド・デバッグ機能を無効化したリリース近似ビルドで、GPU負荷やメモリをざっくり確認したい | Unity ProfilerはDevelopment Buildが前提 | Metal Performance HUD（Development Build不要）。設定アプリのDeveloper > Metal HUD、または`CAMetalLayer.developerHUDProperties`（UnityからはUnityFramework経由でCAMetalLayerを取得しネイティブプラグインで設定）。`Log Metal Performance`を有効にすると1秒間隔で`metal-HUD:`形式のCSVログが出力され、Xcodeコンソール / Console.appで確認、Unified Loggingに残る範囲なら`OSLogStore` APIで過去N秒分をファイル保存できる（MetalPerfKit-Unity `PerformanceLogger`） |

**iOS標準計測の注意**:
- Performance Traceの有効/無効切り替えにはデバイスの再起動が必要。トレースファイルはストレージ残量が少なくなると自動削除される
- iOS 27 / macOS 27 / StateReporter APIは執筆時点（2026年8月）でベータ。正式リリースまでに変わる可能性がある
- System ActivityのSummary / Detailedの違いは執筆時点でAppleの公式説明が見当たらない（原稿の調査結果）

## 計測手法の原則

- **中央値だけで判断しない**: スパイク系の問題（間引き位相・Simulate・PSO）は平均・中央値に出ない。p95/p99/標準偏差を併記する
- **Profiler接続の影響**: 接続時キャプチャは非接続より重く出る。合否判定数値は非接続で取る
- **シェーダー関数の単体計測**: FrameTimingManagerでGPU時間を取得し、対象関数だけ差し替えたバリアント（`multi_compile`。`shader_feature`は未使用バリアントが剥がれる）とベースラインの差分をとる。ループは`UNITY_LOOP`で展開抑止
- **端末世代差**: 最も重い超越関数は端末で異なる（tan: iPhone 14/X、atan: Galaxy S9/iPhone 15 Pro）。判断はターゲット端末群の下限機で実測
- **A/B比較の分離確認**: 効果の実在は「条件間の完全分離」（最悪値 < 相手の最良値）で判定。重複するなら効果量は測定ノイズに埋もれている
- **iOS標準計測の位置づけ**: Metal Performance HUD / Performance TraceはDevelopment Build不要。まずHUDやトレースで大まかな負荷を把握してボトルネック候補を絞り、その後Development Buildに切り替えてUnity Profiler / Instrumentsで詳細調査する
- **metalperftraceの構造化出力**: `--json`でJSON出力でき、スクリプトによるリグレッションテストやAIエージェントによる自動トリアージに活用できる
