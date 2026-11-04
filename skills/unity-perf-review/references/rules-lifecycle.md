# ライフタイム管理 パフォーマンスルール

> 出典: UnityPerformanceTuningBible 2026 - UniTask/キャンセル処理 / AssetBundleのライフタイム管理

## ルール一覧

### LT01 CancellationTokenSourceのDispose漏れ
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: newしたCancellationTokenSourceがusing宣言/using文/明示的Disposeで管理されていない
- **grep_pattern**: `new CancellationTokenSource\(`（同スコープの`using`/`Dispose`を確認。生成した側がDisposeする責務）
- **理由**: CancellationTokenSourceはIDisposable。長寿命CTSへのコールバック登録などで参照が残り、保持するタイマーやコールバック登録が解放されない
- **対処**: `using var cts = new CancellationTokenSource();`で管理し、正常終了・キャンセル・例外のいずれでも脱出時にDispose。使用を終えた時点でのDisposeを原則とする

### LT02 CreateLinkedTokenSourceの戻り値のDispose漏れ
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: CreateLinkedTokenSourceの戻り値をusingで管理せずDisposeも呼んでいない
- **grep_pattern**: `CreateLinkedTokenSource\(`（using無し箇所を抽出し手動確認）
- **理由**: CreateLinkedTokenSourceは元トークンにキャンセルコールバックを登録する。元トークンが長寿命の間「長寿命CTS → コールバック登録 → linkedCts → 短命側への登録情報」の参照連鎖が残り、linkedCts.Tokenに登録されたコールバックが参照する他オブジェクトも保持されることがある（この参照関係だけで必ずリークするわけではない。リスクが顕在化するのは長寿命CTSが生存し続けるケース。内部実装はUnity向けMono実装に基づきバージョンで変わりうる）
- **対処**: 戻り値をリンクトークンを使う非同期処理を含むスコープでusing管理。不要になった時点で必ずDispose（内部でCancellationTokenRegistrationが破棄されコールバック解除）

### LT03 CancellationToken.Register登録の解除漏れ
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: CancellationToken.Registerの戻り値（CancellationTokenRegistration）を破棄せず放置
- **grep_pattern**: `[Tt]oken\.Register\(`（戻り値の保持とDisposeを確認）
- **理由**: キャンセル前に登録されたコールバックは発行元CTSに保持され、登録不要になってもDisposeしないとコールバックが参照するオブジェクトが解放されないことがある
- **対処**: CancellationTokenRegistrationを保持し、登録が不要になった時点でDisposeして解除

### LT04 動的ロードの窓口未集約
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: AssetBundle/Addressablesのロード・解放APIがローダー基盤クラス以外の各所から直接呼ばれている
- **grep_pattern**: `(AssetBundle\.LoadFrom|Addressables\.(LoadAssetAsync|InstantiateAsync|LoadSceneAsync))`（基盤クラス以外でのヒットをフラグ）
- **理由**: 各実装者が個別に管理すると解放漏れや不要なアセットの常駐が発生しやすい
- **対処**: ロードと解放の窓口をシステム基盤へ集約し、ロード時にライフタイムを決定、終了時に基盤側でまとめて解放する仕組みにする

### LT05 ロード時のライフタイム未決定
- **レベル**: Warning
- **対象**: コードレビュー / 設計レビュー
- **検出パターン**: ロードAPIにライフタイム指定（引数または関数分け）が無く、解放タイミングが呼び出し側の任意になっている
- **grep_pattern**: (手動確認 — 自前ローダーAPIのシグネチャレビュー)
- **理由**: ロード時にライフタイムが必ず決まり、その終了時に基盤側で解放できることが要点。決まらない設計は解放漏れ・常駐の温床
- **対処**: 引数指定方式か関数分け方式（LoadCommonAsync/LoadSceneAsync/LoadViewAsync等）を採用。分類例: Common（ログアウト/アカウント切替まで）、Scene、View、Custom

### LT06 非同期ロード完了前のライフタイム終了への未対応
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: ロード完了前にシーン/画面が終了した場合の、キャンセルまたは完了後結果の解放処理が無い
- **grep_pattern**: (手動確認 — ローダー基盤のロード追跡・途中終了処理の実装確認)
- **理由**: 非同期ロードの完了前にライフタイムが終了すると、キャンセルか完了後解放が無い限りアセットが宙に浮く
- **対処**: ロード開始時から処理を追跡し、途中終了時はキャンセルするか完了後の結果を解放。複数ライフタイムからの共用アセットは参照数を管理し最後の利用終了時に解放

### LT07 GameObjectスコープの破棄後Add未対応
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: OnDestroyでハンドルを解放するスコープComponentが、破棄後にAddされたハンドルを解放しない
- **grep_pattern**: (手動確認 — スコープ管理クラスの解放済みフラグ相当の有無)
- **理由**: GameObject破棄後に非同期ロードが完了してAddが呼ばれるケースがあり、対応しないとハンドルがリークする
- **対処**: 解放済みフラグを持ち、破棄後のAddは即座にDispose。管理用GameObjectはアセットごとでなく1ライフタイム単位（画面/ダイアログ）につき1つ

## 補足: CancelとDisposeの役割の違い

レビュー時の典型的な誤解ポイント:
- **Cancel** = 処理へのキャンセル要求の通知
- **Dispose** = CTSが保持するタイマー・コールバック登録の解除・解放（Disposeだけではキャンセル要求は通知されない）
- CancellationToken自体はIDisposable非実装（Dispose不要）。IDisposableなのはCancellationTokenSourceとCancellationTokenRegistration
- Dispose漏れ＝即メモリリークではない。問題になるのは長寿命CTS（アプリ全体スコープ等）へのコールバック登録が残るケース
