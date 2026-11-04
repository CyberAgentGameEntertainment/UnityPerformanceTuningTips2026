# **Unity Managed Code Stripping 實戰指南**

Mukai Yuichiro / 所屬: 株式會社 Applibot  
yucchiy\_  / blog.yucchiy.com / yucchiy  
\---

本章介紹 Unity 作為縮減安裝包大小的手段所提供的 Managed Code Stripping 功能，它會在建置時移除未使用的程式碼。  
這是只需一項設定即可啟用的便利功能，但若加強削減力度，可能引發「在編輯器中正常運作，卻只在建置後的應用程式中特定功能損壞」的問題。  
本章將介紹 Managed Code Stripping 的機制與用法、程式碼被過度削減的典型模式及其因應方式等。

驗證環境如下。

*  Unity 6000.3.19f1  
* Addressables 4.0.1  
* macOS Sequoia 15.7.3

先說結論：Managed Code Stripping 並不是把設定調成 High 就算完成的最佳化。

必須明確標示靜態分析看不見的相依關係，並搭配持續驗證建置後應用程式的大小與行為的流程，才能安全地使用這項功能。

# **什麼是 Managed Code Stripping**

## **功能概要**

Managed Code Stripping 是 Unity 的標準功能，會在建置時偵測未使用的受控程式碼（C\# 程式碼），並從建置後的應用程式中移除。

藉由移除場景或腳本未使用的類別與方法，可以縮減應用程式的程式碼大小。

削減的範圍不只是使用者程式碼，也包含 UnityEngine 的模組、.NET 的類別函式庫，以及專案內套件與外掛程式的組件，因此連自己難以修改的函式庫內部的未使用程式碼也能一併削減。

特別是在 IL2CPP 建置中，受控程式碼減少後，後續 IL2CPP 產生的 C++ 程式碼也會隨之減少，因此也有助於縮小安裝包大小與縮短建置時間。

另外，有一個名稱相似的設定 Strip Engine Code，但它是針對 Unity 引擎原生程式碼的削減設定，與針對受控程式碼的 Managed Code Stripping 是不同的功能。

### **\#\#\# Managed Stripping Level**

要多積極地削減程式碼，由名為 Managed Stripping Level 的設定控制。

可在 Edit \> Project Settings \> Player \> Other Settings \> Optimization 的「Managed Stripping Level」中設定，並可選擇下列等級。

表: Managed Stripping Level 的選項 {{table:stripping_levels}}

| Level | 概要 |
| :---- | :---- |
| Disabled | 不進行剝除。僅能在 Mono 建置中選擇 |
| Minimal | 僅針對 UnityEngine 與 .NET 的類別函式庫削減，不刪除使用者程式碼。IL2CPP 建置的預設值 |
| Low | 使用者程式碼也納入分析範圍，但保守地削減 |
| Medium | 針對所有組件，在大小與安全性之間取得平衡來削減 |
| High | 對所有組件進行最積極的削減 |

Mono 建置的預設值是 Disabled，因此若不做任何設定，就不會進行剝除。

另一方面，IL2CPP 建置的預設值是 Minimal，且無法選擇 Disabled，因此無論程度如何，一定會進行剝除。

另外，Unity 6.3 的文件已預告 Low 將於未來廢止，不建議使用。

參考：\[Managed code stripping\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping.html) / \[Configure managed code stripping\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-configure.html)

## **基本運作原理**

Managed Code Stripping 是由 Unity 內部開發的工具 \*\*UnityLinker\*\* 實現的。

UnityLinker 會在應用程式的建置管線中、C\# 程式碼編譯完成後執行，從已編譯的組件（DLL）中移除未使用的程式碼。

在 IL2CPP 建置中，經過這次剝除的組件會成為後續轉換為 C++ 程式碼的輸入。

UnityLinker 以下列三個階段判定未使用的程式碼。

1. 決定作為分析起點的程式碼（\*\*root\*\*）  
2. 從 root 遞迴追蹤靜態參照的型別與成員，標記為使用中（\*\*mark\*\*）  
3. 將直到最後都未被標記的程式碼視為無法到達，從組件中移除

被視為 root 的代表性項目如下。

* 建置所包含的場景或資產所參照的型別  
* 繼承 MonoBehaviour 或 ScriptableObject 的型別  
* 加上 \[RuntimeInitializeOnLoadMethod\] 等屬性的方法  
* 以 link.xml 或 \[Preserve\] 屬性明確指定保留的程式碼（將在後述的因應方式中說明）

這裡的重點在於，程式碼是否保留，並非依「執行階段是否實際使用」，而是依「建置時的靜態分析能否從 root 到達」來判斷。

所謂靜態分析，是指不執行程式碼，只機械式地追蹤已編譯程式碼中出現的參照的分析方式。

例如以 new 建立型別或直接呼叫方法時，參照目標的型別與成員資訊會原樣留在已編譯的程式碼中，因此 UnityLinker 能夠追蹤。

另一方面，以程式碼上不出現參照的形式被呼叫的程式碼，無論執行階段是否使用，都會被視為無法到達。

因此，僅具有靜態分析看不見的參照的程式碼，即使在執行階段是必要的，也會被判定為未使用而遭刪除。

此外，mark 是以成員為單位而非型別為單位進行，因此即使型別留下，其中未使用的方法仍會被個別刪除。

實際上哪些程式碼會被刪除，將在下一節搭配具體程式碼與測量結果確認。

### **以 Mono.Cecil 確認哪些被刪除了**

本章在之後的各節中，會反覆確認「哪些型別與方法留在了建置中」。

這項確認使用的是分析 .NET 組件的函式庫 \*\*Mono.Cecil\*\*。

讀取建置輸出中包含的組件（若是 macOS 的 Mono 建置，則為「應用程式名稱.app/Contents/Resources/Data/Managed/Assembly-CSharp.dll」），將型別與方法的清單寫入檔案。

CODE START csharp | 輸出型別與方法清單 {{list:cecil_dump}}  
// 讀取組件，寫出型別與方法（含 .ctor）的清單  
using var assembly \= AssemblyDefinition.ReadAssembly(dllPath);  
var lines \= assembly.MainModule.Types  
    .Where(type \=\> type.Name \!= "\<Module\>")  
    .SelectMany(type \=\> new\[\] { type.FullName }  
        .Concat(type.Methods.Select(method \=\> $"  {method.Name}")));  
File.WriteAllLines(outputPath, lines);  
CODE END

方法清單中也包含建構函式（.ctor），因此也可用來確認後述的「型別留下但只有建構函式消失」的症狀。

Mono.Cecil 可透過 Unity Package Manager 安裝使用。在編輯器用的 Assembly Definition 中啟用 overrideReferences，並將 Mono.Cecil.dll 加入 precompiledReferences，即可從編輯器程式碼參照。

對變更 Level 的兩個建置分別取得這份清單並比較差異，就能找出哪些型別消失、哪些方法以成員為單位被刪除。

建置後，剝除後的組件有時會以中間產物的形式留在 Library 底下。

在本次環境中，Library/Bee/artifacts/MacStandalonePlayerBuildProgram/ManagedStripped/ 底下留有與建置輸出內容相同的 DLL，不必展開建置產物也能以此作為分析目標。

官方文件並未明確記載剝除後的組件會儲存在此路徑，若找不到，請改為分析建置輸出。

後續各節中寫到「確認了型別與方法」之處，基本上都是以這個方法確認的。比較剝除前後的 DLL，還可以確認削減的明細。

參考：\[Unity linker marking rules\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-marking-rules.html)

## **為什麼想削減程式碼**

使用 Managed Code Stripping 的主要目的是縮減安裝包大小。

附帶地，由於未使用的程式碼不會載入記憶體，也可期待降低執行階段的記憶體用量。

為了確認效果程度，建置一個只放了一支結束處理腳本的簡單場景，比較 Managed Stripping Level 為 Minimal 與 High 的結果。

測量結果如下（{{table:size_comparison}}）。

表: 簡單場景中 Minimal 與 High 的比較 {{table:size_comparison}}

| 指標 | Minimal | High | 削減 |
| :---- | :---- | :---- | :---- |
| 受控組件總大小 | 7,702,016 bytes | 7,188,480 bytes | 約 6.7% |
| 組件（DLL）數量 | 52 | 51 | 1 個 |
| 應用程式整體大小 | 89,069,307 bytes | 88,548,299 bytes | 約 0.6% |

即使是小型範例專案，受控程式碼也減少了 6.7%。

從明細來看，沒有任何地方使用的 System.Runtime.Serialization.dll 被整個刪除，mscorlib.dll 減少約 272KB，而含有大量場景未參照程式碼的 Assembly-CSharp.dll 也減少了 22.5%。

受控程式碼的削減量，取決於專案參照的函式庫數量，以及其中實際未使用程式碼的比例。

大型專案傾向引入許多函式庫，而函式庫中實際未使用的功能往往也很多，因此更容易受惠於程式碼削減。

在 IL2CPP 建置中，被削減的受控程式碼對應的 C++ 轉換與編譯範圍也會減少，因此效果會連帶擴及原生二進位檔大小與建置時間。

# **套用 Managed Code Stripping 時的注意事項**

如前一節所見，Managed Code Stripping 的判斷依賴建置時的靜態分析。

因此，僅具有靜態分析看不見的參照的程式碼，即使在執行階段是必要的，也會被刪除。

這個問題麻煩之處在於，症狀只會出現在建置後的應用程式中。

在編輯器中執行，或在剝除停用的預設 Mono 建置中都能正常運作，因此開發期間不易察覺，問題會以「一提高 Stripping Level，就只有特定功能損壞」的形式浮現。

本節將探討程式碼被過度刪除的兩種典型模式。

一種是透過反射呼叫程式碼，另一種是未被 Unity 進入點參照的程式碼。

## **透過反射的程式碼呼叫消失**

以具體程式碼確認前一節說明的「靜態分析能否從 root 到達」的判斷。

準備三個被參照方式不同的類別（{{list:probe_targets}}）。

CODE START csharp | 被參照方式不同的三個類別 {{list:probe_targets}}  
// 以 new 直接建立  
public class DirectlyUsedService  
{  
    public string Describe() \=\> "DirectlyUsedService is alive";  
}

// 僅以 typeof 參照型別  
public class TypeofOnlyService  
{  
    public string Describe() \=\> "TypeofOnlyService is alive";  
}

// 僅透過反射建立  
public class ReflectionOnlyService  
{  
    public string Describe() \=\> "ReflectionOnlyService is alive";  
}  
CODE END

從放在場景中的 MonoBehaviour，如下使用這三個類別（{{list:stripping_probe}}）。

CODE START csharp | 使用三個類別的 MonoBehaviour {{list:stripping_probe}}  
public class StrippingProbe : MonoBehaviour  
{  
    private static readonly string ReflectionTypeName \= "ReflectionOnlyService";  
    private static readonly string TypeofTypeName \= "TypeofOnlyService";  
    private static readonly string TypeofMethodName \= "Describe";

    private void Start()  
    {  
        // 靜態參照：參照目標的型別與方法資訊會留在已編譯的程式碼中  
        var direct \= new DirectlyUsedService();  
        Debug.Log($"direct: {direct.Describe()}");

        // 靜態參照僅有型別：typeof 參照型別，但沒有對成員的靜態參照  
        Debug.Log($"typeof: type is referenced: {typeof(TypeofOnlyService).Name}");

        // 成員是否留下，透過靜態分析看不見的途徑重新取得型別來確認  
        var typeofType \= Type.GetType(TypeofTypeName);  
        var describeMethod \= typeofType?.GetMethod(TypeofMethodName);  
        if (describeMethod \== null)  
        {  
            Debug.Log($"typeof: type {typeofType?.Name} exists but Describe is stripped");  
        }  
        else  
        {  
            Debug.Log($"typeof: {describeMethod.Invoke(Activator.CreateInstance(typeofType), null)}");  
        }

        // 靜態分析看不見的參照：透過反射建立  
        var type \= Type.GetType(ReflectionTypeName);  
        if (type \== null)  
        {  
            Debug.Log($"reflection: Type.GetType(\\"{ReflectionTypeName}\\") returned null");  
        }  
        else  
        {  
            var instance \= Activator.CreateInstance(type);  
            Debug.Log($"reflection: {type.GetMethod("Describe")?.Invoke(instance, null)}");  
        }  
    }  
}  
CODE END

將此範例的 Managed Stripping Level 分別設為 Minimal 與 High 建置並執行，結果如下。

CODE START text  
\# Minimal  
direct: DirectlyUsedService is alive  
typeof: type is referenced: TypeofOnlyService  
typeof: TypeofOnlyService is alive  
reflection: ReflectionOnlyService is alive

\# High  
direct: DirectlyUsedService is alive  
typeof: type is referenced: TypeofOnlyService  
typeof: type TypeofOnlyService exists but Describe is stripped  
reflection: Type.GetType("ReflectionOnlyService") returned null  
CODE END

在 High 中，typeof 的案例雖能取得型別但 GetMethod 回傳 null，反射的案例則是 Type.GetType 本身回傳 null。

整理後的行為如下（{{table:probe_results}}）。

表: Minimal 與 High 下的保留情況 {{table:probe_results}}

| 項目 | Minimal | High |
| :---- | :---- | :---- |
| StrippingProbe | 保留 | 保留 |
| DirectlyUsedService（以 new 參照） | 保留 | 型別與方法都保留 |
| TypeofOnlyService（僅以 typeof 參照型別） | 保留 | 型別保留但 Describe 被刪除 |
| ReflectionOnlyService（僅反射） | 保留 | 整個型別被刪除 |

Minimal 不會刪除使用者程式碼，因此三者都原樣保留。

在 High 中，被參照方式的差異直接反映在結果上。

DirectlyUsedService 以 new 參照型別，也直接呼叫 Describe，因此型別與方法都保留。

以 typeof 參照的 TypeofOnlyService 雖然型別保留，但沒有任何地方呼叫的 Describe 以成員為單位被刪除。

而完全沒有靜態參照的 ReflectionOnlyService，則是整個型別被刪除。

另外，這個實驗中型別名稱與方法名稱的字串全都經由 static readonly 欄位讀取，是刻意的設計。

UnityLinker 能理解部分反射的典型用法，已確認若像 Type.GetType("ReflectionOnlyService") 這樣直接傳入常數字串，即使在 High 下型別也會被保留。

此外，對 typeof 取得的 Type 直接呼叫 GetMethod 時，即使方法名稱不是常數，該型別的 public 方法也會被整批保留。

另一方面，經由欄位傳入的字串，UnityLinker 就不會追蹤。

以欄位讀取的方式，重現了型別名稱或成員名稱來自設定檔、序列化資料、伺服器回應等建置時無法確定的途徑的實際情況。

若將欄位改為 const，會以常數字串的形式嵌入使用處，因此與直接傳入常數的情況相同。

### DI 容器中的型別解析損壞

到目前為止的範例為了說明而簡化，但相同結構的問題會發生在實際的函式庫中。代表性的例子就是 DI 容器。

許多 DI 容器會事先註冊介面與實作型別的對應，並在解析時透過反射呼叫建構函式來建立實例。

以模仿這個行為的最小容器來確認。與實際的 DI 容器相同，介面設計為以 Register 註冊、以 Resolve 解析（{{list:mini_container}}）。

CODE START csharp | 最小的 DI 容器 {{list:mini_container}}  
public sealed class MiniContainer  
{  
    private readonly Dictionary\<Type, Type\> \_registrations \= new Dictionary\<Type, Type\>();

    // 以型別引數註冊實作型別。TImplementation 會成為相當於 typeof 的靜態參照  
    public void Register\<TInterface, TImplementation\>() where TImplementation : TInterface  
    {  
        \_registrations\[typeof(TInterface)\] \= typeof(TImplementation);  
    }

    // 透過反射將已註冊的實作型別實例化並回傳  
    public TInterface Resolve\<TInterface\>()  
    {  
        var implementationType \= \_registrations\[typeof(TInterface)\];  
        return (TInterface)Activator.CreateInstance(implementationType);  
    }  
}  
CODE END

要註冊的實作型別假定只透過容器使用，不從任何地方靜態參照（{{list:notification_service}}）。

CODE START csharp | 要註冊的介面與實作 {{list:notification_service}}  
public interface INotificationService  
{  
    string Notify();  
}

// 只透過容器使用、未被任何地方靜態參照的實作  
public sealed class NotificationService : INotificationService  
{  
    public string Notify() \=\> "NotificationService.Notify succeeded";  
}  
CODE END

從放在場景中的 MonoBehaviour 進行註冊與解析（{{list:di_probe}}）。

CODE START csharp | 進行註冊與解析的 MonoBehaviour {{list:di_probe}}  
public sealed class DiContainerProbe : MonoBehaviour  
{  
    private void Start()  
    {  
        try  
        {  
            var container \= new MiniContainer();  
            container.Register\<INotificationService, NotificationService\>();  
            Debug.Log($"\[DiContainerProbe\] explicit: {container.Resolve\<INotificationService\>().Notify()}");  
        }  
        catch (Exception exception)  
        {  
            Debug.Log($"\[DiContainerProbe\] explicit failed: {exception.GetType().Name}: {exception.Message}");  
        }  
    }  
}  
CODE END

以與先前相同的條件建置後，結果如下。

CODE START text  
\# Minimal  
\[DiContainerProbe\] explicit: NotificationService.Notify succeeded

\# High  
\[DiContainerProbe\] explicit failed: MissingMethodException: Default constructor not found for type NotificationService  
CODE END

在 High 中，Register\<INotificationService, NotificationService\>() 的型別引數會成為靜態參照，因此 NotificationService 的型別本身會保留。

此外，經由介面靜態呼叫的 Notify 也會保留。

然而，只透過反射呼叫的預設建構函式沒有靜態參照，因此以成員為單位被刪除。

結果是 Activator.CreateInstance 找不到建構函式，解析因 MissingMethodException 而失敗。

這就是「型別明明已註冊，卻只在建置後的應用程式中解析失敗」這種問題的原因。

實際的 DI 容器也是相同的結構。

只要建構函式的解析依賴反射，即使是明確註冊的型別，其隱含的預設建構函式也會成為刪除的候選。

若採用從組件掃描實作型別並自動註冊的方式，沒有靜態參照的實作型別會整個被刪除，導致註冊本身失敗。

## **未被 Unity 進入點參照的程式碼消失**

另一種模式是場景與腳本都未參照、只有透過 AssetBundle 等外部內容才會需要的程式碼。

如前一節所見，root 的來源是建置所包含的場景與資產。

另外建置為 AssetBundle 的內容不包含在應用程式本體的建置中，因此該內容所參照的型別不會被視為 root。

因此，只有內容端需要的型別，若應用程式的程式碼中沒有靜態參照，無論執行階段是否會載入，都會被判定為未使用。

以 Timeline 資產重現這個模式。

Timeline 資產只包含在 AssetBundle 中，應用程式端不靜態參照 Unity.Timeline，而是以資產名稱載入後由 PlayableDirector 播放（{{list:timeline_probe}}）。

CODE START csharp | 從 AssetBundle 播放 Timeline 資產的 MonoBehaviour {{list:timeline_probe}}  
public class TimelineAssetBundleProbe : MonoBehaviour  
{  
    private const string BundleDirectoryName \= "TimelineBundles";  
    private const string BundleName \= "timeline.bundle";  
    private const string TimelineAssetName \= "TimelineOnly";

    private void Start()  
    {  
        var bundlePath \= Path.Combine(Application.streamingAssetsPath, BundleDirectoryName, BundleName);  
        var bundle \= AssetBundle.LoadFromFile(bundlePath);  
        if (bundle \== null)  
        {  
            Debug.Log("timeline bundle: failed to load asset bundle");  
            return;  
        }

        var asset \= bundle.LoadAsset(TimelineAssetName);  
        if (asset \== null)  
        {  
            Debug.Log($"timeline bundle: asset {TimelineAssetName} failed to load");  
            bundle.Unload(true);  
            return;  
        }

        Debug.Log($"timeline bundle: loaded {asset.GetType().AssemblyQualifiedName}");  
        var playableAsset \= asset as PlayableAsset;  
        if (playableAsset \== null)  
        {  
            Debug.Log($"timeline bundle: asset {TimelineAssetName} is not playable");  
            bundle.Unload(true);  
            return;  
        }

        var director \= gameObject.AddComponent\<PlayableDirector\>();  
        director.playableAsset \= playableAsset;  
        director.Play();  
        Debug.Log($"timeline bundle: play state={director.state}, graph valid={director.playableGraph.IsValid()}");  
        bundle.Unload(false);  
    }  
}  
CODE END

建立 AssetBundle 後，先從專案中刪除原本的 Timeline 資產再建置應用程式，以此模擬與應用程式分開更新內容的方式。

將 Managed Stripping Level 分別設為 Minimal 與 High 建置。兩個等級的結果相同。

CODE START text  
\# Minimal / High 皆為  
The referenced script (UnityEngine.Timeline.TimelineAsset) on this Behaviour is missing\!  
timeline bundle: asset TimelineOnly failed to load  
CODE END

此外，查看建置後應用程式的 Managed 資料夾，兩個建置中都不存在 Unity.Timeline.dll 本身。

需要注意的是，Minimal 與 High 的結果沒有差別。

對這個模式的因應不是調整 Stripping Level，而是明確指定保留必要的型別或組件。

其手段將在下一節說明，使用 Addressables 時的機制則在應用篇說明。

參考：\[How code stripping affects content\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-content.html) / \[Timeline 資產消失的案例（Qiita，2021，日文）\](https://qiita.com/KyoheiOkawa/items/0cf9cc09679eababec75)

# **程式碼過度刪除問題的因應方式**

## **基本因應方式**

防止必要程式碼被刪除的最基本手段，是明確告知 UnityLinker「這段程式碼會被使用」。

Unity 為此提供了 link.xml 檔案與 \[Preserve\] 屬性兩種手段。

以任一方式指定的程式碼都會被視為分析的 root，即使沒有靜態參照也不會被刪除。

### **link.xml**

link.xml 是列舉要保留的組件、型別、成員的 XML 檔案。

放在專案的 Assets 底下，UnityLinker 就會在建置時讀取。

前一節範例中整個型別被刪除的 ReflectionOnlyService，可用下列 link.xml 保留（{{list:linkxml_reflection}}）。

CODE START xml | 保留 ReflectionOnlyService 的 link.xml {{list:linkxml_reflection}}  
\<linker\>  
  \<assembly fullname="Assembly-CSharp"\>  
    \<type fullname="ReflectionOnlyService" preserve="all" /\>  
  \</assembly\>  
\</linker\>  
CODE END

將這個 link.xml 直接放在 Assets 底下並以 High 重新建置後，執行結果變為如下。

CODE START text  
\# High（無 link.xml）  
reflection: Type.GetType("ReflectionOnlyService") returned null

\# High（有 link.xml）  
reflection: ReflectionOnlyService is alive  
CODE END

查看建置所包含的組件，也能確認 ReflectionOnlyService 連同 Describe 一起被保留。

link.xml 可以在程式碼之外指定保留目標，因此也適用於自己無法編輯的 DLL 或套件中的型別。

保留範圍可以組件為單位、型別為單位，或以方法、欄位等成員為單位指定。

另外，以 preserve="all" 保留整個組件的寫法雖然簡便，卻會抵銷對該組件的剝除效果。

這可用於釐清問題，但作為長期的因應方式，最好能縮小到必要的型別與成員。

### **\[Preserve\] 屬性**

若是自己能編輯的程式碼，可直接對目標加上 UnityEngine.Scripting 命名空間的 \[Preserve\] 屬性。

需要注意的是，\[Preserve\] 的保留範圍會依加註的位置而不同。

以加在型別上的類別與只加在方法上的類別兩者來確認這個差異（{{list:type_preserved}}、{{list:method_preserved}}）。

CODE START csharp | 在型別上加 Preserve 的情況 {{list:type_preserved}}  
using UnityEngine.Scripting;

// 在型別上加 \[Preserve\]，方法上不加  
\[Preserve\]  
public sealed class TypePreservedService  
{  
    public string Describe() \=\> "TypePreservedService.Describe is alive";  
}  
CODE END

CODE START csharp | 只在方法上加 Preserve 的情況 {{list:method_preserved}}  
using UnityEngine.Scripting;

// 型別上不加，只在方法上加 \[Preserve\]  
public sealed class MethodPreservedService  
{  
    \[Preserve\]  
    public string Describe() \=\> "MethodPreservedService.Describe is alive";  
}  
CODE END

以不帶靜態參照的方式透過反射建立並呼叫這兩者，在 High 下的結果如下（{{table:preserve_results}}）。

表: Preserve 加註位置與 High 下的保留範圍 {{table:preserve_results}}

| 項目 | 型別 | 預設建構函式 | Describe |
| :---- | :---- | :---- | :---- |
| TypePreservedService | 保留 | 保留 | 被刪除 |
| MethodPreservedService | 保留 | 被刪除 | 保留 |

加在型別上的 \[Preserve\] 會保留型別與預設建構函式，但不會保留未加屬性的 Describe。

加在方法上的 \[Preserve\] 會保留該型別與方法，但這次預設建構函式不被保留，以 Activator.CreateInstance 建立時會因 MissingMethodException 而失敗。

要讓透過反射建立並呼叫的流程順利運作，必須在型別與方法兩者上都加上 \[Preserve\]。

遇到「加了 \[Preserve\] 卻不能運作」的情況時，請先懷疑屬性加註位置與保留範圍的對應關係。

參考：\[Preserving code using annotations\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-preserving.html)

## **應用篇**

###  **活用 Preserve 以外的屬性**

相對於 \[Preserve\] 是「保留這個元素」的點狀指定，UnityEngine.Scripting 命名空間中還有像「保留這個介面的所有實作」這樣，以模式宣告保留的屬性。

在「以反射尋找並呼叫使用者實作的型別」這類框架常見的結構中，可以不必逐一以 \[Preserve\] 指定實作型別，而是在介面或基底類別上加屬性來因應。

* \[RequireImplementorsAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireImplementorsAttribute.html)：加在介面上時，保留實作該介面的型別](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireImplementorsAttribute.html\)：加在介面上時，保留實作該介面的型別)  
* \[RequireDerivedAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireDerivedAttribute.html)：加在類別上時，保留繼承該型別的型別](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireDerivedAttribute.html\)：加在類別上時，保留繼承該型別的型別)  
* \[RequiredMemberAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredMemberAttribute.html)：加在成員上時，若該型別保留則該成員也一併保留](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredMemberAttribute.html\)：加在成員上時，若該型別保留則該成員也一併保留)  
* \[RequiredInterfaceAttribute\](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredInterfaceAttribute.html)：加在型別上時，保留指定介面的實作資訊

以 \[RequireImplementors\] 為代表確認其行為。

在介面上加屬性，實作不從任何地方靜態參照（{{list:require_implementors}}）。

CODE START csharp | 加上 RequireImplementors 的介面與實作 {{list:require_implementors}}  
// 加在介面上後，實作型別就會成為保留的目標  
\[RequireImplementors\]  
public interface IRequiredContract  
{  
}

// 未被任何地方靜態參照的實作  
public sealed class RequiredContractImpl : IRequiredContract  
{  
}  
CODE END

以 High 建置後，RequiredContractImpl 連同預設建構函式一起被保留。

沒有靜態參照的實作型別，僅憑介面端的宣告就留了下來。

加上 \[RequireDerived\] 的基底類別的衍生型別，也同樣確認到被保留。

### **以 IUnityLinkerProcessor 在建置時產生 link.xml**

若要保留的程式碼已確定，只需在 link.xml 中指定目標類別或成員即可。

另一方面，在每個建置目標要保留的型別不同、必要的型別隨建置所含內容而變等情況下，持續手動更新 link.xml 並不切實際。

使用 \*\*IUnityLinkerProcessor\*\*，可以在建置時動態產生 link.xml 並交給 UnityLinker。

IUnityLinkerProcessor 是編輯器用的介面，將實作類別放在編輯器組件中，建置時就會自動被呼叫。

GenerateAdditionalLinkXmlFile 回傳路徑所指的 link.xml 會被加入 UnityLinker 的輸入。

最小實作如下（{{list:linker_processor}}）。

CODE START csharp | 產生 link.xml 的最小 IUnityLinkerProcessor 實作 {{list:linker_processor}}  
using System.IO;  
using UnityEditor.Build;  
using UnityEditor.Build.Reporting;  
using UnityEditor.UnityLinker;  
using UnityEngine;

public sealed class PreserveListLinkerProcessor : IUnityLinkerProcessor  
{  
    private const string LinkXmlContent \= @"\<linker\>  
  \<assembly fullname=""Assembly-CSharp""\>  
    \<type fullname=""GeneratedPreserveService"" preserve=""all"" /\>  
  \</assembly\>  
\</linker\>  
";

    public int callbackOrder \=\> 0;

    // 回傳路徑所指的 link.xml 會被加入 UnityLinker 的輸入  
    public string GenerateAdditionalLinkXmlFile(BuildReport report, UnityLinkerBuildPipelineData data)  
    {  
        var path \= Path.Combine(Path.GetTempPath(), "managed-stripping-sample-generated-link.xml");  
        File.WriteAllText(path, LinkXmlContent);  
        Debug.Log("\[PreserveListLinkerProcessor\] generated 1 assembly / 1 type.");  
        return path;  
    }  
}  
CODE END

這個範例為了說明而固定了產生內容，實際上應將這部分替換為從建置時的輸入組合而成的處理，例如讀取設定資產或分析內容的相依關係。

準備沒有靜態參照的 GeneratedPreserveService 並以 High 建置後，即使 Assets 底下未放置 link.xml，也確認到型別、建構函式、方法都被保留。

CODE START text  
\[GeneratedLinkXmlProbe\] type=present  
\[GeneratedLinkXmlProbe\] ctor=present, method=GeneratedPreserveService.Describe is alive  
CODE END

參考：\[IUnityLinkerProcessor\](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Build.IUnityLinkerProcessor.html)

###  **Addressables 產生的 link.xml**

對於前一節看到的「只有內容端需要的型別消失」問題，Addressables 的因應方式是將 link.xml 作為內容建置的產物自動產生。

實際確認會產生什麼。內容採最小配置，只建立一個顯示 Cube 網格的 Prefab（在 GameObject 上加上 MeshFilter 與 MeshRenderer），並註冊到 Addressables 的群組。

執行內容建置後，就會產生 link.xml。

在 Addressables 4.0.1 中，產生位置是 Library/com.unity.addressables/aa/\<platform\>/AddressablesLink/link.xml。

本次產生的內容如下（{{list:addressables_linkxml}}）。

CODE START xml | Addressables 產生的 link.xml {{list:addressables_linkxml}}  
\<linker\>  
  \<assembly fullname="Unity.Addressables, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null" preserve="all"\>  
    \<type fullname="UnityEngine.AddressableAssets.Addressables" preserve="all" /\>  
  \</assembly\>  
  \<assembly fullname="Unity.ResourceManager, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null" preserve="all"\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.AssetBundleProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.BundledAssetProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.IAssetBundleResource" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.InstanceProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.SceneProvider" preserve="all" /\>  
  \</assembly\>  
  \<assembly fullname="UnityEngine.CoreModule, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null"\>  
    \<type fullname="UnityEngine.GameObject" preserve="all" /\>  
    \<type fullname="UnityEngine.MeshFilter" preserve="all" /\>  
    \<type fullname="UnityEngine.MeshRenderer" preserve="all" /\>  
    \<type fullname="UnityEngine.Transform" preserve="all" /\>  
  \</assembly\>  
\</linker\>  
CODE END

內容分為兩類。

一類是 Addressables 執行階段（Unity.Addressables 與 Unity.ResourceManager）的 provider 型別，依群組配置而需要。

另一類是已註冊內容所參照的型別。本次組成 Prefab 的 GameObject、Transform、MeshFilter、MeshRenderer 直接列為保留目標。

可見 Addressables 會分析內容端的相依關係，連應用程式程式碼未參照的型別也納入保留目標。

參考：\[Addressables Build Artifacts\](https://docs.unity3d.com/ja/Packages/com.unity.addressables@1.20/manual/BuildArtifacts.html)（關於 link.xml 的說明位於 1.20 版的手冊）

### **將透過反射的程式碼呼叫替換為 Source Generator 的靜態呼叫**

到目前為止的因應方式，都是指定保留以免必要的程式碼被刪除。

相對地，也有另一種方法：以 \*\*Source Generator\*\* 的程式碼產生，取代造成靜態分析看不見的相依關係的反射本身。

Source Generator 是 C\# 編譯器在編譯時分析原始碼並產生額外原始碼的機制。

若在編譯時就完成型別的搜尋與建立，相依關係從一開始就會表現為靜態參照，也就不再需要保留指定。

舉例來說，考慮一個接收命令名稱字串並執行對應 Handler 的分派處理。

Handler 以屬性宣告命令名稱。若在執行階段以反射來實現，就會是從組件中尋找帶有屬性的型別並建立的結構（{{list:command_handlers}}）。

CODE START csharp | 以屬性宣告命令名稱的 Handler {{list:command_handlers}}  
\[AttributeUsage(AttributeTargets.Class)\]  
public sealed class CommandHandlerAttribute : Attribute  
{  
    public CommandHandlerAttribute(string command)  
    {  
        Command \= command;  
    }

    public string Command { get; }  
}

public interface ICommandHandler  
{  
    string Execute();  
}

// Handler 未被任何地方靜態參照  
\[CommandHandler("greet")\]  
public sealed class GreetCommandHandler : ICommandHandler  
{  
    public string Execute() \=\> "GreetCommandHandler.Execute succeeded";  
}

\[CommandHandler("farewell")\]  
public sealed class FarewellCommandHandler : ICommandHandler  
{  
    public string Execute() \=\> "FarewellCommandHandler.Execute succeeded";  
}  
CODE END

兩個 Handler 都未被任何地方靜態參照，因此若維持執行階段掃描的設計並以 High 建置，就會如前一節所見整個型別被刪除。

以 Source Generator 來實現這個依命令名稱解析 Handler 的處理。

準備一個產生器，在編譯時收集加上 \[CommandHandler\] 的類別，並產生依命令字串查出具體型別的 Dispatcher。

產生器的工作分為兩個階段：前半（ForAttributeWithMetadataName）從帶屬性的類別收集命令名稱與型別名稱，後半（GenerateDispatcher）將收集到的配對填入範本。

以下列出產生器的完整程式碼（{{list:dispatcher_generator}}）。

CODE START csharp | 產生 Dispatcher 的 Source Generator {{list:dispatcher_generator}}  
\#nullable enable  
using System.Collections.Immutable;  
using System.Linq;  
using System.Text;  
using Microsoft.CodeAnalysis;  
using Microsoft.CodeAnalysis.CSharp;  
using Microsoft.CodeAnalysis.Text;  
namespace StrippingSample.SourceGenerators  
{  
    \[Generator\]  
    public sealed class CommandDispatcherGenerator : IIncrementalGenerator  
    {  
        public void Initialize(IncrementalGeneratorInitializationContext context)  
        {  
            // 從加上 \[CommandHandler\] 的類別收集命令名稱與型別名稱的配對  
            var handlers \= context.SyntaxProvider  
                .ForAttributeWithMetadataName(  
                    "CommandHandlerAttribute",  
                    static (\_, \_) \=\> true,  
                    static (syntaxContext, \_) \=\> (  
                        Command: (string)syntaxContext.Attributes\[0\].ConstructorArguments\[0\].Value\!,  
                        TypeName: ((INamedTypeSymbol)syntaxContext.TargetSymbol)  
                            .ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat)))  
                .Collect();  
            // 從收集到的配對產生 Dispatcher  
            context.RegisterSourceOutput(handlers, static (sourceContext, commandHandlers) \=\>  
            {  
                if (\!commandHandlers.IsDefaultOrEmpty)  
                {  
                    sourceContext.AddSource(  
                        "CommandDispatcher.g.cs",  
                        SourceText.From(GenerateDispatcher(commandHandlers), Encoding.UTF8));  
                }  
            });  
        }  
        private static string GenerateDispatcher(  
            ImmutableArray\<(string Command, string TypeName)\> handlers)  
        {  
            // 為使輸出具有確定性，先依命令名稱排序再填入  
            var cases \= string.Join("\\n", handlers  
                .Distinct()  
                .OrderBy(static handler \=\> handler.Command, System.StringComparer.Ordinal)  
                .Select(static handler \=\> $$"""  
                            case {{SymbolDisplay.FormatLiteral(handler.Command, true)}}:  
                                return new {{handler.TypeName}}().Execute();  
                """));  
            return $$"""  
                // \<auto-generated/\>  
                public static class CommandDispatcher  
                {  
                    public static string Execute(string command)  
                    {  
                        switch (command)  
                        {  
                {{cases}}  
                            default:  
                                throw new global::System.ArgumentException($"Unknown command: {command}");  
                        }  
                    }  
                }  
                """;  
        }  
    }  
}  
CODE END

依命令名稱排序，是為了讓相同輸入產生相同程式碼，避免每次建置輸出都不一樣。

透過這個產生器，編譯時會產生如下的程式碼（{{list:generated_dispatcher}}）。

CODE START csharp | 產生的 CommandDispatcher {{list:generated_dispatcher}}  
// \<auto-generated/\>  
public static class CommandDispatcher  
{  
    public static string Execute(string command)  
    {  
        switch (command)  
        {  
            case "farewell":  
                return new global::FarewellCommandHandler().Execute();  
            case "greet":  
                return new global::GreetCommandHandler().Execute();  
            default:  
                throw new global::System.ArgumentException($"Unknown command: {command}");  
        }  
    }  
}  
CODE END

原本在執行階段依字串尋找型別的處理，變成了字串與具體型別的對照表。

建構（new）與 Execute 的呼叫都出現在產生的程式碼中，因此對型別、建構函式、方法的靜態參照在產生的程式碼內就已完整。

使用端只需傳入命令字串，完全不參照 Handler 的具體型別。

與反射實驗相同，命令字串經由 static readonly 欄位傳入，維持為 UnityLinker 無法追蹤的輸入（{{list:dispatch_probe}}）。

CODE START csharp | 產生的 Dispatcher 的使用端 {{list:dispatch_probe}}  
// 命令字串經由欄位傳入，使其成為 UnityLinker 無法追蹤的輸入  
private static readonly string\[\] Commands \= { "farewell", "greet" };

private static void RunGeneratedDispatcher()  
{  
    foreach (var command in Commands)  
    {  
        var result \= CommandDispatcher.Execute(command);  
        Debug.Log($"\[SourceGeneratorProbe\] dispatch command={command}, result={result}");  
    }  
}  
CODE END

以 High 建置並執行後，確認兩個命令都成功執行。

CODE START text  
\[SourceGeneratorProbe\] dispatch command=farewell, result=FarewellCommandHandler.Execute succeeded  
\[SourceGeneratorProbe\] dispatch command=greet, result=GreetCommandHandler.Execute succeeded  
CODE END

字串本身仍是建置時無法追蹤的輸入，但字串到型別的對應已從執行階段的搜尋變成產生程式碼中的 switch，因此在 UnityLinker 看來，這與手寫程式碼無異，是一般的靜態參照。

以 Mono.Cecil 確認時，兩個 Handler 的型別、建構函式、Execute 以及 CommandDispatcher.Execute 也都被保留。

這種替換的優點不只在於對剝除的耐受性。

\- 由於相依關係對 UnityLinker 可見，不再需要維護 link.xml 或屬性的保留指定  
\- 執行階段不再有反射搜尋，啟動成本與執行階段失敗的可能性都降低  
\- 目標的收集在編譯時進行，因此可在編譯時診斷漏註冊或重複

實際的函式庫中也有採用這類實作的案例。

序列化器方面，MessagePack for C\# 長期為 IL2CPP 環境提供程式碼產生，目前已轉移到以 Source Generator 為基礎的產生方式。

DI 容器方面，VContainer 提供以 SourceGenerator 產生注入程式碼以減少執行階段反射的選項，透過 Source Generator 解決了前一節看到的「建構函式消失導致解析失敗」的症狀。

在 Unity 專案中導入 Source Generator 的步驟（將 netstandard2.0 的 DLL 加上 RoslynAnalyzer 標籤放置的配置），請參閱 Unity 手冊。

參考：\[Create and use a source generator\](https://docs.unity3d.com/6000.3/Documentation/Manual/create-source-generator.html) / \[MessagePack for C\#\](https://github.com/MessagePack-CSharp/MessagePack-CSharp) / \[MemoryPack\](https://github.com/Cysharp/MemoryPack) / \[VContainer\](https://github.com/hadashiA/VContainer)

# **總結**

Managed Code Stripping 並不是提高 Stripping Level 就算完成的最佳化。

如本章所見，透過反射或外部內容的相依關係在靜態分析中看不見，即使是執行階段必要的程式碼也會從建置中消失。

而且症狀只會出現在建置後的應用程式中，因此必須將「明確標示看不見的相依關係」與「持續以建置後的應用程式驗證」配套執行。

明確指定保留的手段，依目標與情況區分使用（{{table:approach_selection}}）。

表: 因應方式的區分使用 {{table:approach_selection}}

| 方法 | 適用情況 |
| :---- | :---- |
| \[Preserve\] | 保留自己可編輯程式碼中的少數元素 |
| link.xml | 保留包含無法編輯的 DLL 在內的固定目標 |
| Require 系列屬性 | 在框架端宣告「保留實作或衍生」的契約 |
| IUnityLinkerProcessor | 保留目標隨建置目標或內容而變 |
| Source Generator | 將反射搜尋本身替換為靜態參照 |

此外，列舉讓剝除更容易發揮效果的方針。

* 除了會造成問題的情況外，不過度使用 \[Preserve\] 或 link.xml  
* 減少以反射搜尋與建立型別，考慮改為明確註冊或以 Source Generator 轉為靜態參照  
* 程式碼保留的指定盡可能縮小到型別、成員為單位。整個組件的 preserve="all" 僅限用於排查問題  
* 與應用程式分開建置的內容專用型別的保留，不靠手動作業，而是善用 Addressables 的 link.xml 或 IUnityLinkerProcessor 產生等機制  
* 從開發期間就執行並驗證提高 Stripping Level 的建置
