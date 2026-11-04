# アニメーション パフォーマンスルール

> 出典: UnityPerformanceTuningBible 2026 - 3Dキャラクターアニメーション / Animatorプーリング / VAT

## ルール一覧

### AN01 キャラクターFBXのOptimize Game Objects未適用
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: 大量表示されるキャラクターFBXのRig設定でOptimize Game Objectsが無効（全ボーンのGameObject/Transformがランタイム生成される）
- **grep_pattern**: `optimizeGameObjects: 0`（FBXの.meta。キャラクター用FBXかは手動判断）
- **理由**: 実測でTransform 166→24個。300体でMeshSkinning.Update 6.23→1.537ms（-75.7%）、MeshSkinning.CalcMatrices（全スレッド36.633ms）が完全消滅。600体・軽量描画構成でフレーム時間-11.9%
- **対処**: インポート設定でOptimize Game Objects有効化。設定を触れないアセットは生成時に`AnimatorUtility.OptimizeTransformHierarchy(go, null)`（効果は同等）。必要なボーンはExtra Transforms to Exposeで個別露出

### AN02 手動更新Playable Graphの1体1グラフ構成
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: `DirectorUpdateMode.Manual`のグラフをキャラクター単位で生成し、ループ内で`Evaluate()`をN回呼ぶ構成（グラフをまたぐ並列化が行われず直列処理になる）
- **grep_pattern**: `SetTimeUpdateMode\s*\(\s*DirectorUpdateMode\.Manual\s*\)`（周辺でキャラごとの`PlayableGraph.Create`とループ内`.Evaluate(`を確認）
- **理由**: 600体で1体1グラフ8.396ms対全体1グラフ4.500ms（1.87倍差）。1体1グラフではワーカースレッドがほぼIdle、集約時は全ワーカーに均等分散
- **対処**: 手動更新では全キャラクター（または位相バケット単位）で1本のPlayableGraphに集約し、`Evaluate()`の呼び出し回数を最小化

### AN03 Animator.Update()の明示呼び出しによる間引き
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: `animator.enabled = false`にした上で`Animator.Update(deltaTime)`を自前で呼ぶアニメーションLOD/間引き実装（同期評価になりジョブ並列化を喪失）
- **grep_pattern**: `[Aa]nimator\w*\.Update\s*\(`（レシーバ付き呼び出しのみ。MonoBehaviourの`void Update()`定義とは自然に区別される。AN11の正当用法`animator.Update(0f)`＝リセット直後の即時評価は除外）
- **理由**: 1体あたりメインスレッドコストが6.7→15.4µs/体（約2.3倍）。1/2間引きでも削減効果は-4.1%にとどまる（並列化損失に相殺）
- **対処**: `Animator.enabled`切り替えでバッチ更新に参加する体を毎フレーム選ぶ（1/2で-33.5%）、または位相別手動グラフ（1/2で-38.8%）。`Animator.Update()`を自分で呼ぶ実装を選ぶ理由はない

### AN04 Animator無効化間引きでkeepAnimatorStateOnDisable未設定
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: `Animator.enabled`をLOD目的でトグルするコードで`keepAnimatorStateOnDisable = true`の設定がない（既定falseでは無効化のたびステートが巻き戻る）
- **grep_pattern**: `[Aa]nimator\w*\.enabled\s*=`があるファイルで`keepAnimatorStateOnDisable`が不在（AND条件。`\.enabled\s*=`全般はRenderer/Collider等に誤爆するためAnimator変数名に限定）
- **理由**: 既定値falseでは無効化のたびにコントローラのステートが巻き戻りアニメーションが進まない。間引き分の時間補正として`animator.speed = interval`も必要
- **対処**: 初期化時に`animator.keepAnimatorStateOnDisable = true`と`animator.speed = interval`を設定

### AN05 間引き実装に位相ずらし（Slot）がない
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `frameIndex % interval == 0`のような全キャラクター一斉の間引き判定（Nフレームに1回、全体分の負荷が1フレームに集中）
- **grep_pattern**: `%\s*\w*[Ii]nterval`（比較対象がキャラ固有スロットか定数0かを確認）
- **理由**: 600体・1/4間引きで位相なしはp95 9.470ms・標準偏差3.863の櫛状スパイク。位相ずらしはp95 2.707ms・標準偏差0.185で毎フレーム更新より安定。位相ずらしは負荷を減らさず平らにする施策
- **対処**: `index % interval`でキャラクターごとに更新フレームの位相をずらし、毎フレーム1/N体ずつ更新

### AN06 大量キャラクターのプロシージャル処理（IK等）をLateUpdateで実装
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: 多数インスタンス化されるキャラクターで`LateUpdate`内にボーンTransformの反復計算（IK・ルック制御等）がある（全コストがメインスレッド直列）
- **grep_pattern**: `void\s+LateUpdate\s*\(`（同一ファイルの`\.rotation\s*=|Quaternion\.`と組み合わせ）
- **理由**: Two-Bone IK解法64回・300体でLateUpdate実装は+6.498ms（100%メインスレッド）、Animation Job化で+0.199ms（33倍差）。ただし演算量が少なければノード固定コスト（300体で約2.16ms）によりLateUpdateの方が総CPU時間は少ない
- **対処**: 演算量が多い場合は`AnimationScriptPlayable` + `IAnimationJob`に移し、ボーンアクセスは`TransformStreamHandle`/`AnimationStream`、毎フレーム変わる入力は`NativeArray`経由

### AN07 [BurstCompile]を付けただけのSIMD高速化期待
- **レベル**: Info
- **対象**: コードレビュー
- **検出パターン**: `IAnimationJob`実装に`[BurstCompile]`のみ付与し、SoA化・分岐除去・超越関数排除などSIMD前提のデータ設計をしていない
- **grep_pattern**: `\[BurstCompile\]`と`IAnimationJob`の共起（設計妥当性は手動確認）
- **理由**: 実測でBurstの総CPU時間削減は11.0%、フレーム時間は差なし。AnimationStreamのGet/Setが反復間依存を作り、1体分のストリームしか見えずレーンを埋めるデータがなく、acos・三角関数がベクトル化を止める
- **対処**: 数倍を狙うなら`TransformAccessArray` + `IJobParallelForTransform`等でN体分をバッチジョブに集め、SoA化・`math.select`分岐除去・超越関数のループ外追い出し。Burst Inspectorで最適化適用を確認

### AN08 Optimize Game Objects適用対象でボーン階層に依存するコード
- **レベル**: Warning
- **対象**: コードレビュー / アセット設定
- **検出パターン**: `optimizeGameObjects: 1`のモデルに対し`transform.Find`等でボーンのGameObject階層をたどるコードが残っている
- **grep_pattern**: `extraExposedTransformPaths: \[\]`が空のまま`optimizeGameObjects: 1`（.meta側は機械検出可能、コード側は手動確認）
- **理由**: ボーンがGameObjectとして存在しないため、階層をたどってボーンを触るコードは動かなくなる
- **対処**: 必要なボーンだけExtra Transforms to Exposeで露出。デバッグ時は`AnimatorUtility.DeoptimizeTransformHierarchy`で一時的に戻せる

### AN09 プール返却時のAnimator状態リセット漏れ
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: unity>=6000.3
- **検出パターン**: Animator付きGameObjectをObjectPoolで再利用しているが、返却/取得時にResetControllerState（または同等の初期化）を呼んでいない
- **grep_pattern**: `(ObjectPool<|IObjectPool<)`（プール対象にAnimator含むか、返却処理を手動確認）
- **理由**: 再利用ではComponentが前回利用時の状態を持ったまま戻り、前回のStateやparameter（特にTrigger残留）が次回利用へ影響する
- **対処**: Unity 6.3以降は返却時に`animator.ResetControllerState(true)`（全LayerのState・Float/Bool/Int既定値化・Triggerリセット）。PlayableGraph内のAnimatorControllerPlayableにも同APIあり

### AN10 ResetControllerState対象外プロパティの戻し漏れ
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: unity>=6000.3
- **検出パターン**: ResetControllerState呼び出し箇所で、SetLayerWeightで変更したLayer Weight・Animator.speed・StateMachineBehaviourの独自変数を戻していない
- **grep_pattern**: `ResetControllerState\(`（周辺で`SetLayerWeight`/`speed`リセットの有無を確認）
- **理由**: Layer Weight・speed・StateMachineBehaviour独自変数はControllerのState/parameterとは別プロパティでResetControllerStateでは戻らない。物理・Particle・Audio等も対象外
- **対処**: ResetControllerState(true)に続けて明示的に既定値へ戻す。初期化対象をAnimatorController内/外に分けて責務を明確化

### AN11 リセット直後のStateInfo即時参照
- **レベル**: Info
- **対象**: コードレビュー
- **適用条件**: unity>=6000.3
- **検出パターン**: ResetControllerState直後にGetCurrentAnimatorStateInfo等の評価結果を参照している
- **grep_pattern**: (手動確認)
- **理由**: リセット直後の評価結果参照には明示的な評価が必要
- **対処**: `animator.Update(0f)`で評価してから参照

### AN12 破片・部品ごとの個別MeshRenderer + Animator構成
- **レベル**: Critical
- **対象**: アセット設定 / シーン構成
- **検出パターン**: 破壊表現や群衆など多数の剛体パーツを、パーツごとに個別MeshRendererとして持ちAnimator（Transformアニメーション）で再生するPrefab構成
- **grep_pattern**: `m_Script:.*Animator`を含むPrefab内の`MeshRenderer`出現数カウント（半機械的）
- **理由**: 実測（破片1,315個×16体、Pixel 8a）でCPUメインスレッド154.3ms（うちSRPBatcher.Flush 76.17ms）、描画命令約21万回/フレーム、GPUメモリ+411.8MB。1メッシュ統合Skin方式は16.5ms、VAT方式は12.6ms・99回・メモリ増ほぼゼロ
- **対処**: 1つのメッシュに統合。個別制御不要な決まったアニメーションならVAT化（Rigid-Body Dynamics）、CPU再生許容なら1破片1ボーンのSkinnedMeshRenderer統合。なおGPU時間はSkin方式が有利（22.2ms対VAT 32.5ms）で、VATが常に最善ではない

### AN13 VATテクスチャの圧縮設定違反
- **レベル**: Critical
- **対象**: アセット設定
- **適用条件**: vat
- **検出パターン**: VAT用テクスチャ（ポジション/回転/頂点カラー）にテクスチャ圧縮が適用されている
- **grep_pattern**: VATテクスチャの.metaで圧縮設定を検査（VATテクスチャの特定は命名規則依存）
- **理由**: VATは正確な値が必要でテクスチャ圧縮と相性が悪く事実上非圧縮で扱う。精度不足でアニメーションが壊れる
- **対処**: 圧縮無効化を含む専用インポート設定（VAT Import Settings Guide）を適用。量産時はScripted Importerで自動化

### AN14 VAT Shader Graphテンプレートの直接利用・Sub Graph参照の挿げ替え漏れ
- **レベル**: Warning
- **対象**: アセット設定 / シェーダーレビュー
- **適用条件**: vat
- **検出パターン**: SideFX VATパッケージ同梱Shader Graphの直接利用、またはコピーがパッケージ側Sub Shader Graphを参照したまま
- **grep_pattern**: .shadergraphからSideFXパッケージ配下GUIDへの参照検査（半機械的）
- **理由**: 公式Shader Graphはテンプレート位置づけで実用上カスタマイズが必要。パッケージ側参照のままだと意図しない挙動・依存になる
- **対処**: プロジェクト用にコピーし、Sub Shader Graphの参照もコピー側へ挿げ替える
