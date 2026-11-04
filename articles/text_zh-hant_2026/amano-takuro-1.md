* 使用 GraphicsStateCollection 進行著色器預熱  
* Amano Takuro/株式會社 CyberAgent SGE 核心技術本部

# 

著色器第一次用於繪製時，會在執行階段進行編譯處理，可能引發稱為著色器編譯卡頓的效能下降現象。本章以行動裝置環境（Metal、Vulkan）為範圍，介紹使用 GraphicsStateCollection 進行著色器預熱的基本用法，以及導入後仍殘留卡頓的實際案例與因應方式。另外，本章內容以 Unity 6.3 LTS（6000.3）為準。

# **GraphicsStateCollection 概觀**

GraphicsStateCollection（GSC）是 Unity 6 起可使用的 API{{fn:gsc_docs}}。它是一種記錄遊戲過程中使用的著色器變體與圖形狀態（繪製時的渲染設定）的組合，並一次性預先編譯（也就是預熱）的機制。它被定位為 ShaderVariantCollection 在 Metal、Vulkan、D3D12 等現代圖形 API 上的替代方案。另外，截至 Unity 6000.3，它仍是隸屬於 \`UnityEngine.Experimental.Rendering\` 命名空間的 Experimental API，規範在未來版本可能變更。  
![][image1]  
図: GSC 所記錄資訊的範例 {{fig:gsc_recorded_info}}

圖為以腳本取得 GSC 所記錄的資訊，並顯示在 EditorWindow 中的範例。記錄內容的取得方法將在本章最後（{{list:gsc_variant_list}}）介紹。

著色器無法直接在 GPU 上執行，必須先編譯為 GPU 的機器指令。然而機器指令會因 GPU 型號與驅動程式而異，因此無法針對所有裝置預先編譯後再發布。因此，著色器會經由以下流程在 GPU 上執行。

1\. 開發者以 HLSL 等著色器語言撰寫著色器（資產）  
2\. 建置時，Unity 將著色器編譯為各圖形 API 對應的中間表示，並存入應用程式或 AssetBundle。在 Vulkan 中此中間表示為 SPIR-V，在 Metal 中則為 Metal Shading Language（MSL）{{fn:gsc_spirv_msl}}  
3\. 應用程式執行期間，當著色器第一次用於繪製時，從已存入的中間表示產生著色器程式  
4\. 接著，將著色器程式與渲染設定組合，由 GPU 驅動程式編譯為 GPU 的機器指令

在第一次繪製之前，先一次完成步驟 3、4 的執行階段編譯，就是著色器預熱的基本思路。

另外，一個著色器會依每個通道（繪製階段），再依光照影響、是否有霧效等條件（著色器關鍵字）的組合，產生多個可執行程式。這個產生的單位稱為著色器變體。

## **Pipeline State Object**

在現代圖形 API 中，繪製時被編譯的單位稱為 Pipeline State Object（PSO）。PSO 是將著色器變體與渲染設定（頂點佈局、色彩格式與 MSAA 等渲染通道附件、混合狀態、深度測試、模板狀態）視為一體，由 GPU 驅動程式編譯為 GPU 機器指令後的物件。產生的 PSO 會快取在裝置上，只要渲染設定相同就會重複使用。換言之，即使是相同的著色器變體，只要渲染設定不同，就會被編譯為不同的 PSO。

## **著色器編譯卡頓的發生原因**

卡頓的原因在於執行階段進行的兩階段編譯處理。GPU 第一次繪製某個著色器變體時，會在 Main Thread 產生著色器程式（步驟 3），並在 Render Thread 由 GPU 驅動程式產生 PSO（步驟 4，編譯為機器指令）。

在 Unity Profiler 中，前者可觀測為 Main Thread 的 \`Shader.CreateGPUProgram\`，後者可觀測為 Render Thread 的 \`GpuProgramMetal.GetCachedPipeline\`（iOS）或 \`CreateGraphicsPipelineImpl\`（Android）。特別是在 Metal（iOS）、Vulkan（Android）上，PSO 的產生成本很高，是第一次繪製時卡頓的主因。使用 GSC 的預熱，就是在遊戲啟動後等任意時機先完成這些 PSO 產生的手段。
  
傳統的 ShaderVariantCollection 可以管理「要編譯哪些著色器變體」，但不包含渲染設定。因此 ShaderVariantCollection 的預熱僅止於步驟 3 的著色器程式產生，實際繪製時 GPU 驅動程式仍需重新產生 PSO，卡頓依然存在。  

![][image2]![][image3]  
図: Main Thread 與 Render Thread 發生編譯的範例 {{fig:gsc_profiler_stutter}}

圖為在 Vulkan（Android）上發生編譯的影格，以 Profiler 的 Timeline 檢視顯示的範例。上方為選取 Main Thread 的 \`Shader.CreateGPUProgram\` 的畫面，可確認對應的著色器名稱、通道名稱與關鍵字。下方為選取 Render Thread 的 \`CreateGraphicsPipelineImpl\` 的畫面，此例中單次 PSO 產生耗時數十毫秒，可見其為卡頓的主因。

# **使用 GSC 的預熱流程**

使用 GSC 的預熱分為「收集渲染設定（追蹤）」與「預熱（預先編譯）」兩個階段。

## **收集渲染設定**

追蹤的基本形式如 {{list:gsc_trace_basic}} 所示。追蹤期間繪製的物件的渲染設定，會以 \`GraphicsStateCollection.GraphicsState\` 的形式記錄到 GSC。清單中雖彙整在同一個方法內，但實際上 \`BeginTrace()\` 與 \`EndTrace()\` 會在不同時機呼叫，跨多個影格收集繪製。

CODE START csharp | 渲染設定的追蹤 {{list:gsc_trace_basic}}  
private GraphicsStateCollection \_gsc;

void Trace() {  
    // 開始收集  
    \_gsc \= new GraphicsStateCollection();  
    \_gsc.BeginTrace();

    // 在此跨多個影格繪製要預熱的物件  
    // 已繪製的渲染設定會記錄為 GraphicsState

    // 結束收集並儲存  
    \_gsc.EndTrace();  
    \_gsc.SaveToFile(path);  
}  
CODE END

產生的 \`GraphicsStateCollection.GraphicsState\` 會因圖形 API 而異。請在實際執行建置後應用程式的裝置上進行追蹤。

## **預熱（預先編譯）**

載入收集到的 GSC 並執行預熱（{{list:gsc_warmup_progressive}}）。此例中，將匯入專案的 GSC 作為 Addressables 的資產載入。

CODE START csharp | 使用 WarmUpProgressively 的分散預熱 {{list:gsc_warmup_progressive}}  
async Task WarmUpAsync() {  
    // 在載入包含著色器的資產之後執行  
    \_gsc \= await Addressables.LoadAssetAsync\<GraphicsStateCollection\>(address).Task;

    // 分階段 WarmUp（以 Jobs 為基礎，每影格編譯 n 項）  
    StartCoroutine(WarmUpCoroutine());  
}

IEnumerator WarmUpCoroutine() {  
    JobHandle handle \= default;  
    int requestedCount \= 0;  
    // 發生錯誤時 isWarmedUp 可能不會變為 true，因此以請求數量判定結束  
    while (requestedCount \< \_gsc.totalGraphicsStateCount) {  
        handle \= \_gsc.WarmUpProgressively(4, handle);  
        handle.Complete();  
        requestedCount \+= 4;  
        yield return null;  
    }  
}  
CODE END

實作預熱時的重點如下。

\* 使用 \`WarmUp(dependency)\` 也能執行預熱，但會一次完成所有編譯而產生尖峰。請以 \`WarmUpProgressively(count, dependency)\` 指定每次的編譯數量，分散到多個影格  
\* 在載入著色器資產之後執行。若著色器的實例不存在，該變體的預熱會被略過  
\* 完成與否可透過 \`isWarmedUp\` 或 \`completedWarmupCount \== totalGraphicsStateCount\` 確認  
\* 不過，發生預熱錯誤時 \`isWarmedUp\` 可能不會變為 true（於 Unity 6000.3 確認）。因此清單中以請求數量的總和與 \`totalGraphicsStateCount\` 的比較來判定結束，避免無限迴圈

# **收集流程的實作範例**

也可以在應用程式啟動時呼叫 \`BeginTrace()\`，以手動遊玩的方式收集渲染設定。但依應用程式的規模，每次版本更新都以手動方式完整涵蓋並不容易。因此，這裡介紹在收集專用場景中依序繪製資產、以機械方式執行流程來收集的方法。實作範例使用 Addressables 與 UniTask{{fn:gsc_unitask}}。即使未使用這些工具，只要將資產的非同步載入與等待一個影格繪製替換為自身專案的機制，也能實作相同的流程。整體流程如下。

1\. 下載所需的資產（若僅靠 Built-in 資產即可完成則不需要）  
2\. 從 Addressables 的內容目錄（\`IResourceLocator\`）取得收集所需資產的 Address  
3\. 切換 Quality Settings（若著色器不隨 Quality Settings 變化則不需要切換）  
4\. 開始收集（\`BeginTrace()\`）  
5\. 以 Additive 方式載入場景，依序載入並繪製要收集的資產（模型繪製、\`ParticleSystem\` 播放、Timeline 播放）  
6\. 結束收集（\`EndTrace()\`）  
7\. 從 GSC 刪除不需要的著色器  
8\. 儲存 GSC（\`SaveToFile(path)\`）  
9\. 卸載場景，一邊切換 Quality Settings 一邊重複步驟 3〜8

從內容目錄取得會影響渲染設定的資產（場景）、角色與道具等模型資產、特效資產、Timeline 資產，一邊切換渲染設定一邊依序繪製，以達到全面收集。之所以依每個 Quality Settings 分別收集，是因為 MSAA、HDR 等會影響 PSO 的渲染設定會隨 Quality Level 而改變。

處理的骨架如 {{list:gsc_scenario_skeleton}} 所示。針對每個 Quality Settings 重複追蹤，並以 Quality Level 名稱命名，儲存為個別檔案。執行時，選擇並載入對應目前 Quality Level 的檔案進行預熱。

CODE START csharp | 收集流程的骨架 {{list:gsc_scenario_skeleton}}  
private GraphicsStateCollection \_gsc \= new GraphicsStateCollection();

private async UniTaskVoid StartAsync() {  
    // 取得內容目錄並下載所需資產  
    // 事先依 Prefix 取得各類 Address 清單  
    await AddressableSetupAsync();

    var qualitySettingNames \= QualitySettings.names;  
    for (var i \= 0; i \< qualitySettingNames.Length; i++) {  
        QualitySettings.SetQualityLevel(i);  
        await TraceAsync();  
    }  
}

private async UniTask TraceAsync() {  
    // 開始 GSC 收集  
    \_gsc.ClearVariants();  
    \_gsc.BeginTrace();

    // 執行收集流程  
    await ExecuteScenarioAsync();

    // 結束 GSC 收集  
    \_gsc.EndTrace();

    // 從 GSC 刪除不在收集範圍內的著色器  
    RemoveUnnecessaryVariants();

    var qualityLevelName \= QualitySettings.names\[QualitySettings.GetQualityLevel()\];  
    \_gsc.SaveToFile(\_gscSavePathFormat \+ $"\_{qualityLevelName}");  
}  
CODE END

流程本體會針對每個場景依序繪製模型、特效、Timeline。{{list:gsc_scenario_batch}} 是以數個為一批建立並繪製角色模型的部分節錄。等待一個影格的繪製，該影格的渲染設定就會記錄到 GSC。

CODE START csharp | 依序繪製模型資產（節錄） {{list:gsc_scenario_batch}}  
// 收集角色模型  
for (var i \= 0; i \< \_charaAddressArray.Length; i \+= \_characterNum) {  
    // 以數個為一批建立角色模型  
    var batchCount \= Mathf.Min(\_characterNum, \_charaAddressArray.Length \- i);  
    for (var j \= 0; j \< batchCount; j++) {  
        var charaHandle \= Addressables.LoadAssetAsync\<GameObject\>(\_charaAddressArray\[i \+ j\]);  
        var charaPrefab \= await charaHandle.ToUniTask();  
        // 均等配置在攝影機可拍到的位置  
        var pos \= new Vector3((j \- batchCount \* 0.5f) \* 2f, 0f, 5f);  
        \_charaObjectArray\[j\] \= Object.Instantiate(charaPrefab, pos, Quaternion.identity);  
        Addressables.Release(charaHandle);  
    }

    // 等待一個影格的繪製  
    await UniTask.NextFrame();

    for (var j \= 0; j \< batchCount; j++) {  
        DestroyImmediate(\_charaObjectArray\[j\]);  
        \_charaObjectArray\[j\] \= null;  
    }  
}  
CODE END

\`ParticleSystem\` 同樣以數個為一批建立，不同之處在於先以 \`Simulate()\` 同步重現播放途中的狀態（此例為 0.5 秒後）再繪製（{{list:gsc_scenario_particle}}）。由於不需等待實際時間的播放，可縮短收集時間。

CODE START csharp | ParticleSystem 的收集（節錄） {{list:gsc_scenario_particle}}  
// 收集 ParticleSystem  
for (var i \= 0; i \< \_particlesAddressArray.Length; i \+= \_effectNum) {  
    var batchCount \= Mathf.Min(\_effectNum, \_particlesAddressArray.Length \- i);  
    for (var j \= 0; j \< batchCount; j++) {  
        var particleHandle \= Addressables.LoadAssetAsync\<GameObject\>(\_particlesAddressArray\[i \+ j\]);  
        var particlePrefab \= await particleHandle.ToUniTask();  
        var pos \= new Vector3((j \- batchCount \* 0.5f) \* 3f, 0f, 5f);  
        \_particleObjectArray\[j\] \= Object.Instantiate(particlePrefab, pos, Quaternion.identity);  
        Addressables.Release(particleHandle);

        // 以同步處理重現 \_effectSimulateTime 秒後的狀態（不需要 Play）  
        var ps \= \_particleObjectArray\[j\].GetComponentInChildren\<ParticleSystem\>();  
        if (ps \!= null) {  
            ps.Simulate(\_effectSimulateTime, withChildren: true, restart: true);  
        }  
    }

    // 等待一個影格的繪製  
    await UniTask.NextFrame();

    for (var j \= 0; j \< batchCount; j++) {  
        DestroyImmediate(\_particleObjectArray\[j\]);  
        \_particleObjectArray\[j\] \= null;  
    }  
}  
CODE END

使用 Timeline 製作的演出效果，以 \`PlayableDirector\` 實際播放來收集（{{list:gsc_scenario_timeline}}）。由於需等待播放時間，建議將要收集的 Timeline 資產縮減為必要的範圍。

CODE START csharp | Timeline 的收集（節錄） {{list:gsc_scenario_timeline}}  
// 收集 Timeline  
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

收集完成後，只保留想要預熱的著色器（{{list:gsc_remove_variants}}）。以許可清單（\`\_allowShaderNames\`）指定要保留的著色器，並從 GSC 刪除範圍外的著色器與參照遺失（null）的變體。

CODE START csharp | 刪除收集範圍外的變體 {{list:gsc_remove_variants}}  
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

        // 從 GSC 刪除不需要的著色器  
        \_gsc.RemoveVariant(variant.shader, variant.passId, variant.keywords);  
    }  
}  
CODE END

流程與資產的管理方式因專案而異，請依自身專案調整場景組成與收集範圍。

# **卡頓殘留的實際案例與因應方式**

即使導入了 GSC 預熱，有時卡頓仍會殘留。這裡介紹其實際案例與因應方式。

## **Shader Stripping 導致的變體缺失**

此案例為預熱時的日誌出現 \`variant not found\` 錯誤，且在執行階段也觀測不到該變體的編譯（如 Profiler 的 \`Shader.CreateGPUProgram\`）。

原因是 GSC 所包含的著色器變體未包含在應用程式或 AssetBundle 中。依 \*\*Project Settings \> Graphics \> Shader Stripping\*\* 的設定，建置時未使用的變體可能會被排除。這是在著色器或資產變更導致所需變體改變時容易發生的問題。

請確認 Shader Stripping 的設定，並在進行會影響變體的變更後重新收集 GSC。

另外，雖然 GSC 中記錄了變體，但 \`GraphicsState\` 為 0 項時，也會發生相同的 \`variant not found\` 錯誤。此時執行階段會發生該變體的編譯，因此可依執行階段是否發生編譯來區分。

## **中繼資料不一致導致的 mismatch 警告**

此案例為執行預熱時，主控台出現以下警告。

CODE START text  
WarmUp may be inaccurate due to GraphicsDeviceType mismatch.  
WarmUp may be inaccurate due to RuntimePlatform mismatch.  
WarmUp may be inaccurate due to Quality Level mismatch.  
CODE END

原因是 GSC 檔案中儲存的 \`graphicsDeviceType\` / \`runtimePlatform\` / \`qualityLevelName\` 中繼資料與執行環境不一致。主要發生情況有以下兩種。

\* 沿用在不同平台收集的 GSC（例：將在 iOS 實機收集的 GSC 用於 Android 建置）  
\* 以 Editor 腳本編輯並儲存 GSC 時，中繼資料被 Editor 的 Platform 設定覆寫

因應方式依發生情況而異。在不同平台收集的 GSC，即使改寫中繼資料，內容中的 \`GraphicsState\` 也與執行環境不一致，因此需在對應平台重新收集。若是被 Editor 的 Platform 設定覆寫，則將中繼資料修正為實機的值。另外，\`qualityLevelName\` 的不一致，僅在會影響 PSO 的渲染設定（MSAA、HDR 等）與著色器不隨 Quality Level 變化時可以忽略。此時可如 {{list:gsc_quality_fix}} 所示，在預熱前一刻改寫以避免警告。

CODE START csharp | 改寫 qualityLevelName 以避免警告 {{list:gsc_quality_fix}}  
// 載入 GSC  
var gsc \= LoadGsc();  
// 將 qualityLevelName 改為符合執行環境的值  
gsc.qualityLevelName \= QualitySettings.names\[QualitySettings.GetQualityLevel()\];  
// 執行預熱  
CODE END

## **iOS（Metal）上 AssetBundle 重複打包導致的雙重實例**

此案例僅發生於 iOS（Metal）。預熱時的日誌沒有錯誤，Profiler 的 Main Thread 也沒有發生 \`Shader.CreateGPUProgram\`，但 Render Thread 卻發生 \`GpuProgramMetal.GetCachedPipeline\`。

原因是 AssetBundle 的重複打包導致著色器被建立為雙重實例。同一個著色器被重複打包到多個 AssetBundle 時，Unity 會在記憶體中建立各自獨立的著色器實例。預熱只會針對 GSC 所參照的著色器實例建立 PSO，因此即使是相同的著色器，成為不同實例的著色器就不會被預熱。另外，Vulkan 不會發生此問題，推測是因為 Metal 與 Vulkan 的 PSO 快取管理方式不同。

發生此問題的組成範例如下。同一個著色器 X 因相依關係被打包到兩個 Bundle 中。

{{enlargepage}}

CODE START text  
\- 資產  
  \- 著色器 X  
  \- 材質 A（使用著色器 X）  
  \- 材質 B（使用著色器 X）  
  \- GraphicsStateCollection

\- AssetBundle 組成  
  \- Bundle A  
    \- GraphicsStateCollection  
    \- 材質 A（使用著色器 X）  
    \- 著色器 X ← 因相依關係被打包  
  \- Bundle B  
    \- 材質 B（使用著色器 X）  
    \- 著色器 X ← 因相依關係被打包  
（著色器 X 未被指派 Address 的狀態）  
CODE END

在此組成下載入 Bundle A 與 Bundle B 時，會依以下流程在繪製材質 B 時發生卡頓。

CODE START text  
\- 執行階段的流程  
  1\. 載入 Bundle A 與 Bundle B  
     \- Bundle A 的著色器 X（instanceId=100）  
     \- Bundle B 的著色器 X（instanceId=200）  
  2\. 執行 GraphicsStateCollection 的預熱  
     \- 對 GSC 所參照的著色器 X（instanceId=100）進行預熱  
  3\. 繪製材質 A  
     \- 著色器 X（instanceId=100）的 PSO 快取已建立，因此沒有卡頓  
  4\. 繪製材質 B  
     \- 著色器 X（instanceId=200）沒有 PSO 快取，  
       Render Thread 上執行 PSO 建立，發生卡頓  
CODE END

因應方式是將著色器彙整到單一 Bundle（著色器專用群組），排除隱含打包到內容 Bundle 的情況。若難以集中到單一 Bundle，則為每個 Bundle 準備各自的 GSC。重複的著色器可用如 {{list:gsc_duplicate_check}} 的程式碼確認。若使用 Addressables，也可在 \*\*Addressables Report\*\* 視窗中確認著色器的重複。

CODE START csharp | 確認著色器實例的重複 {{list:gsc_duplicate_check}}  
using System.Linq;

// 在載入 AssetBundle 後執行，確認實例是否重複  
var all \= Resources.FindObjectsOfTypeAll\<Shader\>();  
var duplicates \= all.GroupBy(s \=\> s.name).Where(g \=\> g.Count() \> 1);  
foreach (var g in duplicates)  
    foreach (var s in g)  
        Debug.Log($"Duplicate: {s.name} | InstanceID: {s.GetInstanceID()}");  
CODE END

## **iOS（Metal）上使用 SubPass 的著色器預熱失敗**

到此為止是由專案設定或 Bundle 設計的失誤所引起的問題。接下來介紹因 GSC 機制本身而發生的問題。

將一個渲染通道分割為多個繪製步驟，使前一步驟的繪製結果不必寫回主記憶體即可由下一步驟參照的機制，稱為 SubPass。在行動裝置 GPU 上，這一連串步驟可在晶片內的高速記憶體區域（圖塊記憶體）上完成，節省記憶體頻寬。SubPass 在 Metal、Vulkan 上會以 Native Render Pass 執行。僅在 iOS（Metal）上，存在已執行預熱且日誌沒有錯誤，但遊戲過程中仍發生 PSO 編譯的案例。原因是 iOS（Metal）的預熱處理，對於在此 SubPass（Native Render Pass）內繪製的著色器無法如預期運作。無論是否有讀取前一個 SubPass 輸出的 input attachments 都會發生。另外，由於預熱的內部處理為非公開 API，這裡僅介紹發生條件。

受影響的條件如下。

\* 目標為 iOS / Metal（Apple A/M 晶片的實機）  
\* URP 繪製處理基礎（RenderGraph）的設定 \`RenderGraph.nativeRenderPassesEnabled\` 為啟用（Unity 6000.5 以後此設定已廢除，恆為啟用）  
\* 使用在 SubPass（Native Render Pass）內繪製的著色器

無論是僅寫入的 SubPass（如 URP Deferred 的 GBuffer 通道），或是具有 input attachments 的 SubPass（如同為 Deferred 的 Lighting 通道）都同樣會發生。作為受影響 SubPass 的描述範例，{{list:gsc_subpass_input}} 展示設定 input attachments 的情況。

CODE START csharp | 在 SubPass 設定 input attachments 的情況 {{list:gsc_subpass_input}}  
// 在以 ScriptableRenderContext.BeginRenderPass 開始的  
// 渲染通道內設定 SubPass 的範例  
var inputAttachments \= new NativeArray\<int\>(new\[\] { 0, 1 }, Allocator.Temp);  
context.BeginSubPass(  
    colors,  
    inputAttachments,   // 繫結前一個 SubPass 的輸出  
                        // 在 Metal 上會轉換為 framebuffer fetch，  
                        // 但不會反映到 GSC 預熱時的 PSO 產生  
    false  
);  
CODE END

在 Unity 6000.3 中難以從根本解決，只能重新檢視 SubPass 的使用，或以預先渲染來避免遊戲中的卡頓。預先渲染是指在載入畫面等期間，實際繪製一次使用該著色器的物件，事先建立 PSO 快取的手法。可以想像成在遊戲內以與本章收集流程相同的依序繪製來取代預熱。

## **卡頓殘留時的診斷重點**

如前所述，使用 GSC 的著色器預熱與專案的資產管理設計（Addressables 的組成、著色器關鍵字定義、Bundle 設計）密切相關，因此導入後常需要除錯。請確認預熱時的日誌，並以 Profiler 反覆測量，確認是否有預熱遺漏。特別是掌握以下幾點，診斷會更快。

\* 預熱時的日誌是否出現預熱錯誤或 \`variant not found\`  
\* Profiler 中是否發生 Main Thread 的 \`Shader.CreateGPUProgram\`、Render Thread 的 \`GpuProgramMetal.GetCachedPipeline\`（iOS）/ \`CreateGraphicsPipelineImpl\`（Android）  
\* Shader Stripping 的設定是否適當  
\* AssetBundle 的打包是否符合預期

要確認 GSC 中記錄的變體，可利用下一節介紹的變體清單輸出（{{list:gsc_variant_list}}）。

# **GSC 檔案的操作範例**

最後介紹兩個 GSC 檔案的操作範例。兩者皆假設從 Editor 腳本執行，可用於確認收集到的 GSC 內容，或整合多次收集的結果。

## **確認變體清單與 State 數量**

{{list:gsc_variant_list}} 是載入 GSC 檔案，輸出中繼資訊與所記錄變體清單的範例。

CODE START csharp | 確認變體清單與 State 數量 {{list:gsc_variant_list}}  
void PrintVariantInfo() {  
    // 載入 GSC  
    var gsc \= new GraphicsStateCollection();  
    if (\!gsc.LoadFromFile(path))  
        return;

    // 取得變體  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    gsc.GetVariants(variants);

    // 輸出 GSC 的中繼資訊  
    Debug.Log($"\[GSC\] Variants={variants.Count} TotalStates={gsc.totalGraphicsStateCount} "  
            \+ $"Platform={gsc.runtimePlatform} Device={gsc.graphicsDeviceType} Quality={gsc.qualityLevelName}");

    // 輸出變體的詳細資訊  
    foreach (var v in variants) {  
        int stateCount \= gsc.GetGraphicsStateCountForVariant(v.shader, v.passId, v.keywords);  
        string kws \= v.keywords.Length \> 0  
            ? string.Join(" ", v.keywords.Select(k \=\> k.name))  
            : "\<no keywords\>";  
        Debug.Log($"  {v.shader?.name} | states={stateCount} | {kws}");  
    }  
}  
CODE END

## **合併 GSC**

{{list:gsc_merge}} 是合併兩個 GSC 的範例。可將分多次收集的結果整合為一個。

CODE START csharp | 合併兩個 GSC {{list:gsc_merge}}  
void MergeGsc() {  
    // 載入兩個 GSC  
    var src \= new GraphicsStateCollection();  
    var dst \= new GraphicsStateCollection();  
    if (\!src.LoadFromFile(srcPath) || \!dst.LoadFromFile(dstPath))  
        return;

    // 取得變體  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    var states \= new List\<GraphicsStateCollection.GraphicsState\>();  
    src.GetVariants(variants);

    // 從 src 的 GSC 加入 dst 的 GSC  
    foreach (var v in variants) {  
        // 取得變體所包含的 GraphicsState  
        src.GetGraphicsStatesForVariant(v.shader, v.passId, v.keywords, states);  
        foreach (var s in states) {  
            // 加入 State  
            dst.AddGraphicsStateForVariant(v.shader, v.passId, v.keywords, s);  
        }  
    }

    dst.SaveToFile(dstPath);  
}  
CODE END

依 AssetBundle 的組成，有時需要將收集到的 \`GraphicsState\` 移動、整合到其他 GSC。典型情況即是在雙重實例的因應方式中介紹的、為每個 Bundle 準備各自 GSC 的做法。此時請組合運用本節的操作範例。

[image1]: assets/amano-takuro-1/media/image1.png

[image2]: assets/amano-takuro-1/media/image2.png

[image3]: assets/amano-takuro-1/media/image3.png