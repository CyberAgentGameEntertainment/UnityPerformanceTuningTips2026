# **UIシステムの使い分け — IMGUI / uGUI / UI Toolkit**

名前 / ローマ字：財津 大睦 / Zaitsu Hiromu  
所属：株式会社サムザップ  
Xアカウント / Webサイト / GitHubアカウント（任意）：@RomGameclient

前作『Unity パフォーマンスチューニング バイブル』{{fn:tuning_bible}}では、Canvasの分割やRaycast Targetの調整といった、uGUIの負荷への対処を扱いました。本章では一歩さかのぼり、UIシステムの選択そのものを扱います。どれを選ぶかが、UIの負荷構造と取れる対処の種類を決めるからです。  
UnityにはIMGUI、uGUI、UI Toolkitの3つのUIシステムがあります。この3つを順に取り上げ、それぞれがランタイムとエディター拡張でいまどう位置づけられているかを整理します。本章の記述はUnity 6.5時点の情報に基づきます。

# **IMGUI**

ランタイムのIMGUIは、ゲーム内UIの選択肢から外れています。公式マニュアルは、プレイヤーが操作する通常のゲーム内UIに使うことを意図していないと明記しており、想定用途としてゲーム内のデバッグ表示とツール、カスタムインスペクター、エディター拡張を挙げています。デバッグオーバーレイやランタイムのログビューアという用途では、現在も使用されています。  
一方、エディター拡張においてIMGUIは公式に現役です。公式のUIシステム比較ページ{{fn:ui_system_compare}}はIMGUIを代替手段として挙げ、利点としてエディターの拡張機能に制限なくアクセスできることと、画面にすばやくUIを描画できる軽量なAPIであることの2点を示しています。  
公式のIMGUI移行ガイド{{fn:imgui_migration}}には、UI Toolkitに等価物がないAPIが明記されています。実務で影響が大きいのは InspectorTitlebar()、Toolbar()、IntPopup()、LinkButton() あたりです。いずれもエディターウィンドウの見た目をUnity本体に寄せるためのAPIで、必要な場面では現状IMGUIを使うことになります。  
ただし、エディター拡張として使ううえで知っておきたい負荷特性があります。IMGUIは OnGUI() の中にUIを毎回記述する即時モードのシステムで、UIツリーの情報を保持せず、イベントのたびに構築と破棄を繰り返します。保持モードであるUI Toolkitとの違いは {{fig:render_cycle}} のとおりです。OnGUI() は1フレームに複数回実行され、そのたびにヒープ確保が発生しやすくなります。常時再描画されるエディターウィンドウでは、これがGCへの負荷になります。  
![][image1]  
図: 即時モードと保持モードの描画サイクル {{fig:render_cycle}}

# **uGUI**

CanvasとGameObjectをベースにしたシステムです。要素ひとつひとつがGameObjectであり、Transform階層を持ちます。ランタイム専用で、エディター拡張には使えません。  
ランタイムでは事実上の標準です。利点は3つあります。  
・Sceneビューで直接配置して、その場でプレビューできる  
・AnimatorとTimelineでUIを動かせる  
・Asset StoreにuGUIを前提としたパッケージが大量にある  
負荷の面では、要素に変更があったCanvasではメッシュの再構築が走り、要素数に比例してTransform階層の更新やRaycast Targetの走査のコストが乗ります。前作で扱ったCanvasの分割やRaycast Targetの調整は、いずれもこの構造に由来する負荷への代表的な対処です。

# **UI Toolkit**

VisualElement のツリーを一度構築し、以後は変化した部分だけを更新する保持モードのシステムです。ロジックをC\#、UI構造をUXML、スタイルをUSSに分けて記述します。ランタイムとエディター拡張の両方で使え、エディター拡張では公式の第一推奨です。ただし、公式は「複雑なエディターツールを作るなら」という書き方をしています。数個のフィールドを並べるだけのツールであれば、UXMLとUSSを持ち込まずC\#だけで組んでも困りません。  
負荷の面では、uGUIとの違いがそのまま対処の違いになります。要素をひとつ変えるとCanvas全体のメッシュが作り直されるuGUIと違い、UI Toolkitは変化した部分だけを更新するため、Canvasの分割にあたる対処は不要です。ただし要素数が増えればレイアウト計算のコストは同じように増えるので、要素を減らす工夫は引き続き必要になります。Unity 6系では、イベントディスパッチの高速化やジオメトリ生成のジョブ化、レイアウト計算のキャッシュ改善といった性能改善も続いています。

## **uGUIと比べて不足している点**

Unity 6.5時点でできないこととしては、次のようなものが挙げられます。いずれもUnityが開発中としている項目ですが{{fn:uitk_status}}、いま採用を判断するなら、できない前提で考える必要があります。  
・VisualElement のキーフレームアニメーションをAnimationウィンドウで扱う機能  
・SceneビューでのUI編集  
・UXMLで定義した VisualElement をMonoBehaviourスクリプトから参照する仕組み  
・カスタムシェーダー向けの頂点データ対応  
エコシステムも薄いままです。Asset Storeの資産はGameObject階層を前提にしたものが多く、そのままでは動きません。TextMeshProもUI Toolkitからは使えず、UI Toolkitは専用のテキストシステム（6.5で既定になったAdvanced Text Generator）を使います。DOTweenも VisualElement 用のショートカットを追加していますが、対象はUnity 6.3以降です{{fn:dotween}}。  
uGUIから移行する場合、アンカーとピボットに直接の等価物はありません。UI Toolkitのレイアウトは、親要素の中に子要素を順に流し込んで並べる方式です。要素ごとに位置とサイズを指定していたuGUIとは、組み立て方そのものが変わります。

# **公式の推奨の変遷**

公式のUIシステム比較ページはマニュアルのバージョンごとに公開されており、読み比べるとランタイムUIの推奨理由が入れ替わっています{{fn:ui_system_compare_60}}。その内訳を {{table:recommendation_shift}} に示します。  
表: ランタイムUIの推奨理由の変化 {{table:recommendation_shift}}

| 要件・用途 | Unity 2022.3〜6.1 | Unity 6.5 |
| :---- | :---- | :---- |
| 大量のUIを扱う | UI Toolkit | UI Toolkit |
| アーティストとデザイナー向けの制作ワークフローが必要 | UI Toolkit | UI Toolkit |
| テクスチャレスなUI描画 | UI Toolkit | UI Toolkit |
| 3D空間に配置・ライティングされるUI | uGUI | UI Toolkit |
| カスタムシェーダーとマテリアルによる表現 | uGUI | UI Toolkit |
| MonoBehaviourからの参照が容易 | uGUI | uGUI |

2022.3から6.1まで一貫してuGUI側にあった2つは{{fn:ui_system_compare_2022}}、ワールドスペースUIが6.2、カスタムシェーダーが6.3でURP向けに実装されたことでUI Toolkit側へ移りました。残る「MonoBehaviourからの参照が容易」も、UXMLで定義した要素を参照する仕組みが開発中の項目に入っています。ただし、比較ページの構成そのものはUnity 6.5でも変わっていません。ランタイムUIはuGUIを既定として紹介し、UI Toolkitをその代替として並べる形のままです。入れ替わったのは推奨の理由だけで、どちらを既定に置くかは動いていません。同じページに載っている用途別の一覧も、見出しが2022.3から6.2までは「Recommendation（推奨）」、6.3以降は「Often used for（よく使われる）」に変わりました。Unity自身、これを推奨とは呼ばなくなっています。

# **UIシステムを併用するときの制約**

1つのプロジェクトで複数のUIシステムを使うこと自体はできますが、組み合わせるといくつか問題が出ます。ランタイムとエディター拡張に分けて見ていきます。

## **ランタイム**

uGUIとUI Toolkitは同じプロジェクトで使えますが、一方を他方の中に埋め込むことはできません。ナビゲーションとスタイルの不整合も、公式が制約として挙げています。  
描画順は、それぞれのシステムの中でしか制御できません（{{fig:sort_order}}）。UI ToolkitでUIを画面に出すには、GameObjectに Panel Renderer コンポーネントを付けます。この役割はUnity 6.4以前では UIDocument が担っており、6.5で置き換わりました{{fn:ui_document_component}}。LTSの6.3を使っている場合は UIDocument になります。この Panel Renderer が並び替えの単位で、パネル同士はPanel SettingsのSort Orderで、同じパネル内の Panel Renderer 同士は PanelRenderer.sortingOrder で順序が決まります。uGUIはルートCanvasのSort Orderで並べます。この2つをまたいだ順序を指定するプロパティはありません。  
uGUIからの移行ガイド{{fn:ugui_migration}}はレンダーテクスチャで描画を合成する手も挙げていますが、入力イベントは届かないと明記しています。操作するUIには使えないため、uGUIとUI Toolkitを重ねる設計は避けてください。  
![][image2]  
図: UI ToolkitとuGUIの描画順の決まり方 {{fig:sort_order}}

## **エディター拡張**

エディター拡張では、2つのシステムを入れ子にできる向きが片方だけに限られます（{{fig:propertydrawer_nesting}}）。UI Toolkitのツリーの中にIMGUIの描画を置くことは IMGUIContainer でできますが、その逆、IMGUIの描画の中にUI Toolkitのツリーを置くことはできません。  
これが問題になるのは、インスペクターの表示を自作するときです。インスペクターのカスタマイズは、コンポーネント全体を受け持つカスタムインスペクターと、フィールド単位で見た目を差し替える PropertyDrawer の2段になっており、内側の PropertyDrawer は自分がどちらのシステムのインスペクターから呼ばれるかを選べません。UI Toolkitだけで実装した PropertyDrawer が、IMGUIで書かれたカスタムインスペクターや、IMGUIの PropertyDrawer の中から呼ばれると、そのフィールドは値の欄が「No GUI Implemented」（GUIが実装されていない）という表示に置き換わります。Unity 2022.2以降はインスペクターの既定がUI Toolkitなので多くの場合は起きませんが、IMGUIのカスタムインスペクターやPropertyDrawerが残っているプロジェクトでは、PropertyDrawer側にIMGUI版の描画も用意しておく必要があります。  
![][image3]  
図: PropertyDrawerの入れ子の可否 {{fig:propertydrawer_nesting}}

# **おわりに**

調べてみて印象的だったのは、公式の推奨理由がUnity 6.5でほぼUI Toolkit側へ移っていたことです。ランタイムはuGUIが標準のままで、動いているものを移す理由はありませんが、新規開発でUI Toolkitを避ける理由は確実に減っています。迷ったら、AnimatorとTimelineの演出とAsset Storeの資産にどれだけ依存しているかで判断するのがよさそうです。エディター拡張は、複雑なツールはUI Toolkit、Unity本体の見た目に寄せる場面はIMGUIという整理で当面は困らないと感じました。UIシステムの選択は、前作で扱った対処が必要になるかどうかを決める、いちばん上流のチューニングです。

脚注: {{fn:tuning_bible}} https://cyberagentgameentertainment.github.io/UnityPerformanceTuningBible/ja/preface.html  
脚注: {{fn:ui_system_compare}} https://docs.unity3d.com/6000.5/Documentation/Manual/UI-system-compare.html （2026年8月6日参照）  
脚注: {{fn:imgui_migration}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-IMGUI-migration.html  
脚注: {{fn:uitk_status}} https://discussions.unity.com/t/ui-toolkit-development-status-and-next-milestones-november-2025/1698009  
脚注: {{fn:dotween}} https://dotween.demigiant.com/download.php  
脚注: {{fn:ui_system_compare_60}} https://docs.unity3d.com/6000.0/Documentation/Manual/UI-system-compare.html （2026年8月6日参照）  
脚注: {{fn:ui_system_compare_2022}} https://docs.unity3d.com/2022.3/Documentation/Manual/UI-system-compare.html （2026年8月6日参照）  
脚注: {{fn:ui_document_component}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-create-ui-document-component.html  
脚注: {{fn:ugui_migration}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-Transitioning-From-UGUI.html

[image1]: assets/zaitsu-hiromu-2/media/image1.png

[image2]: assets/zaitsu-hiromu-2/media/image2.png

[image3]: assets/zaitsu-hiromu-2/media/image3.png