* GraphicsStateCollectionによるシェーダーウォームアップ  
* 天野 択郎/Amano Takuro/株式会社サイバーエージェント SGEコア技術本部

# 

シェーダーが初めて描画に使用されるとき、ランタイムでコンパイル処理が実行され、シェーダーコンパイルスタッターと呼ばれる処理落ちを引き起こすことがあります。本章では、モバイル環境（Metal・Vulkan）を対象に、GraphicsStateCollectionによるシェーダーウォームアップについて基本的な使い方から導入後にスタッターが残る実例とその対処までを紹介します。なお、本章の内容はUnity 6.3 LTS（6000.3）を対象としています。

# **GraphicsStateCollectionの概要**

GraphicsStateCollection（GSC）は、Unity 6で使用できるようになったAPIです{{fn:gsc_docs}}。ゲームプレイ中に使用されるシェーダーバリアントとグラフィックス状態（描画時のレンダリング設定）の組み合わせを記録し、まとめて事前コンパイル、つまりウォームアップする仕組みです。Metal・Vulkan・D3D12などのモダングラフィックスAPI向けの、ShaderVariantCollectionの代替として位置づけられています。なお、Unity 6000.3時点では\`UnityEngine.Experimental.Rendering\`名前空間に属するExperimental APIであり、将来のバージョンで仕様が変更される可能性があります。  
![][image1]  
図: GSCに記録される情報の例 {{fig:gsc_recorded_info}}

図は、GSCに記録された情報をスクリプトで取得し、EditorWindowに表示した例です。記録内容の取得方法は、本章の最後（{{list:gsc_variant_list}}）で紹介します。

シェーダーはそのままではGPUで実行できず、GPUのマシン命令へのコンパイルが必要です。ただし、マシン命令はGPUの機種やドライバによって異なるため、すべての端末向けに事前コンパイルした状態で配布することはできません。そのため、シェーダーは以下の流れを経てGPUで実行されます。

1\. 開発者は、HLSLなどのシェーダー言語でシェーダー（アセット）を記述する  
2\. ビルド時に、UnityがシェーダーをグラフィックスAPIごとの中間的な表現へコンパイルし、アプリやAssetBundleに格納する。VulkanではSPIR-V、MetalではMetal Shading Language（MSL）がこの中間的な表現にあたる{{fn:gsc_spirv_msl}}  
3\. アプリの実行中、シェーダーが初めて描画に使用されるときに、格納された中間的な表現からシェーダープログラムを生成する  
4\. 続けて、シェーダープログラムとレンダリング設定を組み合わせて、GPUドライバがGPUのマシン命令へコンパイルする

手順3・4の実行時コンパイルを初回描画より前にまとめて済ませておくことが、シェーダーウォームアップの基本的な考え方です。

なお、1つのシェーダーからは、パス（描画工程）ごと、さらにライトの影響やフォグの有無といった条件（シェーダーキーワード）の組み合わせごとに、複数の実行プログラムが生成されます。この生成される単位をシェーダーバリアントと呼びます。

## **Pipeline State Object**

モダングラフィックスAPIにおいて描画時にコンパイルされる単位を、Pipeline State Object（PSO）と呼びます。PSOは、シェーダーバリアントとレンダリング設定（頂点レイアウト、カラーフォーマットやMSAAなどのレンダーパスアタッチメント、ブレンドステート、デプステスト、ステンシルステート）を一体として、GPUドライバがGPU向けのマシン命令にコンパイルしたオブジェクトです。生成されたPSOは端末にキャッシュされ、同じレンダリング設定であれば再利用されます。つまり、同じシェーダーバリアントであっても、レンダリング設定が異なれば別のPSOとしてコンパイルされます。

## **シェーダーコンパイルスタッターの発生原因**

スタッターの原因は、ランタイムで実行される2段階のコンパイル処理です。GPUが初めてシェーダーバリアントを描画するとき、Main Threadでのシェーダープログラムの生成（手順3）と、Render ThreadでのGPUドライバによるPSO生成（手順4、マシン命令へのコンパイル）が発生します。

Unity Profilerでは、前者はMain Threadの\`Shader.CreateGPUProgram\`、後者はRender Threadの\`GpuProgramMetal.GetCachedPipeline\`（iOS）または\`CreateGraphicsPipelineImpl\`（Android）として観測できます。特にMetal（iOS）・Vulkan（Android）ではPSO生成コストが大きく、初回描画時のスタッターの主因になります。GSCを使ったウォームアップは、このPSO生成をゲーム起動直後などの任意のタイミングで済ませておく手段です。
  
従来のShaderVariantCollectionは「どのシェーダーバリアントをコンパイルするか」は管理できますが、レンダリング設定を持ちません。そのためShaderVariantCollectionのウォームアップは手順3のシェーダープログラム生成までにとどまり、実際の描画時にはGPUドライバが改めてPSOを生成するため、スタッターが残ります。  

![][image2]![][image3]  
図: Main ThreadとRender Threadでのコンパイル発生例 {{fig:gsc_profiler_stutter}}

図は、Vulkan（Android）でコンパイルが発生したフレームを、ProfilerのTimelineビューで表示した例です。上はMain Threadの\`Shader.CreateGPUProgram\`を選択した様子で、対象のシェーダー名・パス名・キーワードが確認できます。下はRender Threadの\`CreateGraphicsPipelineImpl\`を選択した様子で、この例では1回のPSO生成に数十ミリ秒かかっており、スタッターの主因になることがわかります。

# **GSCを使ったウォームアップのフロー**

GSCを使ったウォームアップは、「レンダリング設定の収集（トレース）」と「ウォームアップ（事前コンパイル）」の2つのフェーズに分かれます。

## **レンダリング設定の収集**

トレースの基本形を {{list:gsc_trace_basic}} に示します。トレース中に描画されたオブジェクトのレンダリング設定が、\`GraphicsStateCollection.GraphicsState\`としてGSCに記録されます。リストでは1つのメソッドにまとめていますが、実際には\`BeginTrace()\`と\`EndTrace()\`は別々のタイミングで呼び、複数フレームにわたって描画を収集します。

CODE START csharp | レンダリング設定のトレース {{list:gsc_trace_basic}}  
private GraphicsStateCollection \_gsc;

void Trace() {  
    // 収集開始  
    \_gsc \= new GraphicsStateCollection();  
    \_gsc.BeginTrace();

    // ここで複数フレームにわたり、ウォームアップ対象のオブジェクトを描画する  
    // 描画されたレンダリング設定がGraphicsStateとして記録される

    // 収集終了・保存  
    \_gsc.EndTrace();  
    \_gsc.SaveToFile(path);  
}  
CODE END

生成される\`GraphicsStateCollection.GraphicsState\`はグラフィックスAPIごとに異なります。トレースは、ビルドしたアプリを実際に動かすデバイス上で行いましょう。

## **ウォームアップ（事前コンパイル）**

収集したGSCをロードし、ウォームアップを実行します（{{list:gsc_warmup_progressive}}）。この例では、プロジェクトに取り込んだGSCをAddressablesのアセットとしてロードしています。

CODE START csharp | WarmUpProgressivelyによる分散ウォームアップ {{list:gsc_warmup_progressive}}  
async Task WarmUpAsync() {  
    // シェーダーを含むアセットのロード後に実行する  
    \_gsc \= await Addressables.LoadAssetAsync\<GraphicsStateCollection\>(address).Task;

    // 段階的にWarmUp（Jobsベース。毎フレームn件ずつコンパイル）  
    StartCoroutine(WarmUpCoroutine());  
}

IEnumerator WarmUpCoroutine() {  
    JobHandle handle \= default;  
    int requestedCount \= 0;  
    // エラー時にisWarmedUpが真にならないケースがあるため、依頼件数で終了判定する  
    while (requestedCount \< \_gsc.totalGraphicsStateCount) {  
        handle \= \_gsc.WarmUpProgressively(4, handle);  
        handle.Complete();  
        requestedCount \+= 4;  
        yield return null;  
    }  
}  
CODE END

ウォームアップを実装する際のポイントは以下のとおりです。

\* \`WarmUp(dependency)\`でもウォームアップを実行できますが、一括で全てのコンパイルを行うためスパイクが発生します。\`WarmUpProgressively(count, dependency)\`で1回あたりのコンパイル件数を指定し、複数フレームに分散させましょう  
\* シェーダーアセットのロード後に実行します。シェーダーのインスタンスが存在しない場合、そのバリアントのウォームアップはスキップされます  
\* 完了判定は\`isWarmedUp\`や\`completedWarmupCount \== totalGraphicsStateCount\`で確認できます  
\* ただし、ウォームアップエラーが発生すると\`isWarmedUp\`が真にならない場合があります（Unity 6000.3で確認）。このため、リストでは依頼件数の合計と\`totalGraphicsStateCount\`の比較で終了判定し、無限ループを回避しています

# **収集シナリオの実装例**

アプリ起動時に\`BeginTrace()\`を呼び、手動でプレイしてレンダリング設定を収集する方法もあります。しかしアプリの規模によっては、バージョンアップのたびに手動で網羅するのは困難です。そこで、収集専用シーンでアセットを順次描画するシナリオを機械的に実行して収集する方法を紹介します。実装例にはAddressablesとUniTask{{fn:gsc_unitask}}を使用します。これらを使用していない場合も、アセットの非同期ロードと1フレームの描画待ちを自身のプロジェクトの仕組みに置き換えることで、同じ流れを実装できます。全体のフローは以下のとおりです。

1\. 必要なアセットをダウンロードする（Built-inアセットのみで完結する場合は不要）  
2\. 収集に必要なアセットのAddressをAddressablesのコンテンツカタログ（\`IResourceLocator\`）から取得する  
3\. Quality Settingsを切り替える（Quality Settingsでシェーダーが変化しない場合は切り替え不要）  
4\. 収集を開始する（\`BeginTrace()\`）  
5\. シーンを加算ロードし、収集対象アセットを順次ロードして描画する（モデル描画・\`ParticleSystem\`再生・Timeline再生）  
6\. 収集を完了する（\`EndTrace()\`）  
7\. 不要なシェーダーをGSCから削除する  
8\. GSCを保存する（\`SaveToFile(path)\`）  
9\. シーンを破棄し、Quality Settingsを切り替えながら3〜8を繰り返す

コンテンツカタログからレンダリング設定に影響するアセット（シーン）、キャラクターやプロップなどのモデルアセット、エフェクトアセット、Timelineアセットを取得し、レンダリング設定を切り替えながら順次描画することで網羅的に収集します。Quality Settingsごとに収集するのは、Quality LevelによってMSAAやHDRなどPSOに影響するレンダリング設定が変わるためです。

処理の骨格を {{list:gsc_scenario_skeleton}} に示します。Quality Settingsごとにトレースを繰り返し、Quality Level名を付けて別ファイルに保存しています。実行時は、現在のQuality Levelに対応するファイルを選択してロードし、ウォームアップします。

CODE START csharp | 収集シナリオの骨格 {{list:gsc_scenario_skeleton}}  
private GraphicsStateCollection \_gsc \= new GraphicsStateCollection();

private async UniTaskVoid StartAsync() {  
    // コンテンツカタログの取得と必要アセットのダウンロード  
    // Prefixによる各種Address一覧の取得を事前に行う  
    await AddressableSetupAsync();

    var qualitySettingNames \= QualitySettings.names;  
    for (var i \= 0; i \< qualitySettingNames.Length; i++) {  
        QualitySettings.SetQualityLevel(i);  
        await TraceAsync();  
    }  
}

private async UniTask TraceAsync() {  
    // GSC収集開始  
    \_gsc.ClearVariants();  
    \_gsc.BeginTrace();

    // 収集シナリオを実行  
    await ExecuteScenarioAsync();

    // GSC収集終了  
    \_gsc.EndTrace();

    // 収集対象外のシェーダーをGSCから削除する  
    RemoveUnnecessaryVariants();

    var qualityLevelName \= QualitySettings.names\[QualitySettings.GetQualityLevel()\];  
    \_gsc.SaveToFile(\_gscSavePathFormat \+ $"\_{qualityLevelName}");  
}  
CODE END

シナリオ本体では、シーンごとにモデル・エフェクト・Timelineを順次描画します。{{list:gsc_scenario_batch}} はキャラクターモデルを数体ずつバッチで生成・描画する部分の抜粋です。1フレーム描画を待つことで、そのフレームのレンダリング設定がGSCに記録されます。

CODE START csharp | モデルアセットの順次描画（抜粋） {{list:gsc_scenario_batch}}  
// キャラクターモデルを収集  
for (var i \= 0; i \< \_charaAddressArray.Length; i \+= \_characterNum) {  
    // キャラクターモデルを数体ずつ生成  
    var batchCount \= Mathf.Min(\_characterNum, \_charaAddressArray.Length \- i);  
    for (var j \= 0; j \< batchCount; j++) {  
        var charaHandle \= Addressables.LoadAssetAsync\<GameObject\>(\_charaAddressArray\[i \+ j\]);  
        var charaPrefab \= await charaHandle.ToUniTask();  
        // カメラに収まる位置に均等配置  
        var pos \= new Vector3((j \- batchCount \* 0.5f) \* 2f, 0f, 5f);  
        \_charaObjectArray\[j\] \= Object.Instantiate(charaPrefab, pos, Quaternion.identity);  
        Addressables.Release(charaHandle);  
    }

    // 1フレーム描画待ち  
    await UniTask.NextFrame();

    for (var j \= 0; j \< batchCount; j++) {  
        DestroyImmediate(\_charaObjectArray\[j\]);  
        \_charaObjectArray\[j\] \= null;  
    }  
}  
CODE END

\`ParticleSystem\`も同様に数個ずつバッチで生成しますが、\`Simulate()\`で再生途中の状態（この例では0.5秒後）を同期的に再現してから描画する点が異なります（{{list:gsc_scenario_particle}}）。実時間の再生を待つ必要がないため、収集時間を短縮できます。

CODE START csharp | ParticleSystemの収集（抜粋） {{list:gsc_scenario_particle}}  
// ParticleSystemを収集  
for (var i \= 0; i \< \_particlesAddressArray.Length; i \+= \_effectNum) {  
    var batchCount \= Mathf.Min(\_effectNum, \_particlesAddressArray.Length \- i);  
    for (var j \= 0; j \< batchCount; j++) {  
        var particleHandle \= Addressables.LoadAssetAsync\<GameObject\>(\_particlesAddressArray\[i \+ j\]);  
        var particlePrefab \= await particleHandle.ToUniTask();  
        var pos \= new Vector3((j \- batchCount \* 0.5f) \* 3f, 0f, 5f);  
        \_particleObjectArray\[j\] \= Object.Instantiate(particlePrefab, pos, Quaternion.identity);  
        Addressables.Release(particleHandle);

        // \_effectSimulateTime秒後の状態を同期処理で再現（Playは不要）  
        var ps \= \_particleObjectArray\[j\].GetComponentInChildren\<ParticleSystem\>();  
        if (ps \!= null) {  
            ps.Simulate(\_effectSimulateTime, withChildren: true, restart: true);  
        }  
    }

    // 1フレーム描画待ち  
    await UniTask.NextFrame();

    for (var j \= 0; j \< batchCount; j++) {  
        DestroyImmediate(\_particleObjectArray\[j\]);  
        \_particleObjectArray\[j\] \= null;  
    }  
}  
CODE END

Timelineを使った演出は、\`PlayableDirector\`で実際に再生して収集します（{{list:gsc_scenario_timeline}}）。再生時間の分だけ待つため、対象のTimelineアセットは収集に必要なものへ絞るのがよいでしょう。

CODE START csharp | Timelineの収集（抜粋） {{list:gsc_scenario_timeline}}  
// Timelineを収集  
foreach (var timelineAddress in \_timelineAddressArray) {  
    var timelineHandle \= Addressables.LoadAssetAsync\<GameObject\>(timelineAddress);  
    var timelinePrefab \= await timelineHandle.ToUniTask();  
    var timelineObj \= Object.Instantiate(timelinePrefab, Vector3.zero, Quaternion.identity);  
    Addressables.Release(timelineHandle);  
    var director \= timelineObj.GetComponent\<PlayableDirector\>();  
    if (director \!= null) {  
        director.Play();  
        await UniTask.Delay((int)(director.duration \* 1000));  
    }  
    DestroyImmediate(timelineObj);  
}  
CODE END

収集が終わったら、ウォームアップ対象にしたいシェーダーだけを残します（{{list:gsc_remove_variants}}）。残すシェーダーを許可リスト（\`\_allowShaderNames\`）で指定し、対象外のシェーダーと参照切れ（null）のバリアントをGSCから削除しています。

CODE START csharp | 収集対象外のバリアント削除 {{list:gsc_remove_variants}}  
private void RemoveUnnecessaryVariants() {  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    \_gsc.GetVariants(variants);

    foreach (var variant in variants) {  
        if (variant.shader \== null) {  
            \_gsc.RemoveVariant(null, variant.passId, variant.keywords);  
            continue;  
        }

        if (\_allowShaderNames.Contains(variant.shader.name)) {  
            continue;  
        }

        // 不要なシェーダーをGSCから削除  
        \_gsc.RemoveVariant(variant.shader, variant.passId, variant.keywords);  
    }  
}  
CODE END

シナリオやアセットの管理方法はプロジェクトによって異なるため、シーン構成や収集対象は自身のプロジェクトに合わせて調整してください。

# **スタッターが残る実例と対処**

GSCによるウォームアップを導入したにもかかわらずスタッターが残ることがあります。ここではその実例と対処を紹介します。

## **Shader Strippingによるバリアントの欠落**

ウォームアップ時のログに\`variant not found\`エラーが発生し、かつランタイムでも該当バリアントのコンパイル（Profilerの\`Shader.CreateGPUProgram\`など）が観測されないケースです。

原因は、GSCに含まれるシェーダーバリアントがアプリまたはAssetBundleに含まれていないことです。\*\*Project Settings \> Graphics \> Shader Stripping\*\*の設定によっては、ビルド時に使用されないバリアントが除外されます。シェーダーやアセットの変更によって必要なバリアントが変わった際に発生しやすい問題です。

Shader Strippingの設定を確認し、バリアントに影響のある変更を行った際はGSCを再度収集しましょう。

なお、GSCにバリアントは記録されているものの\`GraphicsState\`が0件の場合にも、同じ\`variant not found\`エラーが発生します。この場合はランタイムでそのバリアントのコンパイルが発生するため、ランタイムでのコンパイル発生の有無で切り分けられます。

## **メタデータ不一致によるmismatch警告**

ウォームアップ実行時に、コンソールに以下の警告が出るケースです。

CODE START text  
WarmUp may be inaccurate due to GraphicsDeviceType mismatch.  
WarmUp may be inaccurate due to RuntimePlatform mismatch.  
WarmUp may be inaccurate due to Quality Level mismatch.  
CODE END

原因は、GSCファイルに保存された\`graphicsDeviceType\` / \`runtimePlatform\` / \`qualityLevelName\`のメタデータが実行環境と一致していないことです。主な発生ケースは以下の2つです。

\* 異なるプラットフォームで収集したGSCを流用している（例: iOS実機で収集したGSCをAndroidビルドで使用）  
\* EditorスクリプトでGSCを編集・保存した際に、メタデータがEditorのPlatform設定で上書きされた

対処は発生ケースによって異なります。異なるプラットフォームで収集したGSCは、メタデータを書き換えても中身の\`GraphicsState\`が実行環境と一致しないため、該当プラットフォームで収集し直します。EditorのPlatform設定で上書きされた場合は、メタデータを実機の値へ修正します。なお、\`qualityLevelName\`の不一致は、PSOに影響するレンダリング設定（MSAAやHDRなど）やシェーダーがQuality Levelによって変化しない場合に限り無視できます。その場合は、{{list:gsc_quality_fix}} のようにウォームアップ直前に書き換えることで警告を回避できます。

CODE START csharp | qualityLevelNameの書き換えによる警告回避 {{list:gsc_quality_fix}}  
// GSCロード  
var gsc \= LoadGsc();  
// qualityLevelNameを実行環境に合わせて変更  
gsc.qualityLevelName \= QualitySettings.names\[QualitySettings.GetQualityLevel()\];  
// ウォームアップ実行  
CODE END

## **iOS（Metal）でのAssetBundle重複パッキングによる二重インスタンス**

iOS（Metal）でのみ発生するケースです。ウォームアップ時のログにエラーはなく、ProfilerのMain Threadでも\`Shader.CreateGPUProgram\`は発生していないのに、Render Threadでは\`GpuProgramMetal.GetCachedPipeline\`が発生します。

原因は、AssetBundleの重複パッキングによるシェーダーの二重インスタンス化です。同一シェーダーが複数のAssetBundleに重複してパッキングされると、Unityはメモリ上に別々のシェーダーインスタンスを生成します。ウォームアップはGSCが参照しているシェーダーインスタンスに対してのみPSOを作成するため、同じシェーダーであっても別インスタンスになっているシェーダーはウォームアップされません。なお、Vulkanではこの問題は発生しません。PSOキャッシュの管理がMetalとVulkanで異なるためと考えられます。

発生する構成の例は以下のとおりです。同一のシェーダーXが、依存関係によって2つのバンドルの両方にパッキングされています。

{{enlargepage}}

CODE START text  
\- アセット  
  \- シェーダーX  
  \- マテリアルA（シェーダーXを使用）  
  \- マテリアルB（シェーダーXを使用）  
  \- GraphicsStateCollection

\- AssetBundle構成  
  \- バンドルA  
    \- GraphicsStateCollection  
    \- マテリアルA（シェーダーXを使用）  
    \- シェーダーX ← 依存関係によるパッキング  
  \- バンドルB  
    \- マテリアルB（シェーダーXを使用）  
    \- シェーダーX ← 依存関係によるパッキング  
（シェーダーXにはアドレスが振られていない状態）  
CODE END

この構成でバンドルAとバンドルBをロードすると、以下の流れでマテリアルBの描画時にスタッターが発生します。

CODE START text  
\- ランタイムの流れ  
  1\. バンドルAとバンドルBをロード  
     \- バンドルAのシェーダーX（instanceId=100）  
     \- バンドルBのシェーダーX（instanceId=200）  
  2\. GraphicsStateCollectionのウォームアップを実行  
     \- GSCが参照しているシェーダーX（instanceId=100）でウォームアップが行われる  
  3\. マテリアルAを描画  
     \- シェーダーX（instanceId=100）のPSOキャッシュは作成済みのためスタッターなし  
  4\. マテリアルBを描画  
     \- シェーダーX（instanceId=200）のPSOキャッシュはなく、  
       Render ThreadでPSO作成が実行されスタッターが発生  
CODE END

対処は、シェーダーを単一のバンドル（シェーダー専用グループ）にまとめ、コンテンツバンドルへの暗黙のパッキングを排除することです。単一バンドルへの集約が難しい場合は、バンドルごとにGSCを用意します。重複しているシェーダーは {{list:gsc_duplicate_check}} のようなコードで確認できます。Addressablesを使っている場合は、\*\*Addressables Report\*\*ウィンドウでもシェーダーの重複を確認できます。

CODE START csharp | シェーダーインスタンスの重複確認 {{list:gsc_duplicate_check}}  
using System.Linq;

// AssetBundleロード後に実行してインスタンスの重複を確認  
var all \= Resources.FindObjectsOfTypeAll\<Shader\>();  
var duplicates \= all.GroupBy(s \=\> s.name).Where(g \=\> g.Count() \> 1);  
foreach (var g in duplicates)  
    foreach (var s in g)  
        Debug.Log($"Duplicate: {s.name} | InstanceID: {s.GetInstanceID()}");  
CODE END

## **iOS（Metal）でのSubPass使用シェーダーのウォームアップ失敗**

ここまではプロジェクト設定やバンドル設計のミスによって引き起こされる問題でした。次はGSCの仕様上発生する問題を紹介します。

1つのレンダーパスを複数の描画ステップに分割し、前のステップの描画結果をメインメモリへ書き戻さずに次のステップから参照できるようにする仕組みを、SubPassと呼びます。モバイルGPUでは、この一連のステップをチップ内の高速なメモリ領域（タイルメモリ）上で完結でき、メモリ帯域を節約できます。SubPassはMetal・VulkanではNative Render Passとして実行されます。iOS（Metal）でのみ、ウォームアップを実施しログにもエラーが出ていないのに、ゲームプレイ中にPSOコンパイルが発生するケースがあります。原因は、iOS（Metal）のウォームアップ処理が、このSubPass（Native Render Pass）内で描画されるシェーダーに対して想定どおり機能しないことです。前のSubPassの出力を読み込むinput attachmentsの有無にかかわらず発生します。なお、ウォームアップの内部処理は非公開APIのため、ここでは発生条件の紹介にとどめます。

影響を受ける条件は以下のとおりです。

\* ターゲットがiOS / Metal（Apple A/Mチップの実機）  
\* URPの描画処理基盤（RenderGraph）の設定である\`RenderGraph.nativeRenderPassesEnabled\`が有効（Unity 6000.5以降ではこの設定は廃止され、常時有効）  
\* SubPass（Native Render Pass）内で描画するシェーダーを使用している

書き込みのみのSubPass（URP DeferredのGBufferパスなど）でも、input attachmentsを持つSubPass（同Deferred Lightingパスなど）でも同様に発生します。影響を受けるSubPassの記述例として、input attachmentsを設定するケースを {{list:gsc_subpass_input}} に示します。

CODE START csharp | SubPassにinput attachmentsを設定するケース {{list:gsc_subpass_input}}  
// ScriptableRenderContext.BeginRenderPassで開始した  
// レンダーパス内で、SubPassを設定する例  
var inputAttachments \= new NativeArray\<int\>(new\[\] { 0, 1 }, Allocator.Temp);  
context.BeginSubPass(  
    colors,  
    inputAttachments,   // 前SubPassの出力バインド  
                        // Metalではframebuffer fetchに変換されるが、  
                        // GSCウォームアップ時のPSO生成には反映されない  
    false  
);  
CODE END

Unity 6000.3では根本的な対処は難しく、SubPassの利用を見直すか、プリレンダリングでプレイ中のスタッターを回避します。プリレンダリングは、対象のシェーダーを使うオブジェクトをロード画面中などに1回実際に描画し、PSOキャッシュを作っておく手法です。本章の収集シナリオと同様の順次描画を、ウォームアップの代わりにゲーム内で行うイメージです。

## **スタッターが残る場合の診断ポイント**

ここまで見てきたように、GSCを使ったシェーダーウォームアップはプロジェクトのアセット管理設計（Addressablesの構成・シェーダーキーワード定義・バンドル設計）と密接に関係するため、導入後のデバッグが必要になるケースが多くあります。ウォームアップ時のログの確認や、Profilerを使った繰り返しの計測を行い、ウォームアップ漏れがないか確認しましょう。特に以下の点を押さえておくと診断が速くなります。

\* ウォームアップ時のログにウォームアップエラーや\`variant not found\`が出ていないか  
\* ProfilerでMain Threadの\`Shader.CreateGPUProgram\`、Render Threadの\`GpuProgramMetal.GetCachedPipeline\`（iOS）/ \`CreateGraphicsPipelineImpl\`（Android）が発生していないか  
\* Shader Strippingの設定は適切か  
\* AssetBundleのパッキングは想定どおりになっているか

GSCに記録されているバリアントの確認には、次節で紹介するバリアント一覧の出力（{{list:gsc_variant_list}}）が利用できます。

# **GSCファイルの操作例**

最後に、GSCファイルの操作例を2つ紹介します。いずれもEditorスクリプトから実行する想定で、収集したGSCの中身の確認や、複数回の収集結果の統合に利用できます。

## **バリアント一覧とState数の確認**

{{list:gsc_variant_list}} は、GSCファイルをロードしてメタ情報と記録されているバリアントの一覧を出力する例です。

CODE START csharp | バリアント一覧とState数の確認 {{list:gsc_variant_list}}  
void PrintVariantInfo() {  
    // GSCのロード  
    var gsc \= new GraphicsStateCollection();  
    if (\!gsc.LoadFromFile(path))  
        return;

    // バリアントの取得  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    gsc.GetVariants(variants);

    // GSCのメタ情報を出力  
    Debug.Log($"\[GSC\] Variants={variants.Count} TotalStates={gsc.totalGraphicsStateCount} "  
            \+ $"Platform={gsc.runtimePlatform} Device={gsc.graphicsDeviceType} Quality={gsc.qualityLevelName}");

    // バリアントの詳細を出力  
    foreach (var v in variants) {  
        int stateCount \= gsc.GetGraphicsStateCountForVariant(v.shader, v.passId, v.keywords);  
        string kws \= v.keywords.Length \> 0  
            ? string.Join(" ", v.keywords.Select(k \=\> k.name))  
            : "\<no keywords\>";  
        Debug.Log($"  {v.shader?.name} | states={stateCount} | {kws}");  
    }  
}  
CODE END

## **GSCのマージ**

{{list:gsc_merge}} は、2つのGSCをマージする例です。複数回に分けて収集した結果を1つに統合できます。

CODE START csharp | 2つのGSCのマージ {{list:gsc_merge}}  
void MergeGsc() {  
    // 2つのGSCをロード  
    var src \= new GraphicsStateCollection();  
    var dst \= new GraphicsStateCollection();  
    if (\!src.LoadFromFile(srcPath) || \!dst.LoadFromFile(dstPath))  
        return;

    // バリアントを取得  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    var states \= new List\<GraphicsStateCollection.GraphicsState\>();  
    src.GetVariants(variants);

    // srcのGSCからdstのGSCに追加  
    foreach (var v in variants) {  
        // バリアントに含まれるGraphicsStateを取得  
        src.GetGraphicsStatesForVariant(v.shader, v.passId, v.keywords, states);  
        foreach (var s in states) {  
            // Stateを追加  
            dst.AddGraphicsStateForVariant(v.shader, v.passId, v.keywords, s);  
        }  
    }

    dst.SaveToFile(dstPath);  
}  
CODE END

AssetBundleの構成によっては、収集した\`GraphicsState\`を別のGSCへ移動・統合するといった対応が必要になる場合があります。二重インスタンスの対処で紹介した、バンドルごとにGSCを用意するケースが典型です。その際は、本節の操作例を組み合わせて活用してください。

[image1]: assets/amano-takuro-1/media/image1.png

[image2]: assets/amano-takuro-1/media/image2.png

[image3]: assets/amano-takuro-1/media/image3.png