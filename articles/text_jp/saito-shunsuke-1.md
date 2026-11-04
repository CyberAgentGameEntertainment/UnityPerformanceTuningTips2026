# **スタイルガイド**

# **Unity Performance Tuning Bible 2026**  **原稿作成テンプレートのご案内**

このドキュメントは、Re:VIEW への変換を目的とした原稿収集用のテンプレートです。

# **1ページ目に記載していただくこと**

* タイトル  
* 名前 / ローマ字 / 所属  
* Xアカウント/Webサイト/GitHubアカウント (任意)

# **適用されている基本スタイル**

* **本文:** Noto Sans JP、12pt、行間 1.8、段落後の間隔 12pt、文字色 \#1f2937  
* **見出し:** 見出し 1/2/3 スタイルを使用、文字色 \#111827、見出し前の間隔は広めに、後の間隔は短めに設定されています。

# **執筆者向け作成ルール (Re:VIEW 変換必須要素)**

**1\. 見出し:** Google ドキュメントの「見出し 1 / 見出し 2 / 見出し 3」スタイルを使用してください。  
**2\. 本文:** 標準テキストスタイルで作成してください。  
**3\. コードブロック、画像、表、脚注:** 詳細は「**リスト・図・表・脚注の書き方**」セクションを参照し、Re:VIEW 変換用の専用フォーマット（CODE START/END、ID参照）を必ず使用してください。  
**4\. 注意:** 色、フォント、インデントなどの変更はしないでください。

# **リスト・図・表・脚注の書き方**

Re:VIEW 変換時に番号は自動で付与されるため、執筆者は **\`リスト8.1\`**、**\`図8.5\`**、**\`表8.1\`** のような固定番号を本文中に直接書かないでください。  
本文中で参照したい場合は、固定番号ではなく専用IDを使います。IDは英数字とアンダースコアのみで、内容が分かる名前にしてください。  
例:

* \`settext\_usage\`  
* \`canvas\_disable\`  
* \`ui\_toggle\_cost\`  
* \`zstring\`

# **コードリスト**

コードは以下の形式で書いてください。  
CODE START csharp | SetTextの利用例 {{list:settext_usage}}  
label.SetText("{0}", number);  
CODE END  
キャプションが不要な場合:  
CODE START csharp  
label.text \= number.ToString();  
CODE END  
ルール:

* \`CODE START 言語名 | キャプション {{list:id}}\` の形式で書きます。  
* C\# の場合は \`csharp\` を使います。  
* 本文中で参照する可能性があるコードには \`{{list:id}}\` を付けます。  
* コード内ではスマートクォート \`“ ”\` を使わず、通常の \`"\` を使ってください。  
* コードに色や太字などの装飾は付けないでください。

本文中で参照する場合:  
\`{{list:settext_usage}} のように SetText を利用すると、第2引数の値を表示できます。\`  
変換後は Re:VIEW 側で リスト8.1 のような番号付き参照に置き換えます。

# **図**

画像は Google Docs 本文中に挿入し、画像の直下にキャプションを書いてください。  
例:  
![][image1]  
図: チューニングバイブルロゴ {{fig:bible_logo}}  
ルール:

* キャプションは必ず画像の直下に書きます。  
* 本文中で参照する可能性がある図には {{fig:id}} を付けます。

本文中で参照する場合:  
これは、チューニングバイブルのロゴです（{{fig:bible_logo}}）。

# **表**

表は Google Docsの表機能で作成して構いません。表の直前にキャプションを書いてください。  
例:  
表: 表示切り替えの処理時間 {{table:ui_toggle_cost}}

| 手法 | 処理時間（表示） | 処理時間（非表示） |
| :---- | :---- | :---- |
| SetActive | 323.79ms | 209.93ms |
| Canvasのenabled | 61.25ms | 61.23ms |
| CanvasGroupのalpha | 3.64ms | 3.40ms |

ルール:

* キャプションは表の直前に書きます。  
* 本文中で参照する可能性がある表には {{table:id}} を付けます。  
* セル結合はできるだけ避けてください。  
* 複雑な装飾や色付けはしないでください。  
* 単位は列名に含めてください。

本文中で参照する場合:  
{{table:ui_toggle_cost}} の結果から、CanvasGroupを使った手法が最も処理時間が短いことが分かります。

# **脚注**

脚注を入れたい箇所には、本文中に {{fn:id}} を書いてください。脚注の内容は、章末または原稿末尾にまとめて書いてください。  
本文中の例:  
このTextMeshProの機能は、ZString{{fn:zstring}}と組み合わせると非常に強力です。  
脚注定義の例:  
脚注: {{fn:zstring}} [\[https://github.com/Cysharp/ZString\](https://github.com/Cysharp/ZString)](https://github.com/Cysharp/ZString)  
ルール:

* 脚注IDは英数字とアンダースコアのみで書きます。  
* URLだけの脚注でも問題ありません。

# **本文とのつなぎ方**

リスト・図・表を本文中で説明するときは、固定番号を直接書かず、ID参照を使ってください。  
避ける例:  
リスト8.1 のように利用すると、第2引数の値を表示できます。図8.5 は Canvas を無効にする例です。表8.1 の結果から分かるように...  
推奨例:  
{{list:settext_usage}} のように利用すると、第2引数の値を表示できます。{{fig:canvas_disable}} は Canvas を無効にする例です。{{table:ui_toggle_cost}} の結果から分かるように...  
最終的な Re:VIEW 変換時に、これらの参照を実際の リスト8.1、図8.5、表8.1 のような番号付き参照へ置き換えます。

# **本文**

# **Unity 3D キャラクターアニメーションのチューニング**

* 名前: 斉藤俊介 \- Shunsuke Saito

* 所属: 株式会社Colorful Palette

# **はじめに**

本章では「Unityでの3Dキャラクターアニメーション機能の最適化手法」について、アセット設定や実装の複数の観点で紹介します。

## **計測環境**

本章の実装・計測は以下の環境で行いました。

表: 計測環境 {{table:measurement_env}}

| 項目 | 値 |
| :---- | :---- |
| Unity | 6000.5.7f1 |
| プラットフォーム | macOS Standalone (StandaloneOSX) |
| Scripting Backend | IL2CPP / IL2CPP Compiler Configuration \= Release / Code Generation \= OptimizeSpeed |
| Graphics API | Metal / GPU Skinning \= 有効 |
| Quality Level | PC / Skin Weights \= Four Bones |

## **使用アセットとその構造**

計測に使うキャラクターはunity-chanをベースに、**Genericリグ**として再インポートしたものです。 後述するOptimize Game Objectsの適用前後で以下のようなスペックとなっています。

表: 計測に使うプレハブの Transform 数と頂点数 {{table:character_prefab_spec}}

| プレハブ | Transform数 | 頂点数 |
| :---- | :---- | :---- |
| Base | 166 | 13,391 |
| Optimized（Optimize Game Objects） | 24 | 13,391 |

# **Unity Animationの基本制御について**

まず、Unityでアニメーションを制御する3つの手法を整理します。

## **1\. Animatorコンポーネントを使用した制御**

\`Animator\`に\`RuntimeAnimatorController\`を割り当てる、もっとも一般的な方法です。ステートマシン・トランジション・ブレンドツリー・パラメータといった仕組みが揃っており、Mecanimのワークフローがそのまま使えます。

CODE START csharp | Animator コンポーネントでの制御 {{list:animator_control}}  
var animator \= GetComponent\<Animator\>();  
animator.runtimeAnimatorController \= controller;   // ステートマシンを割り当てる  
animator.SetFloat("Blend", 0.5f);                  // パラメータで制御する  
CODE END

重要なのは、Animatorは内部的に**Playable Graphを組み立てて実行している**ことです。ステートマシンの評価もクリップのサンプリングも、内部ではグラフのノードとして処理されています。次の2手法は、この内部グラフを自分の手で組む方法にあたります。

## **2\. Playable GraphをPlayable APIで構築し、自動更新で制御**

\`PlayableGraph\`を自分で組み立てる方法です。ステートマシンを介さずにブレンド構造を完全に制御でき、後述するAnimation Job(\`AnimationScriptPlayable\`)をグラフの任意の位置に差し込めます。

CODE START csharp | Playable API でのグラフ構築（自動更新） {{list:playable_auto}}  
var graph \= PlayableGraph.Create("MyGraph");  
var output \= AnimationPlayableOutput.Create(graph, "Animation", animator);

var clipA \= AnimationClipPlayable.Create(graph, waitClip);  
var clipB \= AnimationClipPlayable.Create(graph, walkClip);  
var mixer \= AnimationMixerPlayable.Create(graph, 2);  
graph.Connect(clipA, 0, mixer, 0);  
graph.Connect(clipB, 0, mixer, 1);  
mixer.SetInputWeight(0, 0.5f);  
mixer.SetInputWeight(1, 0.5f);

output.SetSourcePlayable(mixer);  
graph.Play();  
CODE END

更新モードは既定で\`DirectorUpdateMode.GameTime\`です。このモードではUnityのライフサイクル（UpdateとLateUpdateの間にあるAnimationフェーズ）で、Animatorコンポーネントと同じタイミングで**自動的に評価**されます。呼び出し側で毎フレーム何かをする必要はありません。

## **3\. Playable GraphをPlayable APIで構築し、手動更新で制御**

同じグラフに\`DirectorUpdateMode.Manual\`を設定すると、Unityは一切自動評価をしなくなり、\`Evaluate()\`を呼んだときだけグラフが評価されます。

CODE START csharp | 手動更新でのグラフ評価 {{list:playable_manual}}  
graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);  
graph.Play();

void Update()  
{  
    graph.Evaluate(Time.deltaTime);   // 呼んだときだけ評価される  
}  
CODE END

評価のタイミングと頻度を完全に制御できるため、「特定のキャラクター（グラフ）だけ更新を止める」「数フレームに1回だけ更新する」といった制御がそのまま実装できます。

3手法の関係をまとめると次の通りです。

表: アニメーション制御 3 手法の比較 {{table:animation_control_methods}}

| 手法 | グラフの構築 | 評価のタイミング |
| :---- | :---- | :---- |
| Animatorコンポーネント | Unity（内部） | UnityのAnimationフェーズ |
| Playable API \+ 自動更新 | 自分 | UnityのAnimationフェーズ |
| Playable API \+ 手動更新 | 自分 | \`Evaluate()\`を呼んだとき |

# **アセットの設定**

コード側の最適化を紹介する前に、アセット側の設定で適用できる最適化から見ていきます。この節のテーマは**ボーンのTransform階層をなくす**ことです。同じ結果を得る手段が2つあります。

## **Optimize Game Objects（インポート時）**

FBXインポート設定（Rigタブ）の\`Optimize Game Objects\`を有効にすると、実行時にボーンのGameObject / Transformが丸ごと生成されなくなり、Animatorが内部スケルトンだけを持つようになります。今回のキャラクターでは**Transformが166個から24個**（ルート \+ Renderer 23個）に減りました。

※ボーンがGameObjectとして存在しないため、階層をたどってボーンを触るコードは動かなくなります。武器のアタッチポイントなど特定のボーンだけが必要な場合は、\`Extra Transforms to Expose\`で個別に残します。

## **Optimize Hierarchy（ランタイム）**

同じことを実行時に行うAPIが\`AnimatorUtility.OptimizeTransformHierarchy\`です。

CODE START csharp | ランタイムでの階層フラット化 {{list:optimize_hierarchy_api}}  
// go は Animator（と有効な Avatar）を持つルート。第 2 引数は露出させたいボーン名の配列  
AnimatorUtility.OptimizeTransformHierarchy(go, null);

// 逆変換もある（エディタ拡張やデバッグで階層を一時的に戻したいとき）  
AnimatorUtility.DeoptimizeTransformHierarchy(go);  
CODE END

インポート設定を触れないアセットでも、生成時に実行することで同じ構造になります。実際に確認したところ、Baseプレハブ（Transform 166個）に適用すると**Transform 24個・\`hasTransformHierarchy \== false\`**となり、インポート時にOptimize Game Objectsを適用したプレハブと完全に同じ構造になりました。

![][image2]

図: 最適化前のHierarchy。\`Character1\_Hips\`以下にボーンのTransform階層が166個続く。 {{fig:hierarchy_base}}

![][image3]

図: \`AnimatorUtility.OptimizeTransformHierarchy\`適用後。ボーン階層が消え、ルート直下にSkinnedMeshRenderer 23個が並ぶだけになる（Transform 24個）。 {{fig:hierarchy_optimized}}

## **実測: 階層フラット化の効果**

描画負荷を2段階に変えて測りました。1つめはRendererを全部有効にした構成（300体 × 23 Renderer \= 6,900 Renderer）、2つめは各キャラクターで頂点数が最大のSkinnedMeshRenderer 1個だけを有効にして描画負荷を下げた構成です。フレーム時間は中央値です。

表: 階層フラット化の効果（300 体・全 Renderer と 600 体・1 Renderer） {{table:optimize_hierarchy_result}}

| 構成 | バリアント | フレーム時間 | \`MeshSkinning.Update\` (main) | \`MeshSkinning.CalcMatrices\` (all) | \`Semaphore.WaitForSignal\` |
| :---- | :---- | :---- | :---- | :---- | :---- |
| 300体・全Renderer | Base | 18.59 ms | 6.23 ms | 36.633 ms | 5.33 ms |
| 300体・全Renderer | Optimize Game Objects | 18.497 ms (±0) | **1.537 ms** (−75.7 %) | **0 ms** | **10.255 ms** |
| 300体・全Renderer | Optimize Hierarchy | 18.307 ms (±0) | **1.504 ms** (−76.2 %) | **0 ms** | **9.977 ms** |
| 600体・1 Renderer | Base | 6.797 ms | 0.735 ms | 4.385 ms | 5.741 ms |
| 600体・1 Renderer | Optimize Game Objects | **5.991 ms (−11.9 %)** | 0.207 ms | **0 ms** | 4.901 ms |
| 600体・1 Renderer | Optimize Hierarchy | **6.009 ms (−11.6 %)** | 0.208 ms | **0 ms** | 5.010 ms |

CPU側の処理は、どちらの構成でも同じ割合で減っています。メインスレッドのスキニング処理が6.321 → 約1.5 ms（全Renderer）、0.735 → 約0.21 ms（1 Renderer）で、いずれも7割超の削減を達成しています。スキニング行列の計算(\`MeshSkinning.CalcMatrices\`)に至っては全スレッド合計36.6 ms / 4.4 msが**完全に消滅**しています。

ただし「全Renderer構成」では、レンダースレッドの処理がフレーム時間に対するクリティカルパスとなっているため、フレーム時間の削減は見られません。

描画負荷を下げた1 Renderer構成では、同じ削減がそのまま**12 %前後のフレーム時間短縮**として現れます。300体でも同じ傾向で、3.312 → 2.899 / 2.901 ms（−12.5 % / −12.4 %）でした。**インポート時のOptimize Game ObjectsとランタイムのOptimize Hierarchyの差は0.1 %未満**で、構造が同じになる以上、効果も同じだということがわかります。

ProfilerのTimelineでも同じことが起きています。どちらも300体・全Renderer、フレーム時間の中央値付近のフレームです。

![][image4]

図: Base（Transform 166個）。フレーム時間18.59 msのうち、メインスレッドで\`MeshSkinning.Update\`が6.23 msを占め、\`Gfx.WaitForPresentOnGfxThread\` / \`Semaphore.WaitForSignal\`が5.33 ms。 {{fig:profiler_ch2_base}}

![][image5]

図: \`AnimatorUtility.OptimizeTransformHierarchy\`適用後。\`MeshSkinning.Update\`のブロックはラベルが読めない細さまで縮む一方、\`Semaphore.WaitForSignal\`が5.33 → 10.44 msに伸び、\`PlayerLoop\`は18.58 → 18.91 msとほぼ変わっていない。減らしたCPU時間が待ち時間に置き換わっている。 {{fig:profiler_ch2_optimized}}

## **実測: Renderer全無効（アニメーション評価だけ）**

描画とスキニングを完全に外し、アニメーション評価そのものへの寄与を見ます。300体です。

表: Renderer を全無効にしたときのフレーム時間 {{table:optimize_hierarchy_norender}}

| バリアント | フレーム時間 |
| :---- | :---- |
| Base | 2.702 ms |
| Optimize Game Objects | 2.687 ms (−0.6 %) |
| Optimize Hierarchy | 2.496 ms (−7.6 %) |

クリップのサンプリングやブレンドといった評価処理はTransform階層の有無に関係なく走るため、こちらへの寄与はほぼないと言えます。このことから判明するのは、この最適化の目的は主に**スキニング行列の計算とTransformへの書き戻し**に対するもので、アニメーション評価そのものではない、ということです。

## **この章の結論**

* 階層フラット化は**スキニング行列計算(\`MeshSkinning.CalcMatrices\`)**の削減に繋がる

* インポート時(Optimize Game Objects)とランタイム(\`AnimatorUtility.OptimizeTransformHierarchy\`)で効果は同等

* どちらを選ぶかは運用の問題: 自前アセットならインポート設定で済ませる、インポート設定を触れない事情があるならランタイムAPIを使う

* **モデル自体の頂点数・骨数を制作段階で適正化しておくこと**が最優先

# **Job Threadの活用**

## **Unityのアニメーション並列化**

Animatorコンポーネント、および自動更新のPlayable Graphは、Unityが**すべてのグラフをまとめてスケジュールし、Animation Job Threadに分散**します。

問題は手動更新です。\`PlayableGraph.Evaluate()\`は**そのグラフ内部の評価処理を並列化する**もので、グラフをまたいで並列化はしません。したがって「1キャラクター \= 1 Playable Graph」という直感的な構成でN体を制御すると、\`Evaluate()\`をN回呼ぶことになり、**N個のグラフが直列に処理されます**。

これはUnityのPlayable Graphの避けられない仕様なのですが、対策はシンプルで、**N体を1つのグラフにまとめる**ことです。

CODE START csharp | 全キャラクターを 1 本のグラフに集約する {{list:shared_graph}}  
// 1 本のグラフに、キャラクターごとの出力とサブツリーをぶら下げる  
\_sharedGraph \= PlayableGraph.Create("Shared");  
\_sharedGraph.SetTimeUpdateMode(DirectorUpdateMode.Manual);

foreach (var character in characters)  
{  
    var output \= AnimationPlayableOutput.Create(\_sharedGraph, "Animation", character.Animator);  
    output.SetSourcePlayable(BuildClipSource(\_sharedGraph));   // 各キャラのミキサー  
}

\_sharedGraph.Play();

// 更新は 1 回だけ  
void Update() \=\> \_sharedGraph.Evaluate(Time.deltaTime);  
CODE END

## **実測1: 制御手法 × キャラクター数**

Rendererは全無効にして、アニメーション評価のコストだけを取り出しています。フレーム時間の中央値です。

表: 制御手法とキャラクター数ごとのフレーム時間 {{table:driver_frame_time}}

| 制御方法 | 100体 | 300体 | 600体 |
| :---- | :---- | :---- | :---- |
| Animatorコンポーネント | 0.899 ms | 2.413 ms | **5.004 ms** |
| Playable API \+ 自動更新 | 0.899 ms | 2.498 ms | **5.093 ms** |
| Playable API \+ 手動更新（1体1グラフ） | 1.395 ms | 3.925 ms | **8.396 ms** |
| Playable API \+ 手動更新（全体で1グラフ） | 0.800 ms | 2.115 ms | **4.500 ms** |

**AnimatorとPlayable自動更新はほぼ同じコストです。** 「Playable APIのほうが軽い」という言説は、少なくとも「シンプルなアニメーション制御」の条件では成立しません。Animatorのステートマシン評価（\`AnimatorControllerPlayable.PrepareFrame\`）は600体でも全スレッド合計2.624 ms、うちメインスレッドは0.161 msしかなく、この処理がボトルネックになる可能性は極めて低いと言えます。（ただし、2 msを大きい/小さいどちらと捉えるかはゲーム次第だと考えます。）

**手動更新はグラフの構造で1.87倍変わります。**同じ600体でも、グラフのまとめ方しだいで自動更新より速くも遅くもなります。

## **実測2: スレッドの使われ方**

600体、Renderer無効。Unity Profilerのキャプチャからスレッド別に集計した結果です。

表: グラフ粒度によるスレッドの使われ方の違い（600 体） {{table:graph_granularity_threads}}

|  | 1体1グラフ（600グラフ） | 600体1グラフ |
| :---- | :---- | :---- |
| フレーム時間 | **9.490 ms** | **4.928 ms** |
| メインスレッドの\`Evaluate\` | 9.028 ms | 4.556 ms |
| **アニメーション処理を担ったワーカースレッド数** | **5** | **15** |
| \`Animator.ProcessGraph\`全スレッド合計 | 0.920 ms | **19.034 ms** |
| ワーカー1本あたりの\`Animator.ProcessGraph\` | 0.32 / 0.15 / 0.03 / 0.02 / 0.002 ms | 各**約1.19 ms**（15本ほぼ均等） |

**1体1グラフのとき**、アニメーション処理はほぼ全部メインスレッド上にあります。600回の\`Evaluate()\`が順番に走っているだけで、ワーカースレッドはほとんど働いていません。

**600体1グラフのとき**、15本すべてのワーカースレッドがほぼ均等に\`Animator.ProcessGraph\`を実行しています。

![][image6]

図: 1体1グラフ（600グラフ）のTimeline。メインスレッドの\`ATB.ManualEvaluate\`が約8.6 msを占め、Jobワーカーの大半が「Idle」のまま。 {{fig:profiler_timeline_per_character}}

![][image7]

図: 600体1グラフのTimeline。メインスレッドの\`ATB.ManualEvaluate\`は約4.3 msに半減し、全ワーカーが約1.2 msずつの\`EngineJob\`を同時に実行している。 {{fig:profiler_timeline_single_graph}}

**数値の読み方に1点補足があります。**

この表の数値はUnity Profilerを接続した状態のキャプチャから取っているため、Profilerなしで走らせた計測（4.500 ms / 8.396 ms）よりわずかに重く出ています。章末のまとめ表はProfiler非接続の数値を使っています。

## **この章の結論**

表: 600 体でのフレーム時間まとめ {{table:graph_summary}}

| 構成 | 600体のフレーム時間 |
| :---- | :---- |
| 1体1グラフ \+ 手動更新 | 8.396 ms |
| Animator（自動） | 5.004 ms |
| 600体1グラフ \+ 手動更新 | **4.500 ms** |

* Animatorと自動更新のPlayable Graphは、Unityが全グラフをまとめてAnimation Job Threadに分散してくれる

* 手動更新はグラフ単位でしか並列化されない

* 手動更新を選ぶ場合は、グラフの粒度を必ず設計項目に考慮すること

* Playable APIへの置き換え自体は最適化効果はほぼない。Playable APIを用いる真の価値は「Animation Jobを差し込める」「グラフ構造と更新タイミングを制御できる」といった**アニメーション制御の柔軟性**にある

# **Animation C\# Jobsの利用とSIMD対応について**

「プロシージャルにアニメーションを制御したい」というニーズはゲーム開発中に必ず発生します。例えば「キャラクターの足を地面の形状に接地するように動かしたい」などの場合です。

この場合とりうる手段としてTwo-Bone IKという手法があります。

実際にTwo-Bone IKのようなプロシージャルアニメーション制御を実装するとき、選択肢は3つあります。

1. \`LateUpdate\`でTransformを直接書き換える

2. \`AnimationScriptPlayable\`に\`IAnimationJob\`を差し込む

3. 2に加えてBurstでコンパイルする

今回はこれらの手法のパフォーマンスに着目します。

3つとも**完全に同一の演算**を行うように実装しました。解法は1つのstaticメソッドに切り出し、LateUpdate版・Job版・Burst版がそれを共有しているため、違うのは実行される場所だけです。

## **Animation C\# Jobsとは**

Animation C\# Jobsは、\`AnimationScriptPlayable\`と\`IAnimationJob\`によって**アニメーション評価パイプラインの中に自前の処理を差し込む**仕組みです。ジョブはクリップの評価やブレンドと同じ流れの中で呼ばれ、ボーンには\`AnimationStream\`と\`TransformStreamHandle\`を通してアクセスします。

\`LateUpdate\`との違いは、コードが動く場所です。\`LateUpdate\`はメインスレッド上で、アニメーション評価が終わったあとのTransformを直接書き換えます。処理はキャラクター数分メインスレッドに直列に積み上がります。Animation Jobは評価パイプラインの内部（Animation Job Thread）で呼ばれるため、Unityは他のアニメーション評価と同じようにワーカースレッドへ分散できます。同じ演算でも、メインスレッドの負担になるかどうかが変わります。

\`AnimationStream\`経由というアクセス制約はその代償です。ジョブから見えるのは事前に束ねたハンドルのぶんだけで、任意の\`Transform\`や\`GameObject\`は触れません。

## **実装**

ジョブ本体は、\`ProcessAnimation\`で解法を呼ぶだけです。

CODE START csharp | Two-Bone IK の Animation Job（Burst 対応） {{list:two_bone_ik_job}}  
\[BurstCompile\]  
public struct TwoBoneIkJobBurst : IAnimationJob  
{  
    public TransformStreamHandle Root;  
    public TransformStreamHandle Mid;  
    public TransformStreamHandle Tip;  
    public NativeArray\<float3\> Target;   // 長さ 1。毎フレーム、メインスレッドから書き込む  
    public float Weight;  
    public int Iterations;

    public void ProcessRootMotion(AnimationStream stream) { }

    \[BurstCompile\]  
    public void ProcessAnimation(AnimationStream stream)  
    {  
        TwoBoneIkCore.SolveStreamIterations(  
            ref stream, Root, Mid, Tip, Target\[0\], Weight, Iterations);  
    }  
}  
CODE END

ボーンの読み書きは、\`TransformStreamHandle\`と\`AnimationStream\`の組で行います。

CODE START csharp | AnimationStream からボーンを読み書きする {{list:ik_solve_stream}}  
static void SolveStream(  
    ref AnimationStream stream,  
    TransformStreamHandle root, TransformStreamHandle mid, TransformStreamHandle tip,  
    float3 target, float weight)  
{  
    var rootPosition \= (float3)root.GetPosition(stream);  
    var midPosition \= (float3)mid.GetPosition(stream);  
    var tipPosition \= (float3)tip.GetPosition(stream);  
    var rootRotation \= (quaternion)root.GetRotation(stream);  
    var midRotation \= (quaternion)mid.GetRotation(stream);

    Solve(rootPosition, midPosition, tipPosition, target, rootRotation, midRotation, weight,  
        out var newRootRotation, out var newMidRotation);

    mid.SetRotation(stream, newMidRotation);  
    root.SetRotation(stream, newRootRotation);  
}  
CODE END

ハンドルは\`Animator.BindStreamTransform\`で作り、ジョブはクリップの後段としてグラフに挿入します。

CODE START csharp | Animation Job をグラフに挿し込む {{list:ik_playable_connect}}  
var job \= new TwoBoneIkJobBurst  
{  
    Root \= animator.BindStreamTransform(rootTransform),  
    Mid \= animator.BindStreamTransform(midTransform),  
    Tip \= animator.BindStreamTransform(tipTransform),  
    Target \= ikTarget,   // NativeArray\<float3\>  
    Weight \= 1f,  
    Iterations \= iterations,  
};

var ikPlayable \= AnimationScriptPlayable.Create(graph, job, 1);  
graph.Connect(clipSource, 0, ikPlayable, 0);   // clipSource \= クリップ側のミキサー  
ikPlayable.SetInputWeight(0, 1f);  
output.SetSourcePlayable(ikPlayable);  
CODE END

IKターゲットのような毎フレーム変わる入力は\`NativeArray\`に置き、メインスレッドから書き込みます。

Burstは\`BurstCompile\`属性をクラスに付与することで適用されます。

実際にBurst Compilerによる最適化が効いているかはBurst Inspectorなどのツールを用いて確認することができます。

## **実測**

300体、Renderer無効。1体あたりのIK解法回数を1 / 16 / 64と変えています。

表: IK 解法回数ごとのフレーム時間（300 体・Renderer 無効） {{table:ik_frame_time}}

| 実装 | 1回 | 16回 | 64回 |
| :---- | :---- | :---- | :---- |
| IKなし（基準） | 2.501 ms | 2.601 ms | 2.596 ms |
| LateUpdate | 2.699 ms | 4.400 ms | **9.094 ms** |
| Animation Job | 2.411 ms | 2.496 ms | **2.795 ms** |
| Animation Job \+ Burst | 2.494 ms | 2.496 ms | **2.792 ms** |

CPU時間の内訳（64回のとき）:

表: IK 実装ごとの CPU 時間の内訳（解法 64 回） {{table:ik_cpu_breakdown}}

| 実装 | IKのCPU時間（全スレッド） | うちメインスレッド | フレーム時間への上乗せ |
| :---- | :---- | :---- | :---- |
| LateUpdate | 6.910 ms | **6.910 ms（100 %）** | \+6.498 ms |
| Animation Job | 7.790 ms | 0.487 ms（6.3 %） | \+0.199 ms |
| Animation Job \+ Burst | 6.932 ms | 0.436 ms（6.3 %） | \+0.196 ms |

## **Profiler Timelineで見る実行スレッドの違い**

同じ演算がどのスレッドで実行されているかは、Unity EditorのProfilerに現れます。以下は解法64回・300体のキャプチャをProfilerウィンドウで開き、フレーム時間の中央値付近のフレームを選択したものです。Profiler接続時のキャプチャなので、上の表の数値とは完全には一致しません。

![][image8]

図: LateUpdate実装。フレーム時間8.698 msのうち\`LateBehaviourUpdate\`が6.63 msを占める。ワーカースレッド15本はフレームの大半（約6.8〜7.7 ms）が\`Idle\`のまま。 {{fig:profiler_ik_lateupdate}}

![][image9]

図: Animation Job実装。同じ演算が\`AnimationScriptPlayable.ProcessAnimation\`としてワーカースレッド15本に分散し、1本あたり約0.5 msずつ実行されている。フレーム時間は2.796 ms。 {{fig:profiler_ik_animationjob}}

LateUpdate実装では、300体分の\`ATB.TwoBoneIk.LateUpdate\`（合計6.577 ms）がすべてメインスレッドの\`LateBehaviourUpdate\`の中にあります。このときワーカースレッドが担っているのはベースのアニメーション評価（\`Animator.ProcessGraph\`が1本あたり0.12〜0.29 ms）だけで、IKの計算は1つも載っていません。

Animation Job実装では\`AnimationScriptPlayable.ProcessAnimation\`の全スレッド合計が7.891 msで、うちメインスレッドは0.498 msです。残りの7.393 msはワーカースレッド15本にほぼ均等（1本あたり0.47〜0.51 ms）に分散しています。総CPU時間はLateUpdate実装より多いものの、それが16本のスレッドに分かれるため、フレーム時間は8.698 → 2.796 msに収まります。

## **読み取れること**

**このケースの最適化について、最も効果的なことはBurstではなく、実行スレッドを移すことです。** LateUpdate実装はIKのCPU時間を丸ごとメインスレッドで払うため、64回ではフレーム時間が2.596 → 9.094 msに膨らみます。同じ演算をAnimation Jobに移すとメインスレッドの負担はほぼ消え、フレーム時間への上乗せで**33倍の差**がつきます。

一方で、**総CPU時間ではLateUpdate版のほうが少なくなっています**。Animation Jobには\`AnimationScriptPlayable\`ノードごとの固定コストがあり、300体分では約2.16 msです。それでも並列化の利得がはるかに上回ります。

**実運用に近い「1体1回」の条件では、3実装の差はほぼ測定ノイズの範囲**です。300体程度のTwo-Bone IKなら、どの実装を選んでも体感差はありません。

## **Burstは属性を付けるだけでは効かない**

\`\[BurstCompile\]\`を付ければSIMDになって数倍速くなる、という理解は誤りです。今回の実測では、Burstによる総CPU時間の削減は11.0 %です。ノード固定コストを差し引いた「解法1回あたりの限界コスト」で見ると、

* Burstなし: (7.790 − 2.157) / 63 \= **0.0894 ms**

* Burstあり: (6.932 − 2.167) / 63 \= **0.0756 ms**（−15.4 %）

Burstが行うのはLLVMによるネイティブコード生成で、SIMD命令が出るかどうかは書かれたコードの形で決まります。**適切にSIMD化されるコードを実装しなければ、大きな恩恵は得られません。**

Animation C\# Jobsには、SIMD化を難しくする性質が2つあります。

**\`AnimationStream\`の読み書きが処理の中に頻繁に現れます。**解法1回ごとに\`GetPosition\`・\`GetRotation\`でボーンを読み、\`SetRotation\`で書き戻します。反復のたびにストリームの状態が変わるため反復間に依存関係が残り、複数回分を束ねて1つのベクトル命令にはできません。さらに\`IAnimationJob\`が受け取る\`AnimationStream\`は1体分なので、「4体分を同時に計算する」ためのデータがそもそもジョブの中にありません。

**超越関数の利用が多発します。**余弦定理から曲げ角を得る\`acos\`、\`quaternion.AxisAngle\`が内部で呼ぶ三角関数、\`math.length\`の平方根。これらはレーンをまたぐ計算やスカラーのライブラリ呼び出しになり、ベクトル化を止めます。

BurstのLLVMも、解法を繰り返し呼ぶループを**ベクトル化できないと明示的に報告します**（最適化リマーク\`loop not vectorized: instruction return type cannot be vectorized\`）。

## **Burstの恩恵をフル活用するための作戦**

Burstの恩恵をフル活用したいなら、\`C\# Job System\`を使って**\`AnimationStream\`を介さずにTransformを読み書きする**構成が候補になります。Transformの読み書きは\`TransformAccessArray\`と\`IJobParallelForTransform\`でジョブ側にまとめられますし、ボーンの収集と書き戻しだけをAnimation Jobに任せ、重い計算はN体分をまとめたバッチジョブに切り出す形も取れます。どちらもキャラクター単位ではなく**N体分のデータがひとつのジョブの中に揃う**ため、SIMD化の前提が成立します。

そのうえで、バッチジョブの中身は次のように整える必要があります。

* 1成分1配列のSoA（Structure of Arrays）にして、キャラクターをメモリ上で連続させる

* 分岐を\`math.select\`や\`math.max\`に置き換え、ループ本体を1本の直線にする

* 超越関数をループから追い出す（恒等式で書き換える、テーブル化する、近似する）

* \`float3\`ではなく\`float4\`で「4体分を1レーンずつ」書き、自動ベクトル化の判断に依存しない

代償は設計の複雑さです。\`IAnimationJob\`は、グラフに1ノード挿すだけで評価パイプラインの正しいタイミングでボーンの最新状態を触れる、という手軽さが最大の利点で、この作戦はそれを捨てることになります。本記事ではこの構成の実測を行っていないため、どれだけ速くなるかはここでは示しません。Burstに数倍の効果を期待するならこの方向の設計変更が前提になる、という提案として扱ってください。

## **この章の結論**

* Animation Jobの価値は**Animation Job Threadが活用できる**こと

* 演算量が少ないうちは\`AnimationScriptPlayable\`のノード固定コストのほうが大きい。素直にLateUpdateで書いたほうが総CPU時間は少ない

* \`\[BurstCompile\]\`は付けるだけではSIMDにならない。SIMDが適用されることを見越したデータ・処理の設計を行う必要がある

# **アニメーションLODの実装**

## **仕組みと効果的な場面**

ゲーム仕様にもよりますが、基本的には遠くのキャラクターまで毎フレーム更新する必要はありません。Nフレームに1回だけ更新すれば、そのキャラクターの平均負荷は1/Nになります。これが「アニメーションLOD」です。メッシュのLODが描画コストを距離で段階制御するのと同じ発想を、**アニメーションの更新頻度**に適用します。

効果的なのは次のような場面です。

* 画面内に数百体規模のキャラクターがいて、大半は遠景やモブである

* 遠いキャラクターの30 fps / 15 fps相当の動きが体感上許容できる（カメラが遠いほど間引きは気づかれにくい）

* アニメーション評価がフレーム時間の中で無視できない割合を占めている

逆に、近距離でカメラに大写しになるキャラクターに適用するとカクつきがすぐ分かります。距離やスクリーン占有率に応じて間引き段階を切り替えるのが想定される実運用でしょう。

## **シンプルな実装: \`Animator.Update()\`を自分で呼ぶ**

\`animator.enabled \= false\`にしてUnityの自動更新を止め、必要なフレームだけ\`Animator.Update()\`を明示的に呼びます。飛ばしたフレーム分の時間は溜めて渡します。

**この実装にはこれまで説明してきたAnimation Job Threadでの並列性を失うという副作用があります**。

CODE START csharp | Animator.Update() を自分で呼ぶ間引き実装 {{list:animlod_manual_update}}  
animator.enabled \= false;   // 初期化時。Unity の自動更新を止める

void Update()  
{  
    foreach (var character in \_characters)  
    {  
        character.PendingDeltaTime \+= Time.deltaTime;  
        if (\_frameIndex % interval \!= character.Slot) continue;   // Slot \= index % interval

        character.Animator.Update(character.PendingDeltaTime);    // 溜めた時間で明示更新  
        character.PendingDeltaTime \= 0f;  
    }  
    \_frameIndex++;  
}  
CODE END

## **Playable APIを直接用いた実装例**

手動更新（\`DirectorUpdateMode.Manual\`）のPlayable Graphなら、\`Evaluate()\`の呼び出しそのものを間引くだけです。

CODE START csharp | 手動更新グラフの Evaluate() を間引く実装 {{list:animlod_manual_evaluate}}  
graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);  
graph.Play();

void Update()  
{  
    foreach (var character in \_characters)  
    {  
        character.PendingDeltaTime \+= Time.deltaTime;  
        if (\_frameIndex % interval \!= character.Slot) continue;

        character.Graph.Evaluate(character.PendingDeltaTime);  
        character.PendingDeltaTime \= 0f;  
    }  
    \_frameIndex++;  
}  
CODE END

1つ注意があります。前章で推奨した「全キャラクターを1グラフに集約する」構成では、グラフ全体を1回の\`Evaluate()\`で評価するため、**キャラクター単位の間引きが表現できません**。上のコードのように1体1グラフにすれば間引けますが、そのぶん集約による並列化を失います。

どちらの実装でも重要なのが\`Slot\`（位相）です。全キャラクターを同じフレームに更新すると、そのフレームだけ負荷が集中します。\`index % interval\`で位相をずらせば、毎フレーム1/N体ずつが更新される形になります。

## **実測**

600体、Renderer無効。まずAnimatorでの実装です。

**平均値**は12秒間の総負荷を、**中央値とp95の開き**はフレーム時間の暴れ具合を表します。

表: 間引きと位相ずらしのフレーム時間統計（Animator・600 体） {{table:animlod_animator_stats}}

| 方式 | 平均 | 中央値 | p95 | p99 | 最大 | 標準偏差 |
| :---- | :---- | :---- | :---- | :---- | :---- | :---- |
| 毎フレーム更新（Animator自動） | 5.037 ms | 5.000 ms | 5.303 ms | 5.707 ms | 16.901 ms | 0.433 |
| 2フレームに1回・位相なし | 4.781 ms | 0.648 ms | 9.607 ms | 9.816 ms | 12.194 ms | 4.544 |
| 2フレームに1回・位相ずらし | 4.831 ms | 4.803 ms | 5.099 ms | 5.212 ms | 8.804 ms | 0.171 |
| 4フレームに1回・位相なし | 2.613 ms | **0.382 ms** | **9.470 ms** | 9.658 ms | 11.171 ms | 3.863 |
| 4フレームに1回・位相ずらし | 2.578 ms | **2.593 ms** | **2.707 ms** | 2.908 ms | 11.722 ms | 0.185 |

Playable API（1体1グラフ・手動更新）でも同じ傾向です。

表: 間引きと位相ずらしのフレーム時間統計（Playable API・1 体 1 グラフ） {{table:animlod_playable_stats}}

| 方式 | 平均 | 中央値 | p95 | 標準偏差 |
| :---- | :---- | :---- | :---- | :---- |
| 毎フレーム更新 | 8.496 ms | 8.493 ms | 9.001 ms | 0.337 |
| 4フレームに1回・位相なし | 2.438 ms | 0.369 ms | 8.855 ms | 3.570 |
| 4フレームに1回・位相ずらし | 2.486 ms | **2.495 ms** | **2.613 ms** | 0.173 |

## **位相ずらしは「負荷を減らす」のではなく「平らにする」**

4分の1に間引いたときの平均値は位相の有無でほとんど変わらず、**総負荷はほぼ同じ**です。位相ずらしは仕事を減らしているわけではありません。

一方で、フレーム時間の分布には次の違いが出ています。

* 位相なし: 4フレームのうち3フレームは何もせず、1フレームで600体分をまとめて処理する。中央値は下がるがp95と標準偏差が跳ね上がる

* 位相ずらし: 毎フレーム150体ずつ処理する。中央値とp95がほぼ一致し、ばらつきが小さい

中央値0.382 msという数字だけを見て「1/13になった」と喜ぶと、実際には9.5 msのスパイクが4フレームに1回刺さるゲームができあがります。**位相ずらしは体感品質のための施策**であり、標準偏差0.185は毎フレーム更新の0.433よりも安定しています。

この分布の違いはProfilerのCPUチャートにそのまま現れます（いずれもProfiler接続時のキャプチャなので、上表の非接続の数値より重めに出ています。またチャートの縦軸スケールは自動調整のため2枚で異なります）。

![][image10]

図: 4フレームに1回・位相なし。CPUチャートが「ほぼ0 msの谷3フレーム \+ 約10 msのスパイク1フレーム」の櫛状になる。下のTimelineでは選択中の空フレーム（0.47 ms）の両隣に、600体分をまとめて処理する\`ATB.ManualAnimatorUpdate\`（10.37 ms）のフレームが写っている。 {{fig:profiler_lod_decimated}}

![][image11]

図: 4フレームに1回・位相ずらし。毎フレーム150体ずつ処理するため波形は平坦（約3.5 ms）で、スパイクが消えている。 {{fig:profiler_lod_staggered}}

## **素朴な実装は並列化を失う**

**2フレームに1回の間引きは、ほとんど何も削っていません。**更新回数を半分にしたのに、平均フレーム時間の削減はわずか**4.1 %**です。

この理由は、Profilerマーカーの内訳から分かります。

表: 素朴な間引き実装で失われる並列性 {{table:animlod_parallel_loss}}

| 方式 | 1フレームに更新する体数 | メインスレッドのアニメーション更新時間 | \`Animator.ProcessGraph\`全スレッド合計 | うちメインスレッド |
| :---- | :---- | :---- | :---- | :---- |
| 毎フレーム更新（Unityのバッチ更新） | 600 | \`Animators.Update\` 4.046 ms | 20.104 ms | 1.257 ms |
| 2フレームに1回・位相ずらし | 300 | \`ATB.ManualAnimatorUpdate\` 4.532 ms | 0.549 ms | 0.317 ms |
| 4フレームに1回・位相ずらし | 150 | \`ATB.ManualAnimatorUpdate\` 2.316 ms | 0.278 ms | 0.180 ms |

毎フレーム更新では、仕事のうちメインスレッドに残るのは6 %だけで、残りはワーカースレッドに散っています。一方\`Animator.Update()\`を明示的に呼ぶと**その場で同期的に評価される**ため、ワーカースレッドがほとんど使われません。Unityのバッチ更新が持っていた並列化を、間引きと引き換えに手放しています。

1体あたりのメインスレッドコストに直すと、

* 毎フレーム更新: 4.046 ms / 600体 \= **6.7 µs/体**（並列）

* 2フレームに1回: 4.532 ms / 300体 \= **15.1 µs/体**（直列）

* 4フレームに1回: 2.316 ms / 150体 \= **15.4 µs/体**（直列）

直列化した瞬間に1体あたり約2.3倍になり、その後は間引き倍率によらず一定です。つまり**2倍の間引きで得られる2倍の削減が、直列化による2.3倍の悪化でほぼ相殺されます**。実測の4.1 %はその残差にすぎません。

素朴な実装のままでは、4倍間引き（**−48.8 %**）でようやく実質的な効果が出ます。ただし、これは避けられる損失です。

## **並列性を保ったまま間引く**

\`Animator.Update()\`が同期的なのは避けられません。しかし**「Unityのバッチ更新に参加させる体を毎フレーム選ぶ」**という形にすれば、間引きながら並列性を保てます。2通りの実装があり、どちらもAnimator（とそのステートマシン）を使ったままです。

### **方法1: \`Animator.enabled\`を切り替える**

Unityのアニメーション更新は、その時点で有効なAnimatorを集めて1つのバッチにし、Animation Job Threadに分散します。したがって\`Animator.Update()\`を呼ぶ代わりに、**そのフレームに更新したい体だけ\`enabled \= true\`にしておけば済みます**。評価はUnityが行うので並列化はそのまま残ります。

CODE START csharp | Animator.enabled を切り替える間引き実装 {{list:animlod_enable_toggle}}  
// 初期化時。飛ばしたフレーム分は speed で取り戻す  
foreach (var character in \_characters)  
{  
    var animator \= character.Animator;  
    animator.keepAnimatorStateOnDisable \= true;   // 無効化でステートを巻き戻さない（必須）  
    animator.speed \= interval;                    // interval フレームに 1 回しか更新されないぶんを補正  
}

void Update()  
{  
    foreach (var character in \_characters)  
    {  
        var scheduled \= \_frameIndex % interval \== character.Slot;  
        if (character.Animator.enabled \!= scheduled)  
        {  
            character.Animator.enabled \= scheduled;   // Unity のバッチ更新に参加する体を選ぶだけ  
        }  
    }  
    \_frameIndex++;  
}  
CODE END

\`keepAnimatorStateOnDisable \= true\`は忘れてはいけません。既定値の\`false\`ではAnimatorを無効化するたびにコントローラのステートが巻き戻り、アニメーションが進まなくなります。

制約は\`Animator.speed\`が**レート**であることです。\`Animator.Update(pendingDeltaTime)\`のように「溜めたΔtを正確に渡す」ことはできず、更新フレームの\`Time.deltaTime × interval\`だけ進みます。フレーム時間が揺れる環境では進み方も揺れますが、平均の再生速度は保たれます。

### **方法2: 位相ごとにグラフを分ける**

前章の結論は「N体を1つの手動グラフにまとめれば並列化される」でした。これを間引きに組み合わせます。**位相の数だけグラフを作り、1フレームに1本だけ\`Evaluate()\`する**のです。1本のグラフにはN/interval体が入っているので、その1回の\`Evaluate()\`の中で並列化が効きます。

ステートマシンは\`AnimatorControllerPlayable\`としてグラフに持たせれば、Animatorコンポーネントに割り当てたときと同じ評価が走ります。

CODE START csharp | 位相ごとにグラフを分ける間引き実装 {{list:animlod_bucketed_graphs}}  
// 初期化時: 位相バケットごとに手動グラフを 1 本  
for (var bucket \= 0; bucket \< interval; bucket++)  
{  
    var graph \= PlayableGraph.Create($"Bucket\_{bucket}");  
    graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);  
    \_bucketGraphs\[bucket\] \= graph;  
}

foreach (var character in \_characters)  
{  
    var graph \= \_bucketGraphs\[character.Slot\];  
    character.Animator.runtimeAnimatorController \= null;   // 評価はグラフ側に任せる  
    var output \= AnimationPlayableOutput.Create(graph, "Animation", character.Animator);  
    output.SetSourcePlayable(AnimatorControllerPlayable.Create(graph, controller));  
}

foreach (var graph in \_bucketGraphs) graph.Play();

void Update()  
{  
    for (var bucket \= 0; bucket \< interval; bucket++) \_pendingDeltaTime\[bucket\] \+= Time.deltaTime;

    var scheduled \= \_frameIndex % interval;  
    \_bucketGraphs\[scheduled\].Evaluate(\_pendingDeltaTime\[scheduled\]);   // 1 フレームに 1 バケットだけ  
    \_pendingDeltaTime\[scheduled\] \= 0f;  
    \_frameIndex++;  
}  
CODE END

こちらは\`Evaluate()\`に**溜めたΔtをそのまま渡せる**ので、進み方が正確です。代わりにグラフを間引き段階数だけ持つことになり、LOD段階を動的に変えるにはキャラクターをグラフ間で移す（出力を作り直す）必要があります。

### **実測: 3つの実装の比較**

600体、Renderer無効。平均フレーム時間と、毎フレーム更新（5.037 ms）に対する削減率です。

表: 3 つの間引き実装のフレーム時間比較（600 体） {{table:animlod_impl_compare}}

| 実装 | 1/2間引き | 1/4間引き |
| :---- | :---- | :---- |
| \`Animator.Update()\`を明示呼び出し（位相ずらし） | 4.831 ms（−4.1 %） | 2.578 ms（−48.8 %） |
| \`Animator.enabled\`の切り替え | **3.350 ms（−33.5 %）** | **1.835 ms（−63.6 %）** |
| 位相ごとの手動グラフ（\`AnimatorControllerPlayable\`） | **3.082 ms（−38.8 %）** | **1.541 ms（−69.4 %）** |

**素朴な実装がいちばん割に合いません。**とくに1/2間引きでの差が大きく、−4.1 %が−33.5 % / −38.8 %になりました。「2倍の間引きは並列化の損失に食われる」という前節の結論は、**実装を変えれば成立しません**。

並列性が保たれていることはProfilerマーカーで確認できます。

表: 3 つの間引き実装のスレッド別内訳 {{table:animlod_impl_threads}}

| 方式 | \`Animator.ProcessGraph\`全スレッド合計 | うちメインスレッド | メインスレッド側の更新時間 |
| :---- | :---- | :---- | :---- |
| 毎フレーム更新（600体/フレーム） | 20.104 ms | 1.257 ms | \`Animators.Update\` 4.046 ms |
| \`Animator.Update()\`・1/4（150体/フレーム） | 0.278 ms | 0.180 ms | \`ATB.ManualAnimatorUpdate\` 2.316 ms |
| \`Animator.enabled\`・1/4（150体/フレーム） | **4.424 ms** | 0.288 ms | \`Animators.Update\` 1.104 ms ＋ 切り替え0.125 ms |
| 位相ごとの手動グラフ・1/4（150体/フレーム） | **4.253 ms** | 0.279 ms | \`ATB.BucketEvaluate\` 1.278 ms |

新しい2実装では、**\`Animator.ProcessGraph\`の仕事量が毎フレーム更新のちょうど1/4**になり（1フレームに150体しか評価しないので当然です）、その仕事は依然としてワーカースレッドに散っています。一方\`Animator.Update()\`版はこのマーカーにほとんど計上されず、仕事の本体はメインスレッドの\`ATB.ManualAnimatorUpdate\`の中にあります。

1体あたりのメインスレッドコストで並べると差がはっきりします。

* 毎フレーム更新: 4.046 ms / 600体 \= **6.7 µs/体**

* \`Animator.enabled\`切り替え・1/4: 1.229 ms / 150体 \= **8.2 µs/体**

* 位相ごとの手動グラフ・1/4: 1.278 ms / 150体 \= **8.5 µs/体**

* \`Animator.Update()\`・1/4: 2.316 ms / 150体 \= **15.4 µs/体**

新しい2実装は1体あたりのコストがほぼ毎フレーム更新のままです（差は切り替えや\`Evaluate\`の呼び出しオーバーヘッド）。そのため間引き倍率がそのまま削減率に近づきます。

再生速度が保たれていることも確認しました。最後にスポーンしたキャラクター（位相スロットが最大の体）の指先ボーンが1秒あたりに動く距離を測ると、毎フレーム更新0.509 m/sに対し\`Animator.enabled\`切り替え0.496 m/s、位相ごとの手動グラフ0.517 m/sです。間引きで動きが直線的に近似されるぶんの誤差の範囲で、どちらも速度が変わっていません。

分布については1点注意があります。1/2間引きの手動グラフ版はp99が9.102 ms、標準偏差1.086と、\`enabled\`切り替え版（p99 4.100 ms、標準偏差0.562）より裾が重く出ました。1/4では逆に手動グラフ版のほうが安定します（標準偏差0.306対0.340）。グラフの本数と1本あたりの体数の兼ね合いで挙動が変わるので、採用するなら平均だけでなくp99も見てください。

どちらを選ぶかは要件次第です。Animatorのステートマシンとインスペクタのワークフローをそのまま使い続けたいなら\`Animator.enabled\`の切り替えが圧倒的に手軽です（数行で済み、既存コードへの影響もほぼありません）。進み方の正確さや、Animation Jobを差し込むといったグラフ側の制御が要るなら位相ごとの手動グラフです。**どちらにしても\`Animator.Update()\`を自分で呼ぶ実装を選ぶ理由はありません。**

## **この章の結論**

* アニメーションLODは「更新頻度の間引き \+ 位相ずらし」のセットで設計する

* 位相ずらしは総負荷を減らさない（平均2.613対2.578 ms）。減らすのは**フレーム時間のばらつき**（標準偏差3.863 → 0.185）であり、これは体感品質のための施策

* **\`Animator.Update()\`を自分で呼ぶ実装は選ばない。**その場で同期評価されるためジョブ並列化を失い、1体あたりのメインスレッドコストが6.7 → 15.4 µsと約2.3倍になる

* 代わりに**Unityのバッチ更新に参加する体を毎フレーム選ぶ**。\`Animator.enabled\`の切り替え（\`keepAnimatorStateOnDisable \= true\`と\`Animator.speed\`での補正が必要）か、位相ごとに手動グラフを分けて\`AnimatorControllerPlayable\`を載せる形。1体あたり8.2〜8.5 µsに収まり、並列化も残る

* 実装を変えると**2倍の間引きでも効く**: −4.1 %が−33.5 % / −38.8 %になった。4倍なら−63.6 % / −69.4 %

* キャラクター単位の間引きは1本の集約グラフでは表現できない。ただし「位相の数だけグラフを分ける」なら並列化と更新頻度制御を両立できる

* 中央値だけで判断しないこと。総負荷は平均で、体感はp95・p99と標準偏差で見る

{{pagebreak}}

# **まとめ**

各章の実測結果です。

表: 各章の実測結果まとめ {{table:overall_summary}}

| 章 | 施策 | 条件 | 結果 |
| :---- | :---- | :---- | :---- |
| アセットの設定 | Optimize Game Objects | 600体・Renderer 1個 | 6.797 → 5.991 ms（**−11.9 %**） |
| アセットの設定 | Optimize Hierarchy（ランタイム） | 600体・Renderer 1個 | 6.797 → 6.009 ms（**−11.6 %**） |
| アセットの設定 | 上記2手法 | 300体・全Renderer | 18.205 → 18.3〜18.5 ms（改善なし・描画律速） |
| Job Thread | 手動更新グラフを1本に集約 | 600体 | 8.396 → 4.500 ms（**−46.4 %**） |
| Animation C\# Jobs | Two-Bone IKをAnimation Job化 | 300体・64回解法 | 9.094 → 2.795 ms（**−69.3 %**） |
| Animation C\# Jobs | さらにBurstでコンパイル | 300体・64回解法 | 2.795 → 2.792 ms（フレーム時間は差なし） |
| アニメーションLOD | 1/4間引き・\`Animator.Update()\`を明示呼び出し | 600体 | 平均5.037 → 2.578 ms（**−48.8 %**） |
| アニメーションLOD | 1/4間引き・位相ごとの手動グラフ | 600体 | 平均5.037 → 1.541 ms（**−69.4 %**） |
| アニメーションLOD | 1/2間引き・\`Animator.Update()\`を明示呼び出し | 600体 | 平均5.037 → 4.831 ms（−4.1 %） |
| アニメーションLOD | 1/2間引き・位相ごとの手動グラフ | 600体 | 平均5.037 → 3.082 ms（**−38.8 %**） |

## **最終的な最適化アプローチ**

この記事の実測を踏まえた、大量キャラクターのアニメーション最適化の進め方です。

**1\. アセット設定を最適化する**

まずはモデル自体の頂点数や骨数の仕様を適切な値に定めましょう。基準となるサポート端末でベンチマークを計測し、適切なモデルレギュレーションを定めることがまずは理想です。そのうえでUnity側でOptimize Game Objectsの適用を検討しましょう。3Dタイトルにおいて、上流に位置するモデル制作を量産が進んだプロジェクト終盤で大きく修正していくことは難しいため、プロジェクト序盤に行えることが最適です。

**2\. Animation Job Threadでの処理の並列性を高める**

 Animatorと自動更新のPlayable GraphはUnityが自動で並列化しますが、手動更新はグラフ単位でしか並列化されません。キャラクター制御の設計を行う時点で「Animation Job Threadを有効活用できるか」を念頭に入れましょう。

**3\. Animation JobのBurst適用を含めた最適化を行う**

 IKなどの後処理はまずメインスレッドからAnimation Jobへ移します。

さらにパフォーマンスが必要とされる場合はSIMD化を適用できるデータ・処理設計を適用しましょう。

**4\. アニメーションLODなどのアプリケーション的な工夫を行う**

アセット・エンジン設定・コード最適化を尽くしたら、最後は「そもそもアニメーションを処理しない」という工夫です。

プロジェクトの要件と相談になりますが、ゲーム体験的に許容できるラインを探りながら処理を抑える仕組みを検討しましょう。

{{pagebreak}}

# **脚注**

## **Unity公式ドキュメント**

* \[Playable API\](https://docs.unity3d.com/Manual/Playables.html) — Playable Graphの構築と\`DirectorUpdateMode\`

* \[\`AnimatorUtility.OptimizeTransformHierarchy\`\](https://docs.unity3d.com/ScriptReference/AnimatorUtility.OptimizeTransformHierarchy.html) — ランタイムでの階層フラット化

* \[Model Import Settings: Rig（Optimize Game Objects）\](https://docs.unity3d.com/Manual/FBXImporter-Rig.html)

* \[\`IAnimationJob\`\](https://docs.unity3d.com/ScriptReference/Animations.IAnimationJob.html) — \`AnimationScriptPlayable\`に差し込むジョブ

* \[\`AnimationScriptPlayable\`\](https://docs.unity3d.com/ScriptReference/Animations.AnimationScriptPlayable.html)

* \[C\# Job System\](https://docs.unity3d.com/Manual/JobSystem.html) — \`AnimationStream\`を介さずに並列処理を書くための仕組み

## **使用アセット**

* \[ユニティちゃん3Dモデルデータ\](https://unity-chan.com/) — © Unity Technologies Japan/UCL

[image1]: assets/saito-shunsuke-1/media/image1.png

[image2]: assets/saito-shunsuke-1/media/image2.png

[image3]: assets/saito-shunsuke-1/media/image3.png

[image4]: assets/saito-shunsuke-1/media/image4.png

[image5]: assets/saito-shunsuke-1/media/image5.png

[image6]: assets/saito-shunsuke-1/media/image6.png

[image7]: assets/saito-shunsuke-1/media/image7.png

[image8]: assets/saito-shunsuke-1/media/image8.png

[image9]: assets/saito-shunsuke-1/media/image9.png

[image10]: assets/saito-shunsuke-1/media/image10.png

[image11]: assets/saito-shunsuke-1/media/image11.png