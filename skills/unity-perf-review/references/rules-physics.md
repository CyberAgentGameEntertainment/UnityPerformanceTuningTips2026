# Physics パフォーマンスルール

> 出典: UnityPerformanceTuningBible - tuning_practice_physics

## ルール一覧

### PH01 MeshCollider多用
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: 動的オブジェクトにMeshCollider（静的な地形・背景への使用は正当な慣行のためlocator。同一Prefab/オブジェクトにRigidbodyが併存する場合はCritical相当）
- **grep_pattern**: `MeshCollider:`（Rigidbody併存の確認と組み合わせる）
- **理由**: 三角形単位の衝突判定で極めて重い（コスト順: Sphere < Capsule < Box < Mesh）
- **対処**: BoxCollider/SphereCollider/CapsuleColliderで近似

### PH02 Convex未設定のMeshCollider
- **レベル**: Critical
- **対象**: アセット設定
- **検出パターン**: Rigidbody付きMeshColliderでConvex=false（m_Convex: 0はデフォルト値のため静的メッシュコライダーにも全ヒットする。Rigidbody併存時のみ問題）
- **grep_pattern**: `m_Convex:\s*0`（同一オブジェクトのRigidbody併存を確認）
- **理由**: 非Convex MeshCollider + 非Kinematic RigidbodyはPhysXが未サポートでエラーになる（「重い」ではなく動作しない）。静的用途の非Convexは正常
- **対処**: 動的オブジェクトはConvex有効化（三角形255以下に簡略化。頂点数ではない）またはプリミティブ近似

### PH03 Fixed Timestep不適切
- **レベル**: Warning
- **対象**: プロファイル解析
- **検出パターン**: Fixed Timestepが過剰に小さい（0.01未満=100Hz超）
- **grep_pattern**: `Fixed Timestep:\s*0\.00`（対象: ProjectSettings/TimeManager.asset。実キーはスペース入りの`Fixed Timestep:`。0.01-0.02はターゲットFPS次第のため手動判断）
- **理由**: 物理演算頻度が高すぎてCPU負荷増大
- **対処**: ターゲットFPSに合わせる（60fps想定なら0.0167も可、モバイルの負荷重視なら0.02-0.04）。Maximum Allowed Timestepの調整も併用

### PH04 Physics.Raycast毎フレーム多発
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: Update内で複数回の`Physics.Raycast`
- **grep_pattern**: `Physics\.Raycast\(|Physics\.SphereCast\(|Physics\.BoxCast\(`
- **理由**: Broadphase+Narrowphaseが毎回走る
- **対処**: 頻度制御、RaycastNonAlloc、レイヤーマスク限定

### PH05 RaycastのGCアロケーション
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: `Physics.RaycastAll(`, `Physics.OverlapSphere(`
- **grep_pattern**: `Physics\.RaycastAll\(|Physics\.OverlapSphere\(|Physics\.OverlapBox\(|Physics\.SphereCastAll\(`
- **理由**: 結果配列が毎回ヒープ確保される
- **対処**: `NonAlloc`バリアント使用（RaycastNonAlloc, OverlapSphereNonAlloc）

### PH06 レイヤーマスク未指定
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: Raycast/OverlapでlayerMask引数省略（引数の省略有無はgrepで判別不能。全呼び出しを列挙して精査するlocator）
- **grep_pattern**: `Physics\.(Raycast|SphereCast|BoxCast|OverlapSphere|OverlapBox)\(`
- **理由**: 省略時はDefaultRaycastLayersが対象になり無駄な計算が走る
- **対処**: 必要なレイヤーのみマスク指定。maxDistanceも必ず指定する（省略時はInfinity）

### PH07 トリガーコライダー過多
- **レベル**: Warning
- **対象**: プロファイル解析
- **検出パターン**: isTriggerコライダーが100超（100は独自の目安。ヒット数集計で判定）
- **grep_pattern**: `m_IsTrigger:\s*1`（ヒット数をカウント）
- **理由**: 重なり合うコライダーのペア数が増えるとNarrowphase含め処理が急増する
- **対処**: 空間分割で削減、不要なトリガー除去

### PH08 Rigidbody.isKinematic未活用
- **レベル**: Info
- **対象**: コードレビュー
- **検出パターン**: 物理演算不要なオブジェクトのRigidbody（m_IsKinematic: 0はデフォルト値で全Rigidbodyにヒットするlocator。物理演算の要否は手動判断）
- **grep_pattern**: `m_IsKinematic:\s*0|isKinematic\s*=\s*false`
- **理由**: 非Kinematicは毎ステップ積分計算が走る
- **対処**: 物理駆動不要ならisKinematic=true。Sleep Threshold調整によるスリープ活用も併用

### PH09 Auto Sync Transforms有効化
- **レベル**: Warning
- **対象**: ProjectSettings検査
- **検出パターン**: Physics設定でAuto Sync Transformsが有効（2018.3以降デフォルト無効。有効時のみTransform変更→物理クエリのたびに暗黙のSyncTransformsが走る）
- **grep_pattern**: `m_AutoSyncTransforms:\s*1`（対象: ProjectSettings/DynamicsManager.asset）
- **理由**: 有効時はTransform変更直後の物理クエリごとに同期処理が走りスパイク要因になる。無効時（既定）は同期がFixedUpdate後にまとまる代わりに「移動直後のRaycastが旧位置に当たる」挙動に注意
- **対処**: 無効（既定）を維持し、即時同期が必要な箇所だけ`Physics.SyncTransforms()`を明示呼び出し

### PH10 Collision Matrixの未整理
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: Layer Collision Matrixが全チェック（全有効のまま未整理）
- **grep_pattern**: `m_LayerCollisionMatrix: f{8,}`（全ビットf=全有効。キー自体は全プロジェクトに存在するため値で判定）
- **理由**: 不要なレイヤー間の衝突判定が走る（Matrixでの除外はBroadphase前に効く最も効率的な削減手段）
- **対処**: 必要なペアのみ有効化

### PH11 FixedUpdate内の重い処理
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: FixedUpdate内でのGetComponent、Find、LINQ
- **grep_pattern**: `void\s+FixedUpdate\s*\(`
- **理由**: FixedUpdateは1フレームで複数回呼ばれる可能性がある
- **対処**: キャッシュ、物理計算以外はUpdateに移動

### PH12 Rigidbody.MovePositionの誤用
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: Update内で`MovePosition`/`MoveRotation`
- **grep_pattern**: `\.MovePosition\(|\.MoveRotation\(`
- **理由**: FixedUpdate以外で呼ぶと物理補間が正しく動かない
- **対処**: FixedUpdate内で使用

### PH13 Physics2Dと3Dの混在
- **レベル**: Info
- **対象**: コードレビュー
- **検出パターン**: 3Dプロジェクトへの2D物理APIの混入（2D単独使用は正常。3D APIとの併存を検出するlocator）
- **grep_pattern**: `Collider2D|BoxCollider2D|CircleCollider2D|PolygonCollider2D`
- **理由**: 2D/3D物理は別エンジンで互いに判定しない
- **対処**: どちらかに統一

### PH14 ContactPoint取得の無駄
- **レベル**: Info
- **対象**: コードレビュー
- **検出パターン**: `collision.contacts`（配列コピー）
- **grep_pattern**: `\.contacts\b|collision\.contacts`
- **理由**: contacts配列は毎回新規生成される
- **対処**: `GetContacts(List<ContactPoint>)`でリスト再利用。`Physics.reuseCollisionCallbacks`有効化も併用

### PH15 Auto Simulation不要時の有効化
- **レベル**: Info
- **対象**: プロファイル解析
- **検出パターン**: 物理コンポーネント未使用プロジェクトでSimulation Mode = FixedUpdate（m_SimulationMode: 0はデフォルト値で全プロジェクトにヒットするため、物理未使用プロジェクトに限定して指摘）
- **grep_pattern**: `m_SimulationMode:\s*[01]|m_AutoSimulation:\s*1`（対象: ProjectSettings/DynamicsManager.asset）
- **理由**: 毎フレーム（またはUpdateごとに）Physics.Simulateが走る
- **対処**: `Physics.simulationMode = SimulationMode.Script`で手動制御（Unity 2022.2+。旧API: `Physics.autoSimulation`）
