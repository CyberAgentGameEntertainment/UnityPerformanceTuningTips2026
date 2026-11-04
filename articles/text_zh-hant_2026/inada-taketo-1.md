# **UniTask 與取消處理的生命週期管理**

姓名 / 羅馬字：Inada Taketo

所屬：株式會社 CyberAgent SGE 核心技術本部

X 帳號 / 網站 / GitHub 帳號（選填）：

在使用 UniTask 的非同步處理中，若未適當管理結束條件與取消，可能會留下永不完成的處理或不必要的參照。

前作《Unity 效能調優寶典》介紹了 MonoBehaviour 被銷毀後 UniTask 仍持續等待的案例。本節將介紹因 CancellationTokenSource 遺漏 Dispose，導致不必要的物件持續被參照的案例。

此外，CancellationTokenSource 與 CreateLinkedTokenSource 並非 UniTask 特有的功能，而是構成 .NET 取消處理的 API。

本節所處理的程式碼中，using 宣告（using var）是 C\# 8 的語法，可在 Unity 2020.2 以後使用。另外，後述的 CreateLinkedTokenSource 內部實作，是參照 Unity Technologies 公開、適用於 Unity 2022.3 的 Mono 實作（unity-2022.3-mbe 分支），可能會因 Unity 版本而有所不同。

### **CancellationTokenSource 的 Dispose**

CancellationToken 是用來向處理傳達取消要求的實值型別。CancellationToken 本身並未實作 IDisposable，因此不需要呼叫 Dispose。

不過，若透過 CancellationToken.Register 註冊回呼，其回傳值 CancellationTokenRegistration 則是 IDisposable {{fn:cancellation_token_registration_dispose}}。在取消前註冊的回呼會被發行來源的 CancellationTokenSource 保留，因此若在不再需要註冊時未呼叫 Dispose 解除，回呼所參照的物件可能無法被釋放。

另一方面，CancellationTokenSource 實作了 IDisposable。使用完畢的 CancellationTokenSource，由其擁有者呼叫 Dispose。

CODE START csharp | 對 CancellationTokenSource 呼叫 Dispose 的範例 {{list:cts_dispose}}

private async UniTask LoadCancelableAsync()

{

	using var cts \= new CancellationTokenSource();

	

	// 省略發出取消要求的處理

	await LoadAsync(cts.Token);

}

CODE END

{{list:cts_dispose}} 中，透過 using 宣告管理 CancellationTokenSource。不僅在非同步處理正常結束時，在因取消或例外而結束時，離開方法時也會自動呼叫 cts.Dispose()。

Cancel 與 Dispose 的職責不同。Cancel 向處理要求取消，Dispose 則解除並釋放 CancellationTokenSource 所持有的計時器與回呼註冊等。僅呼叫 Dispose 並不會向處理通知取消要求。

此外，即使忘記呼叫 Dispose，只要 CancellationTokenSource 及相關物件都不再被外部參照，就會被 GC 回收 {{fn:managed_memory}}。不過，由於對長生命週期的 CancellationTokenSource 註冊回呼等情況可能會留下參照，原則上應在使用完畢時呼叫 Dispose。

### **CancellationTokenSource.CreateLinkedTokenSource**

CancellationTokenSource.CreateLinkedTokenSource 會建立一個監視多個來源 Token 的新 CancellationTokenSource。當任一來源 Token 被取消時，所建立的 CancellationTokenSource 的 Token 就會進入取消狀態。

來源 Token 之間不會互相取消，取消是從來源 Token 傳播至連結目標。

以下將 tokenA 與 tokenB 的發行來源 CancellationTokenSource 分別稱為 A 與 B。

在 Unity Technologies 於 GitHub 公開、適用於 Unity 2022.3 的 Mono 實作 {{fn:mono_cancellation_token_source}} 中，CreateLinkedTokenSource 會向來源 Token 註冊取消回呼。將與參照關係相關的部分簡化後，如 {{list:linked_source_internal}} 所示。

CODE START csharp | CreateLinkedTokenSource 內部實作的要點 {{list:linked_source_internal}}

linkedCts.m\_linkingRegistrations\[0\] \=

	tokenA.InternalRegisterWithoutEC(

    	s\_LinkedTokenCancelDelegate,

    	linkedCts);

 

linkedCts.m\_linkingRegistrations\[1\] \=

	tokenB.InternalRegisterWithoutEC(

    	s\_LinkedTokenCancelDelegate,

    	linkedCts);

CODE END

註冊至來源 Token 的回呼會參照 linkedCts，而 linkedCts 也持有對 A 與 B 的註冊資訊。

因此，若 B 是整個應用程式共用的長生命週期 CancellationTokenSource，且長時間不會被取消，則在未對 linkedCts 呼叫 Dispose 的情況下，可能會留下以下參照。

長生命週期的 B → 回呼註冊 → linkedCts → 對 A 的註冊資訊 → 短生命週期的 A

僅憑這種參照關係並不一定會造成記憶體洩漏。只要所有物件都不再被外部參照，就會一併被 GC 回收。然而，在 B 存活且連結用的回呼尚未解除期間，linkedCts 會持續被 B 參照，而短生命週期的 A 也會經由它維持可到達的狀態。

此外，若註冊至 linkedCts.Token 的回呼參照了其他物件，這些物件也可能會被保留。

對 linkedCts 呼叫 Dispose 後 {{fn:cancellation_token_source_dispose}}，內部會銷毀對 A 與 B 的 CancellationTokenRegistration，並解除註冊至來源 Token 的回呼。

CreateLinkedTokenSource 的回傳值，建議在使用連結 Token 的處理範圍內，透過 using 宣告等方式管理。

CODE START csharp | 對連結的 CancellationTokenSource 呼叫 Dispose 的範例 {{list:linked_source_using}}

private async UniTask LoadAsync(

	CancellationToken tokenA,

	CancellationToken tokenB)

{

	using var linkedCts \=

        CancellationTokenSource.CreateLinkedTokenSource(

        	tokenA,

        	tokenB);

 

	await LoadAssetAsync(linkedCts.Token);

}

CODE END

{{list:linked_source_using}} 中，在 LoadAssetAsync 因正常結束、取消或例外而結束後，會呼叫 linkedCts.Dispose()，解除對來源 Token 的註冊。

即使連結的 CancellationTokenSource 已處於取消狀態，對其他來源 Token 的註冊仍可能殘留。CreateLinkedTokenSource 的回傳值，務必在不再需要時呼叫 Dispose。

### **以團隊之力防止 IDisposable 物件遺漏 Dispose**

至此，我們以 CancellationTokenSource 為例，探討了 IDisposable 的遺漏銷毀問題。對策的前提是物件的擁有者在適當的時機呼叫 Dispose。然而，隨著專案規模擴大，僅靠程式碼審查持續確認所有建立位置是很困難的。特別是 CreateLinkedTokenSource 只需簡短的程式碼即可建立，因此很容易忽略其回傳值是 IDisposable。

要在團隊中防止再次發生，除了個人的注意之外，建議結合以下做法：將需要銷毀的物件集中到易於使用的 API、以 Analyzer 及早偵測問題，並視需要在 CI 中偵測新的違規。不要只把責任集中在 Analyzer 上，而是從設計、靜態分析、維運各方面支撐同一方針，會更有效果。

這裡介紹的以 using 進行銷毀與以 Analyzer 進行偵測，不限於 CancellationTokenSource，也可應用於其他實作 IDisposable 的資源。

本節將介紹在這些制度化做法中，使用可自動檢查 C\# 程式碼的 Roslyn Analyzer 的方法。

### **什麼是 Roslyn Analyzer**

Roslyn Analyzer 是在編譯時或 IDE 上靜態分析 C\# 程式碼，並對可能有問題的位置顯示警告或建議的機制。它可以根據語法、型別資訊、處理路徑等偵測錯誤徵兆或違反編碼規範之處，若備有 Code Fix，也能提示修正方案。

Analyzer 將何者偵測為問題，是以診斷規則的形式定義，每條規則會被賦予如 CA2000 這樣的識別用診斷 ID。CA2000 是 .NET 提供的既有規則，後述的 UTK001 與 UTK002 則是專案自行新增的規則方案。

在 Unity 專案中，將 Analyzer 的 DLL 放置於 Assets 底下，並加上 RoslynAnalyzer 標籤即可導入 {{fn:roslyn_analyzers}}。也可以利用 asmdef 限定套用的組件。

### **以既有的 Analyzer 作為第一道防線**

首先值得考慮的是 .NET 程式碼品質規則中的 CA2000 {{fn:ca2000}}。CA2000 會偵測以區域變數建立的 IDisposable 物件未經銷毀就離開範圍的路徑。由於 CancellationTokenSource 也是 IDisposable，因此可作為偵測 new CancellationTokenSource() 遺漏銷毀的第一道防線。

不過，依所使用的 Analyzer 版本與分析設定不同，對於像 CreateLinkedTokenSource 這種建立並回傳物件的方法的回傳值，以及方法之間的所有權轉移，能追蹤到何種程度也有所差異。導入時，建議針對 new CancellationTokenSource() 與 CreateLinkedTokenSource 兩者，在小型驗證專案中確認可偵測與無法偵測的範例。既有規則無法充分偵測的模式，可視需要以後述的自訂規則補足。

### **非同步處理專用的自訂規則設計範例**

若既有 Analyzer 無法充分追蹤 CreateLinkedTokenSource 的回傳值，可以用專案專屬的 Roslyn Analyzer 補足。以下是實作自訂 Analyzer 時的規則設計範例。為了便於說明，使用 UTK001 與 UTK002 作為假設的診斷 ID。

UTK001 預計檢查由 new CancellationTokenSource() 或 CancellationTokenSource.CreateLinkedTokenSource() 建立的區域變數，是否受到 using 陳述式、using 宣告或 finally 區塊中的 Dispose 保護。不僅是所有正常結束的路徑，透過 return 或例外離開方法的路徑也納入分析範圍。

CODE START csharp | UTK001 偵測到的範例 {{list:analyzer_detects_linked_source}}

private async UniTask LoadAsync(

	CancellationToken tokenA,

	CancellationToken tokenB)

{

	var linkedCts \=

        CancellationTokenSource.CreateLinkedTokenSource(

        	tokenA,

        	tokenB);

 

	await LoadAssetAsync(linkedCts.Token);

}

CODE END

{{list:analyzer_detects_linked_source}} 中，linkedCts 未被 Dispose 就離開了方法。此規則預計在 CreateLinkedTokenSource 的呼叫位置發出警告，並引導以 using 宣告或 try-finally 確保銷毀。

UTK002 預計檢查 CancellationToken.Register 的回傳值 CancellationTokenRegistration 是否被直接丟棄。不接收 Register 的回傳值而直接捨棄的程式碼，其解除註冊的責任歸屬不明確。若回呼只需在非同步處理期間有效，建議以 using 宣告管理。若保留在欄位中，則在擁有者的結束處理中呼叫 Dispose。

CODE START csharp | 管理 CancellationTokenRegistration 的範例 {{list:registration_using}}

private async UniTask WaitAsync(CancellationToken token)

{

	using var registration \= token.Register(OnCanceled);

 

	await WaitForSignalAsync(token);

}

CODE END

{{list:registration_using}} 中，在 WaitForSignalAsync 結束的時間點銷毀 registration。另一方面，也有像是永久註冊至整個應用程式共用的 Token 等，讓註冊與 Token 生命週期一致的設計。若連這類範例也一律警告，會增加誤判，因此建議 UTK002 先以資訊層級試行，並讓專案中常見的安全模式能夠被排除。

保留在欄位中的 CancellationTokenSource，其所有權的判定比區域變數更困難。作為第二階段的規則，可以檢查在將新的 CancellationTokenSource 指派給欄位之前是否已對舊值呼叫 Dispose，以及是否在 MonoBehaviour 的 OnDestroy 等擁有者的結束處理中呼叫 Dispose。不過，由於可能伴隨所有權轉移給其他物件的情況，建議與區域變數的規則分開導入。

### **以 Code Fix 統一修正方式**

Roslyn Analyzer 也可以準備 Code Fix，在 IDE 上自動修正偵測到的問題。例如，針對 UTK001 中所有權在方法內完結的單純案例，提供將 var 轉換為 using var 的 Code Fix，就更容易在團隊內統一可安全機械轉換之模式的修正方式。至於指派給欄位或經由回傳值轉移所有權等無法唯一決定銷毀時機的案例，建議不提示自動修正，僅止於促使確認設計的診斷。

在程式碼審查中，除了 Analyzer 的結果之外，建議也確認能否從程式碼中讀出建立、Cancel、Dispose 各自的負責者。另外也要注意，僅憑 Analyzer 沒有警告，並不能證明安全性。

### **僅靠 Analyzer 無法偵測的問題**

靜態分析有其極限。將 CancellationTokenSource 存入集合，或是傳遞給其他物件並轉移所有權等情況，有時無法正確判斷最終的銷毀位置。若為了避免誤判而削弱分析，這次又會增加漏判。

這個領域可以透過 Development Build 或測試環境中的執行階段驗證來補足。若經由專案共用的建立機制或管理生命週期的型別來建立 CancellationTokenSource，並記錄建立數與銷毀數，就能在場景切換或測試結束時確認是否殘留未銷毀的範圍。若也要記錄建立時的堆疊追蹤，考量到負載，建議僅在開發環境中啟用。

可視需要結合以 Profiler 確認記憶體增加的調查、執行階段的未銷毀偵測，以及 Roslyn Analyzer 在編輯時的警告。不只是問題發生後由負責人調查的流程，若能建立在撰寫程式碼的當下就提示修正方式的流程，將有助於團隊層面防止問題再次發生。

{{pagebreak}}

### **參考資料**

Unity Manual《Roslyn 分析器與來源產生器》（日文）

Microsoft Learn《CA2000：在超出範圍之前處置物件》（日文）

Unity Technologies《Mono CancellationTokenSource.cs（unity-2022.3-mbe 分支）》

Microsoft Learn〈CancellationTokenSource.Dispose Method〉

Microsoft Learn〈CancellationTokenRegistration.Dispose Method〉

Unity Manual《受控記憶體》（日文）

脚注: {{fn:roslyn_analyzers}}

https://docs.unity3d.com/ja/2022.3/Manual/roslyn-analyzers.html（參照日期：2026 年 9 月 8 日）

脚注: {{fn:ca2000}}

https://learn.microsoft.com/ja-jp/dotnet/fundamentals/code-analysis/quality-rules/ca2000（參照日期：2026 年 9 月 8 日）

脚注: {{fn:mono_cancellation_token_source}}

https://github.com/Unity-Technologies/mono/blob/unity-2022.3-mbe/mcs/class/referencesource/mscorlib/system/threading/CancellationTokenSource.cs（參照日期：2026 年 9 月 8 日）

脚注: {{fn:cancellation_token_source_dispose}}

https://learn.microsoft.com/ja-jp/dotnet/api/system.threading.cancellationtokensource.dispose（參照日期：2026 年 9 月 8 日）

脚注: {{fn:cancellation_token_registration_dispose}}

https://learn.microsoft.com/ja-jp/dotnet/api/system.threading.cancellationtokenregistration.dispose（參照日期：2026 年 9 月 8 日）

脚注: {{fn:managed_memory}}

https://docs.unity3d.com/ja/2022.3/Manual/performance-managed-memory.html（參照日期：2026 年 9 月 8 日）