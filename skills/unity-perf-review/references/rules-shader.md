# シェーダー パフォーマンスルール

> 出典: UnityPerformanceTuningBible 2026 - GraphicsStateCollectionウォームアップ / 超越関数最適化

## ルール一覧

### SH01 ShaderVariantCollectionによるウォームアップ（モダンAPI環境）
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: unity>=6000.0
- **検出パターン**: Metal/Vulkan/D3D12対象プロジェクトで`ShaderVariantCollection.WarmUp()`によるウォームアップを実装している（GraphicsStateCollection未使用）
- **grep_pattern**: `ShaderVariantCollection`と`\.WarmUp\s*\(`
- **理由**: ShaderVariantCollectionはレンダリング設定を持たずウォームアップはシェーダープログラム生成までにとどまり、描画時にドライバがPSOを生成しスタッターが残る。Vulkan実例で1回のPSO生成に数十ミリ秒
- **対処**: Unity 6以降のモダンAPIではGraphicsStateCollection（GSC）ウォームアップへ移行（AS16も参照）

### SH02 GSCの一括WarmUpによるスパイク
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: unity>=6000.0
- **検出パターン**: `GraphicsStateCollection`に対し`WarmUpProgressively`ではなく`WarmUp(dependency)`を呼んでいる
- **grep_pattern**: `GraphicsStateCollection`を含むファイルで`\.WarmUp\s*\(`にマッチし`WarmUpProgressively`不在
- **理由**: `WarmUp(dependency)`は全コンパイルを一括実行するためスパイクが発生する
- **対処**: `WarmUpProgressively(count, dependency)`で件数指定し複数フレームに分散（参考実装値: 毎フレーム4件）

### SH03 isWarmedUpによる完了待ちループ
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: unity>=6000.0
- **検出パターン**: GSCウォームアップの終了判定を`isWarmedUp`のみで行うループ
- **grep_pattern**: `isWarmedUp`
- **理由**: ウォームアップエラー発生時に`isWarmedUp`が真にならない場合があり（Unity 6000.3で確認）、完了待ちループが無限ループ化する
- **対処**: 依頼件数の合計と`totalGraphicsStateCount`の比較で終了判定

### SH04 GSCメタデータの実行環境不一致
- **レベル**: Critical
- **対象**: アセット設定
- **適用条件**: unity>=6000.0
- **検出パターン**: GSCファイルの`graphicsDeviceType`/`runtimePlatform`/`qualityLevelName`がビルドターゲット・実行環境と不一致（別プラットフォーム収集の流用、Editorスクリプト再保存による上書き）
- **grep_pattern**: (手動確認)（ランタイムログ`WarmUp may be inaccurate due to .* mismatch`で検出可能）
- **理由**: GraphicsStateはグラフィックスAPIごとに異なり、別プラットフォームで収集したGSCはメタデータを書き換えても中身が一致せずウォームアップが不正確になる
- **対処**: 発生ケースで対処を分ける。別プラットフォーム収集の流用は該当プラットフォームの実機で収集し直す。Editorスクリプト再保存による上書きはメタデータを実機の値へ修正すれば足りる（再収集不要）。qualityLevelName不一致のみ、PSOに影響するレンダリング設定やシェーダーが変化しない場合に限りウォームアップ直前の書き換えで警告回避可

### SH05 シェーダーのAssetBundle重複パッキング（iOS/Metal）
- **レベル**: Critical
- **対象**: アセット設定
- **適用条件**: unity>=6000.0, ios
- **検出パターン**: 同一シェーダーが複数のAssetBundleに依存関係で暗黙にパッキングされている（シェーダー専用グループ未設定）
- **grep_pattern**: (手動確認)（Addressables Reportで重複確認、または`Resources.FindObjectsOfTypeAll<Shader>()`をnameでGroupByしCount()>1を検出）
- **理由**: iOS（Metal）ではシェーダーが二重インスタンス化され、GSCが参照するインスタンスしかウォームアップされない。別インスタンス側の初回描画時にPSO作成が走りスタッターが発生（エラーは出ず、`Shader.CreateGPUProgram`なしで`GpuProgramMetal.GetCachedPipeline`が発生）。Vulkanでは発生しない
- **対処**: シェーダーを専用グループの単一バンドルにまとめ暗黙パッキングを排除。集約困難ならバンドルごとにGSCを用意

### SH06 SubPass内シェーダーのウォームアップ失敗（iOS/Metal）
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: unity>=6000.0, ios
- **検出パターン**: iOSターゲットかつNative Render Pass有効（Unity 6000.5以降は常時有効）でSubPass内描画シェーダーをGSCウォームアップに依存
- **grep_pattern**: `BeginSubPass\s*\(`または`nativeRenderPassesEnabled`（URP Deferred使用も該当）
- **理由**: iOS（Metal）ではGSCウォームアップがSubPass内描画シェーダーに機能せず、エラーなしで完了してもプレイ中にPSOコンパイルが発生する
- **対処**: SubPassの利用を見直すか、対象シェーダーをロード画面中に1回実際に描画するプリレンダリングでPSOキャッシュを作る

### SH07 Experimental APIへの依存（GSC）
- **レベル**: Info
- **対象**: コードレビュー
- **適用条件**: unity>=6000.0
- **検出パターン**: `UnityEngine.Experimental.Rendering`名前空間のGSC APIを使用
- **grep_pattern**: `UnityEngine\.Experimental\.Rendering`と`GraphicsStateCollection`
- **理由**: Unity 6000.3時点でGSCはExperimental APIであり将来仕様変更の可能性がある
- **対処**: バージョンアップ時にAPI変更の影響を確認する運用を組み込む

### SH08 Quality Settings非対応のGSC収集
- **レベル**: Warning
- **対象**: コードレビュー / アセット設定
- **適用条件**: unity>=6000.0
- **検出パターン**: Quality LevelでMSAA/HDR等PSOに影響する設定が変化するのに、GSC収集を単一Quality Levelでしか行っていない
- **grep_pattern**: 収集コードに`QualitySettings\.SetQualityLevel`が含まれるか（半機械的）
- **理由**: 収集されていないQuality Levelではウォームアップが効かない
- **対処**: Quality Settingsごとにトレースし、Quality Level名を付けて別ファイル保存。実行時は現在のQuality Levelに対応するGSCをロード

### SH09 フラグメントシェーダー内の超越関数多用
- **レベル**: Warning
- **対象**: コードレビュー（シェーダー）
- **検出パターン**: フラグメントシェーダー内で三角関数・指数関数・対数関数を呼び出している（同一関数5回超は特に深刻）
- **grep_pattern**: `\b(sin|cos|tan|asin|acos|atan|exp|exp2|pow|log|log2|log10)\s*\(`（.shader/.hlsl/.cginc。fragment関数内かはgrep単体で判定不能のため半機械的・手動確認前提）
- **理由**: 超越関数はコンパイラで数十命令に展開され、SFUはALUより少なく、レジスタプレッシャーも高める。実測: sin1回増でiPhone 15 Pro約0.1ms、旧世代端末（iPhone X/Galaxy S9）は5回超で1回につき約0.2〜0.6ms増加。存在するだけで旧世代端末は+0.5ms、新世代端末も+0.25ms（レジスタプレッシャー）
- **対処**: 呼び出し回数を削減、可能なら近似計算に置き換え。未使用の超越関数コードはバリアント分割で生成コードから除去

### SH10 sin関数の放物線近似への置き換え余地
- **レベル**: Info
- **対象**: コードレビュー（シェーダー）
- **検出パターン**: フラグメントシェーダーで`sin()`を高頻度に呼び、入力域が|x|<=PIに収まり誤差0.056程度が許容できる箇所
- **grep_pattern**: `\bsin\s*\(`（置き換え可否は手動判断）
- **理由**: 放物線近似`(4.0/PI)*x - (4.0/(PI*PI))*x*abs(x)`は乗算とabsのみ。実測（10回実行時）でiPhone X約2.5ms、Galaxy S9約1.3ms削減
- **対処**: 精度要件を確認のうえ近似式に置き換え。係数は定数畳み込みされ実行時除算コストなし

### SH11 tan/atan関数の使用（超越関数中で最重量級）
- **レベル**: Warning
- **対象**: コードレビュー（シェーダー）
- **検出パターン**: フラグメントシェーダー内の`tan()`/`atan()`呼び出し
- **grep_pattern**: `\b(tan|atan|atan2)\s*\(`
- **理由**: tanは内部で除算が呼ばれsin/cosより高負荷。実測ではiPhone 14/Xでtanが、Galaxy S9でatanが最重量。近似置き換えで最大約6ms削減（iPhone X・10回実行時）
- **対処**: 軽量重視はテイラー展開`x * (1.0 + x*x * 0.3333333)`（|x|がPI/4を超えると急速に精度低下）、精度重視はパデ近似`x * (15.0 - x*x) / (15.0 - 6.0 * x*x)`（|x|<PI/4で高精度、除算1回含む）

### SH12 定数指数のpowをuniform経由で書いている
- **レベル**: Info
- **対象**: コードレビュー（シェーダー）
- **検出パターン**: `pow(x, n)`の指数が実行時uniformで渡されているが実際は固定値
- **grep_pattern**: `\bpow\s*\(.+,\s*_\w+`（第1引数に関数呼び出しを含む形にも対応。固定値かは手動確認）
- **理由**: 指数を定数にすると`pow(x, 2) -> x * x`のようにコンパイラが畳み込み、汎用pow経路（超越関数展開）を回避できる
- **対処**: 指数が固定なら定数リテラルにするか乗算に展開

### SH13 グローバル値のUnityPerMaterial混入
- **レベル**: Warning
- **対象**: コードレビュー（シェーダー）
- **適用条件**: srp
- **検出パターン**: `Shader.SetGlobal*`で設定するグローバル値を`CBUFFER_START(UnityPerMaterial)`内に宣言している
- **grep_pattern**: `CBUFFER_START\(UnityPerMaterial\)`ブロックの宣言とC#側`Shader\.SetGlobal(Float|Int|Vector|Color)`対象名の突き合わせ（半機械的）
- **理由**: マテリアル単位でない値をUnityPerMaterialに入れるとSRP Batcherが壊れる（GR03も参照）
- **対処**: グローバル値はUnityPerMaterialの外で宣言

### SH14 動的ループ使用シェーダーのシェーダーモデル指定漏れ
- **レベル**: Info
- **対象**: コードレビュー（シェーダー）
- **検出パターン**: uniform変数を条件とする動的ループを含むシェーダーで`#pragma target`が3.0未満（未指定時の既定2.5を含む）
- **grep_pattern**: `UNITY_LOOP|\[loop\]`を含むファイルで`target\s+[3-9]`不在（半機械的。Surface Shaderの`#pragma surface ... target 3.0`形式も考慮）
- **理由**: ループ回数がuniformの動的ループにはtarget 3.0以上が必要
- **対処**: `#pragma target 3.0`以上を指定
