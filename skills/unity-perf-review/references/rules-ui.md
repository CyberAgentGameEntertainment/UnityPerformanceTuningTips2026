# UI パフォーマンスルール

> 出典: UnityPerformanceTuningBible - tuning_practice_ui

## ルール一覧

### UI01 Canvas未分割
- **レベル**: Critical
- **対象**: コードレビュー / アセット設定
- **検出パターン**: 1つのCanvasに動的要素と静的要素が混在
- **grep_pattern**: `^Canvas:`（.unity/.prefab。CanvasはビルトインコンポーネントのためYAMLでは専用ブロックとして直列化される。m_Script行では検出不能）
- **理由**: 1要素の変更でCanvas全体のメッシュ再構築が走る
- **対処**: 静的/動的/頻繁更新でCanvasを分割。ただし分割するとCanvasをまたぐバッチングが切れるトレードオフあり。入れ子Canvas配下のSetActiveは親Canvas側が高負荷になる場合がある点にも注意

### UI02 Raycast Target不要な有効化
- **レベル**: Warning
- **対象**: コードレビュー / アセット設定
- **検出パターン**: 装飾用Image/TextでRaycast Target = true（ボタン等の正当な有効化にもヒットするため手動確認前提のlocator）
- **grep_pattern**: `m_RaycastTarget:\s*1`
- **理由**: EventSystem Raycast時に不要な判定対象が増加
- **対処**: タップ不要な要素はRaycast Target無効化。Preset機能でImage/RawImageのデフォルトをプロジェクト全体で無効化しておくと再発を防げる

### UI03 Layout Group毎フレーム再計算
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: 頻繁に子要素が変わるLayout Group
- **grep_pattern**: `HorizontalLayoutGroup|VerticalLayoutGroup|GridLayoutGroup`
- **理由**: 子要素変更の度にGetComponent+レイアウト再計算が走る
- **対処**: 必要時のみ手動レイアウト、またはLayoutRebuilder制御

### UI04 Legacy Text使用
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `UnityEngine.UI.Text`、`using UnityEngine.UI`でのText参照
- **grep_pattern**: コード側 `UnityEngine\.UI\.Text\b|\bText\s+\w+`（using UnityEngine.UI併存確認）、シーン/プレハブ側はTextコンポーネントのGUID `guid: 5f7201a12d95ffc409449d95f23cf332`（m_Script行はGUIDのみでクラス名を含まないため）
- **理由**: フォント毎にテクスチャ生成、リッチテキストでGC発生
- **対処**: TextMeshPro使用

### UI05 ScrollViewの大量要素
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: ScrollRect配下に100超の子要素（要素数はgrepで判定不能。全ScrollRect検出後に手動確認）
- **grep_pattern**: `ScrollRect`
- **理由**: 非表示要素もメッシュ生成+レイアウト計算対象（RectMask2d配下ならレンダリングはカリングされるがメッシュ生成/レイアウトコストは残る）
- **対処**: オブジェクトプール + 仮想スクロール実装

### UI06 透明Imageのフィルレート消費
- **レベル**: Warning
- **対象**: アセット設定
- **検出パターン**: alpha=0または極低alphaのImageコンポーネント（タップ領域用）
- **grep_pattern**: `a:\s*0\.0[0-9]|a:\s*0\}`
- **理由**: CanvasRenderer.cullTransparentMesh無効時（Unity 2020.2より前の作成物や明示無効化）や極低alpha（0.01等）ではフラグメントシェーダーが実行されフィルレートを消費。cullTransparentMesh有効時もメッシュ生成・リビルドコストは残る
- **対処**: Raycastのみ必要なら専用の空Graphicコンポーネント（現在もベストプラクティス）

### UI07 SetActive頻繁切り替え
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: UIパネルの`SetActive(true/false)`頻繁呼び出し（UI以外のSetActiveにもヒットするため、uGUIコンポーネントを持つオブジェクトへの呼び出しに限定して精査）
- **grep_pattern**: `\.SetActive\(|\.gameObject\.SetActive\(`
- **理由**: OnEnable/OnDisableでCanvas再構築、コンポーネント初期化が走る（書籍実測: SetActive 323msに対しCanvasGroup.alpha 3.6ms）
- **対処**: CanvasGroup.alpha=0 + CanvasGroup.blocksRaycasts=false、またはCanvas.enabled=false。いずれもGameObjectはアクティブのままなのでUpdate等は動き続ける点に注意

### UI08 ランタイムゲームUIでのIMGUI（OnGUI）使用
> 出典: UnityPerformanceTuningBible 2026 - UIシステムの使い分け
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: 非Editorコードに実処理を持つOnGUIが存在し、デバッグ表示以外のゲーム内UIに使われている
- **grep_pattern**: `void\s+OnGUI\s*\(`（Editor/フォルダを除外。デバッグオーバーレイ用途は許容。空のOnGUIはUS01）
- **理由**: 公式マニュアルがゲーム内UI用途を意図しないと明記。即時モードでUIツリーを保持せずイベントごとに構築破棄を繰り返し、OnGUIは1フレームに複数回実行されるたびヒープ確保が発生しやすい
- **対処**: ゲーム内UIはuGUIまたはUI Toolkitへ。IMGUIはデバッグ表示・ツール・エディター拡張に限定

### UI09 常時再描画エディターウィンドウでのIMGUI多用
> 出典: UnityPerformanceTuningBible 2026 - UIシステムの使い分け
- **レベル**: Info
- **対象**: コードレビュー（Editorコード）
- **検出パターン**: EditorWindow派生クラスがOnGUIで常時再描画されている
- **grep_pattern**: `class\s+\w+\s*:\s*EditorWindow`（OnGUI実装と常時Repaintの有無は手動確認）
- **理由**: 常時再描画されるエディターウィンドウではOnGUIのたびのヒープ確保がGCへの負荷になる
- **対処**: 複雑なエディターツールはUI Toolkit（保持モード）で構築。等価APIが無い場面のみIMGUI

### UI10 uGUIとUI Toolkitの重ね合わせ設計
> 出典: UnityPerformanceTuningBible 2026 - UIシステムの使い分け
- **レベル**: Critical
- **対象**: コードレビュー / シーン構成
- **検出パターン**: 同一画面でuGUI CanvasとUI Toolkit（UIDocument / Panel Renderer）を重ねて描画順・入力を前提にした設計
- **grep_pattern**: `UIDocument|PanelRenderer`（同一シーン内Canvasとの重なりは手動確認）
- **理由**: 描画順は各システム内でしか制御できず、2つをまたいだ順序指定プロパティは存在しない。レンダーテクスチャ合成は入力イベントが届かない（公式移行ガイド明記）
- **対処**: 重ねる設計を避け、画面単位でどちらかに寄せる

### UI11 UI Toolkit版のみのPropertyDrawer
> 出典: UnityPerformanceTuningBible 2026 - UIシステムの使い分け
- **レベル**: Warning
- **対象**: コードレビュー（Editorコード）
- **検出パターン**: CreatePropertyGUIのみ実装したPropertyDrawerが、IMGUIのカスタムインスペクター/PropertyDrawerが残るプロジェクトに存在する
- **grep_pattern**: `override\s+VisualElement\s+CreatePropertyGUI`（IMGUI版`OnGUI`オーバーライド不在＋プロジェクト内にIMGUIカスタムインスペクター存在を確認）
- **理由**: IMGUIの中にUI Toolkitツリーは置けない（逆はIMGUIContainerで可能）。IMGUI側から呼ばれるとフィールドが「No GUI Implemented」表示になる
- **対処**: IMGUIのカスタムインスペクターが残るプロジェクトではPropertyDrawerにIMGUI版描画も用意
