# Unity スクリプト パフォーマンスルール

> 出典: UnityPerformanceTuningBible - tuning_practice_script_unity

## ルール一覧

### US01 空のUnityイベント関数
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: 空の`Update()`, `LateUpdate()`, `FixedUpdate()`, `OnGUI()`
- **grep_pattern**: `void\s+(Update|LateUpdate|FixedUpdate|OnGUI)\s*\(\s*\)\s*\{\s*\}`（複数行の空ボディは一次候補 `void\s+(Update|...)\s*\(\s*\)` から中身の有無を精査）
- **理由**: 空でもC++→C#のマネージドコール発生、毎フレームコスト
- **対処**: 使わないイベント関数は削除

### US02 GetComponent毎フレーム呼び出し
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: Update系内の`GetComponent<`, `GetComponentInChildren<`（Awake/Startでのキャッシュ取得は正当。指摘はUpdate/LateUpdate/FixedUpdate内のみ）
- **grep_pattern**: `GetComponent<|GetComponentInChildren<|GetComponentInParent<|GetComponents<`
- **理由**: ネイティブ呼び出し+コンポーネントリストの型検索コストが毎フレーム発生する
- **対処**: Awake/Startでキャッシュ

### US03 Find系メソッドのランタイム使用
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: `GameObject.Find(`, `FindObjectOfType(`, `FindObjectsOfType(`
- **grep_pattern**: `GameObject\.Find\(|Find(ObjectOfType|ObjectsOfType|AnyObjectByType|ObjectsByType)\s*[<(]`（ジェネリック形式`FindObjectsOfType<T>()`も検出）
- **理由**: シーン全体を走査するO(n)処理。Criticalは毎フレーム/頻繁な呼び出し。初期化時の単発使用はInfo相当
- **対処**: 参照をインスペクタで設定、またはレジストリパターン。旧FindObjectOfTypeはUnity 2023.1以降非推奨（FindAnyObjectByType/FindObjectsByType + FindObjectsSortMode.Noneが高速な代替）

### US04 CompareTag未使用
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `.tag ==`, `.tag !=`
- **grep_pattern**: `\.tag\s*[!=]=`
- **理由**: tagプロパティアクセスで文字列がヒープ確保される
- **対処**: `CompareTag("TagName")`を使用

### US05 Debug.Logのリリースビルド残留
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: `Debug.Log(`, `Debug.LogWarning(`, `Debug.LogError(`（条件コンパイル外。自作ログラッパークラス内部の1箇所は正当なので除外）
- **grep_pattern**: `Debug\.Log\(|Debug\.LogWarning\(|Debug\.LogError\(|Debug\.LogException\(`
- **理由**: ログはリリースビルドでも実行され、logEnabled=falseでも文字列生成・連結は走る
- **対処**: `[Conditional]`属性付き自作ログラッパー経由に統一（UnityEngine.Debug.Logには直接付けられない）+ Scripting Define Symbolsで制御

### US06 Instantiate/Destroyの頻繁な使用
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: ループ内/Update内の`Instantiate(`, `Destroy(`
- **grep_pattern**: `\bInstantiate\(|\bDestroy\(|\bDestroyImmediate\(`
- **理由**: オブジェクト生成/破棄はGC+内部処理ともに重い
- **対処**: オブジェクトプール使用

### US07 Coroutineの多用
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `StartCoroutine(`, `yield return new`
- **grep_pattern**: `StartCoroutine\(|yield\s+return\s+new`
- **理由**: StartCoroutineでGC、yield return newで毎回ヒープ確保
- **対処**: `WaitForSeconds`はキャッシュ、UniTask検討

### US09 SendMessageの使用
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: `SendMessage(`, `BroadcastMessage(`
- **grep_pattern**: `SendMessage\(|BroadcastMessage\(|SendMessageUpwards\(`
- **理由**: リフレクション使用で極めて遅い+文字列GC
- **対処**: 直接メソッド呼び出し、イベント、インターフェース

### US10 アニメーション文字列パラメータ
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `.SetFloat("`, `.SetBool("`, `.SetTrigger("` 等の文字列パラメータ指定（インスタンス経由呼び出しが大半）
- **grep_pattern**: `\.(SetFloat|SetBool|SetTrigger|SetInteger|GetFloat|GetBool|GetInteger|ResetTrigger|Play|CrossFade)\(\s*"`
- **理由**: 文字列からハッシュ変換が毎回走る（Material.SetFloat("_Prop")等の文字列指定も同種の問題）
- **対処**: `Animator.StringToHash()`/`Shader.PropertyToID()`で事前キャッシュ

### US11 ParticleSystem.Simulateへの大きな秒数指定
> 出典: UnityPerformanceTuningBible 2026 - ParticleSystemのSimulateスパイク
- **レベル**: Warning（Noiseモジュール有効なパーティクルに対してはCritical）
- **対象**: コードレビュー / アセット設定
- **検出パターン**: `ParticleSystem.Simulate`の第1引数に大きな秒数（または上限管理されていない変数）を渡している。特にNoiseModuleが有効なParticleSystemとの併用
- **grep_pattern**: `\.Simulate\(`（`Physics.Simulate`/`Physics2D.Simulate`/`PhysicsScene.Simulate`は別APIのため除外し、ParticleSystem型変数への呼び出しのみ対象。大値リテラルは`\.Simulate\(\s*[0-9]{2,}`。プレハブ側のNoise有効は`grep -A2 'NoiseModule:' *.prefab | grep 'enabled: 1'`と突き合わせ。差分レビューでコード側だけが差分に出た場合も、参照先PrefabのNoiseModuleを読み込んで判定する）
- **理由**: Simulateは指定時間まで一括シミュレーションする。実測で20秒指定によりNoiseJobが約2000回コールされ約50msのスパイクが発生
- **対処**: 長時間指定が本当に必要か確認。Noise有効パーティクルではSimulateの利用を控える（Noiseを外すか秒数を最小化）。Max Particlesの削減は緩和にはなるが根本対策ではない
