# **AssetBundle 的生命週期管理**

姓名 / 羅馬字：Inada Taketo  
所屬：株式會社 CyberAgent SGE 核心技術本部  
X 帳號 / 網站 / GitHub 帳號（選填）：

遊戲會在執行階段載入紋理、網格、音訊等大量資產。若由各實作者各自管理，就容易發生忘記釋放或不需要的資產持續常駐的問題。

因此，對於從 AssetBundle 或 Addressables 等動態載入的資產，原則上應將載入與釋放的入口集中到系統基礎層。在載入時決定資產的生命週期，並建立在生命週期結束時由基礎層統一釋放的機制。

本節所說的「釋放」，是指釋放由載入基礎層管理的參照或釋放用 handle。資產是否真的從記憶體卸載，取決於基礎層的實作、參照計數與相依關係的狀態。

此外，後續介紹的分類與實作方式僅為參考範例。請依各專案的遊戲結構選擇合適的做法。

### **依生命週期分別管理資產**

首先，整理遊戲中需要資產的期間。例如可以分類如下。

表: 資產生命週期的分類範例 {{table:asset_lifetime_types}}

生命週期	保留期間的範例  
Common	直到登出、切換帳號、返回標題畫面等重設共用資料為止  
Scene	直到結束對應的場景為止  
View	直到關閉對應的畫面或對話框為止  
Custom	直到戰鬥、活動等自行定義的期間結束為止

重要的不是 Common、Scene、View 這些名稱，而是明確定義各生命週期何時開始、何時結束。

分類過粗，不需要的資產會長時間殘留；分類過細，載入與釋放的次數和管理成本都會增加。另外，若生命週期不同的資產包含在同一個 AssetBundle 中，以 AssetBundle 為單位卸載時就無法只挑選其中部分資產卸載。生命週期的分類應與 AssetBundle 的粒度（前作《Unity 效能調優寶典》（日文）第 5 章「Tuning Practice \- AssetBundle」）一併設計。

### **在載入時決定生命週期**

分類好生命週期後，在載入資產時決定要登錄到哪個生命週期。

其中一種實作範例，是在載入函式的引數中指定生命週期。

CODE START csharp | 指定生命週期進行載入的範例 {{list:asset_load_with_lifetime}}  
var background \=  
    await AssetLoader.LoadAsync\<GameObject\>(  
        address,  
        AssetLifetime.Scene);  
CODE END

在 {{list:asset_load_with_lifetime}} 中，將載入的資產登錄到 Scene 生命週期。場景結束時，由基礎層統一釋放已登錄的資產。

另一種實作範例，是依生命週期分別提供載入函式。

CODE START csharp | 依生命週期分別提供載入函式的範例 {{list:asset_load_by_lifetime}}  
var commonIcon \=  
    await AssetLoader.LoadCommonAsync\<Sprite\>(  
        commonAddress);

var stagePrefab \=  
    await AssetLoader.LoadSceneAsync\<GameObject\>(  
        stageAddress);

var dialogPrefab \=  
    await AssetLoader.LoadViewAsync\<GameObject\>(  
        dialogAddress,  
        viewId);  
CODE END

像 LoadCommonAsync、LoadSceneAsync、LoadViewAsync 這樣拆分函式，就能從呼叫處判斷生命週期，也較容易防止登錄目標指定錯誤。另一方面，以引數指定的方式則具有容易新增自訂生命週期的特點。

無論採用哪種方式，重要的是載入時必定決定生命週期，且在其結束時能由基礎層釋放。

為了因應非同步載入完成前生命週期就已結束的情況，應從開始載入時就追蹤處理，在中途結束時取消載入，或在完成後釋放其結果。此外，若同一資產會被多個生命週期使用，則需管理參照計數，在最後一個使用結束時釋放。

### **將生命週期繫結到 GameObject**

另一種管理方式，是讓 GameObject 成為生命週期的擁有者。

在畫面或對話框的根 GameObject 上加入專用的 Component，登錄已載入資產的釋放用 handle。當 GameObject 被銷毀時，從 OnDestroy 釋放已登錄的 handle。

CODE START csharp | 將生命週期繫結到 GameObject 的範例 {{list:asset_scope_component}}  
public sealed class AssetLifetimeScope : MonoBehaviour  
{  
    private readonly List\<IDisposable\> \_handles \=  
        new List\<IDisposable\>();

    private bool \_released;

    public void Add(IDisposable handle)  
    {  
        if (\_released)  
        {  
            handle.Dispose();  
            return;  
        }

        \_handles.Add(handle);  
    }

    private void OnDestroy()  
    {  
        \_released \= true;

        for (var i \= \_handles.Count \- 1; i \>= 0; i--)  
        {  
            \_handles\[i\].Dispose();  
        }

        \_handles.Clear();  
    }  
}  
CODE END

在 {{list:asset_scope_component}} 中，即使 GameObject 銷毀後非同步載入才完成並呼叫 Add，也會立即釋放新加入的 handle。

不過，僅銷毀 GameObject 並不會自動釋放動態載入的資產。必須從專用的 Component 呼叫對應載入基礎層的釋放處理。

管理用的 GameObject 不要為每個資產各建立一個，而是以一個畫面或對話框等一個生命週期單位為單位，彙整為一個。此方式在 GameObject 與資產的生命週期一致時有效，但對於跨多個場景使用的資產等情況可能不適用。請視需要與依生命週期區分的管理表搭配使用。

### **確認是否有忘記釋放**

使用 Memory Profiler 擷取畫面或場景切換前後的快照，比較已結束生命週期的資產是否仍殘留。多次重複同一切換，確認資產或載入 handle 是否持續增加，也是有效的做法。

測量時，應在必要的釋放處理完成後，以相同的基準狀態互相比較，最終在實機的 Player 建置上確認。不過，也有被其他功能參照的資產，或是刻意作為快取保留的資產，因此不能僅憑資產殘留就判斷為忘記釋放，而要確認到實際的引用方。

資產管理若在開發後期變更，會影響大量載入處理。請在開發早期就整理出所需的生命週期，並設計出即使實作者不特別注意個別釋放時機，也能套用符合專案的生命週期的機制。
