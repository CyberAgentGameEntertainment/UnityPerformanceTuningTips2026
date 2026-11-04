# ビルド設定 パフォーマンスルール: Managed Code Stripping

> 出典: UnityPerformanceTuningBible 2026 - Managed Code Stripping
> 対象: IL2CPP。実測はUnity 6000.3.19f1 / Addressables 4.0.1ベース。
> LTO系のルールは rules-build.md を参照。

## ルール一覧

### BD12 リフレクション経由でのみ参照される型・メンバーの保持指定漏れ
> 出典: UnityPerformanceTuningBible 2026 - Managed Code Stripping
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: il2cpp
- **検出パターン**: `Type.GetType` / `Activator.CreateInstance` / `GetMethod` 等に、定数でない文字列（フィールド・設定ファイル・シリアライズデータ・サーバー応答由来）を渡しており、対象型に link.xml / [Preserve] / Require系属性の保持指定がない。Managed Stripping Level が Medium/High（ユーザーコードも削減対象）の場合に問題化
- **grep_pattern**: `Type\.GetType\(|Activator\.CreateInstance\(|\.GetMethod\(`（ヒット箇所で文字列が定数か、保持指定があるかを精査）
- **理由**: 実測（Unity 6000.3.19f1、High）で、リフレクションのみで参照される型は型ごと削除され`Type.GetType("ReflectionOnlyService")`がnullを返した。typeofで型だけ参照した場合も、どこからも呼ばれないメソッドはメンバー単位で削除されGetMethodがnullを返した。なおUnityLinkerはリフレクションの典型的な使い方を一部理解しており、`Type.GetType("...")`に定数文字列を直接渡した場合はHighでも型が保持され、typeofで得たTypeに直接GetMethodを呼んだ場合はその型のpublicメソッドが一括保持される。一方、フィールド経由で渡した文字列は追跡されない（constフィールドは使用箇所へ定数として埋め込まれるため追跡される）
- **対処**: link.xml（編集できないDLL含む決まった対象）または[Preserve]（自コードの少数要素）で保持を明示する。フレームワーク的な「実装を探して呼ぶ」構造は[RequireImplementors]/[RequireDerived]で契約宣言する。恒久対応としてはリフレクション探索自体をSource Generatorによる静的呼び出しへ置き換えれば保持指定の保守が不要になり、起動コストと実行時失敗の可能性も減る

### BD13 DIコンテナー登録型のデフォルトコンストラクタ削除
> 出典: UnityPerformanceTuningBible 2026 - Managed Code Stripping
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: il2cpp
- **検出パターン**: リフレクションでコンストラクタを呼ぶDIコンテナー（Register/BindとResolveの組）をManaged Stripping Level Medium/Highで使用しており、実装型のコンストラクタ保持策（link.xml / [Preserve] / Source Generator注入コード生成）がない。アセンブリスキャンによる自動登録方式はさらに危険度が高い
- **grep_pattern**: `\.Register<|\.Bind<|\.Resolve<`（コンテナー実装がリフレクション生成か、保持策があるかを精査）
- **理由**: 実測（High）で、`Register<INotificationService, NotificationService>()`の型引数は静的参照になるため型とメソッドは残るが、リフレクションでしか呼ばれない暗黙のデフォルトコンストラクタはメンバー単位で削除され、`Activator.CreateInstance`が`MissingMethodException: Default constructor not found for type NotificationService`で解決に失敗した。型は登録してあるのにビルドしたアプリでだけ解決に失敗する。アセンブリから実装型をスキャンして自動登録する方式では、静的な参照を持たない実装型が型ごと削除され登録自体が失敗する
- **対処**: 実装型のコンストラクタまで保持指定する（[Preserve]は型とメソッドの両方に必要な点に注意）か、Source Generatorで注入コードを生成する方式（例: VContainerのSourceGeneratorオプション）へ切り替えて実行時リフレクションを減らす

### BD14 AssetBundle専用コンテンツが参照する型・アセンブリの保持漏れ
> 出典: UnityPerformanceTuningBible 2026 - Managed Code Stripping
- **レベル**: Critical
- **対象**: コードレビュー / ビルド設定
- **適用条件**: il2cpp
- **検出パターン**: AssetBundleを直接ロードする運用（Addressables非経由）で、アプリ本体のコードから静的参照がない型（Timeline等）をコンテンツ側だけが必要としているのに、link.xml等での保持指定がない。rootの供給源はビルドに含まれるシーン・アセットであり、別ビルドのAssetBundleはrootに寄与しない
- **grep_pattern**: `AssetBundle\.LoadFrom\w*\(`（ヒット時、バンドル内コンテンツが必要とするアセンブリの保持指定〈Assets配下のlink.xml等〉の有無を精査）
- **理由**: 実測で、TimelineアセットをAssetBundleにのみ含めアプリ側がUnity.Timelineを静的参照しない構成では、「The referenced script (UnityEngine.Timeline.TimelineAsset) on this Behaviour is missing!」が出てアセットのロードに失敗し、ビルドのManagedフォルダーにUnity.Timeline.dll自体が存在しなかった。この結果はMinimalでもHighでも同じであり、Stripping Levelの調整では対処できない
- **対処**: 必要な型・アセンブリをlink.xmlで明示保持する。ビルドターゲットやコンテンツで保持対象が変わる場合はIUnityLinkerProcessor（GenerateAdditionalLinkXmlFileが返したパスのlink.xmlがUnityLinkerの入力に追加される）でビルド時に生成する。Addressables利用時はコンテンツビルドが依存を解析したlink.xml（Addressables 4.0.1ではLibrary/com.unity.addressables/aa/<platform>/AddressablesLink/link.xml）を自動生成するため、手作業でなくこの仕組みを活用する

### BD15 link.xmlのアセンブリ全体preserve="all"の恒久運用
> 出典: UnityPerformanceTuningBible 2026 - Managed Code Stripping
- **レベル**: Info
- **対象**: コードレビュー（Assets配下のlink.xml）
- **適用条件**: il2cpp
- **検出パターン**: 手書きのlink.xmlで`<assembly ... preserve="all">`を型・メンバーの絞り込みなしに指定している
- **grep_pattern**: `<assembly[^>]*preserve="all"`（対象: Assets配下のlink.xml。子要素で型を絞っているかを確認）
- **理由**: アセンブリ全体のpreserve="all"は手軽だが、そのアセンブリに対するストリッピングの効果を打ち消す。原稿は「問題の切り分けには使えるが、恒久的な対応では必要な型・メンバーまで絞り込める方が良い」とし、方針として「[Preserve]やlink.xmlを過剰に利用しない」「保持指定はできる限り型・メンバー単位まで絞る」を挙げている
- **対処**: 切り分け完了後、保持範囲を型単位・メンバー単位（メソッド・フィールド）まで絞り込む

### BD16 [Preserve]属性の付与位置と保持範囲の不一致
> 出典: UnityPerformanceTuningBible 2026 - Managed Code Stripping
- **レベル**: Warning
- **対象**: コードレビュー
- **適用条件**: il2cpp
- **検出パターン**: リフレクションで「生成して呼び出す」対象なのに、[Preserve]が型かメソッドの片方にしか付いていない。型に付けた場合はデフォルトコンストラクタまで、メソッドに付けた場合は型とそのメソッドまでしか保持されない
- **grep_pattern**: `\[Preserve\]`（ヒット型がリフレクションで生成・呼び出しされる場合、型とメソッドの両方に付いているかを精査）
- **理由**: 実測（High）で、型にのみ[Preserve]を付けた型は型とデフォルトコンストラクタは残るがメソッドが削除され、メソッドにのみ付けた型は型とそのメソッドは残るがデフォルトコンストラクタが保持されず`Activator.CreateInstance`がMissingMethodExceptionで失敗した。「[Preserve]を付けたのに動かない」場合はまず付与位置と保持範囲の対応を疑う
- **対処**: リフレクションで生成して呼び出すところまで動かすには、型とメソッドの両方に[Preserve]を付ける。実装型を1つずつ指定する代わりに、インターフェイスへの[RequireImplementors]（実装型をデフォルトコンストラクタごと保持することを実測で確認）や基底クラスへの[RequireDerived]も使える

### BD17 Managed Stripping Level Lowの使用
> 出典: UnityPerformanceTuningBible 2026 - Managed Code Stripping
- **レベル**: Info
- **対象**: ProjectSettings
- **適用条件**: il2cpp
- **検出パターン**: ProjectSettings/ProjectSettings.assetのmanagedStrippingLevelでプラットフォーム値がLow（1）
- **grep_pattern**: `managedStrippingLevel`（後続行のプラットフォーム別値が1=Lowかは手動確認）
- **理由**: LowはUnity 6.3のドキュメントで将来的な廃止が予告されており、利用は推奨されていない
- **対処**: MinimalまたはMedium/Highへ移行する。Medium/Highへ上げる場合はBD12〜BD16の保持指定とBD18の検証運用をセットで行う

### BD18 Stripping Level引き上げ構成のビルド検証欠如
> 出典: UnityPerformanceTuningBible 2026 - Managed Code Stripping
- **レベル**: Warning
- **対象**: ビルド設定 / CIパイプライン
- **適用条件**: il2cpp
- **検出パターン**: Managed Stripping LevelをMedium/Highに設定しているのに、その構成でビルドしたアプリを開発中から動かして検証する運用（CIでの実機/プレイヤービルド動作確認）がない
- **grep_pattern**: (手動確認 — Stripping Level設定とCI/検証運用の有無を確認)
- **理由**: 削りすぎの症状はビルドしたアプリでしか現れない。エディター実行やストリッピング無効の既定Monoビルドでは正常に動くため開発中に気づきにくく、Stripping Levelを上げた途端に特定の機能だけが壊れる形で表面化する。原稿の結論も「静的解析から見えない依存を明示し、ビルドしたアプリのサイズと動作を検証し続ける運用とあわせて、初めて安全に使える機能」
- **対処**: Stripping Levelを上げたビルドを開発中から動かして検証する。何が削られたかはMono.Cecilでストリップ後アセンブリ（ビルド出力、またはLibrary/Bee/artifacts/<target>/ManagedStripped/に残る中間生成物）の型・メソッド一覧（.ctor含む）をダンプし、Levelを変えた2ビルドの差分で特定できる
