# **iOS 的記憶體上限與虛擬位址空間**

姓名 / 羅馬字：Minhyuk Kim  
所屬：株式會社 CyberAgent SGE 核心技術本部

遊戲可使用的裝置記憶體當然有其上限。在場景切換期間資產重疊、或暫時性的繪製緩衝區增加時，這個上限就會成為問題。

除了降低遊戲內的記憶體峰值之外，記憶體上限本身也有調整的空間。Xcode 的「Increased Memory Limit」是在支援的裝置上提高該上限的設定。另一個設定「Extended Virtual Addressing」則是擴大處理程序可使用的虛擬位址空間。兩者名稱相似，但能處理的範圍並不相同。

本章將介紹這兩個設定的區分使用方式、在實機上的效果，以及在 App Store 上的營運經驗。數值均來自筆者的實機測量，並非 Apple 保證的各裝置上限。

# **記憶體上限與虛擬位址空間是不同的限制**

當 App 發生 OOM（Out of Memory）時，至少需要將以下兩者分開考慮。

表: iOS App 中應區分的兩種記憶體限制 {{table:memory_constraints}}

| 限制 | 主要觀察的值 | 典型的表現 |
| :---- | :---- | :---- |
| 實體記憶體側 | `phys_footprint` | 因記憶體使用量過高而被系統終止（jetsam） |
| 虛擬位址空間側 | VM Region Summary、Virtual Size | 記憶體配置失敗，隨後發生當機 |

實體記憶體與實際需要保留在 RAM 中的分頁有關。在 iOS 上，App 可使用的實體記憶體量並不等於裝置搭載的 RAM 本身，還會受到當下的系統狀態以及套用在 App 上的上限所影響。

Apple 的 `os_proc_available_memory()` 會回傳目前的 App 在達到記憶體上限之前還能配置的量。依 Apple 的說明，此回傳值對應「目前的記憶體上限減去 App 的 memory footprint」，不一定等於裝置的可用實體記憶體。此外，上限在 App 的生命週期中也可能變動，因此不應快取此值，而應視為參考資訊 {{fn:os_proc_available_memory}}。

虛擬位址空間是管理記憶體與檔案對應到哪些位址的範圍。例如，即使對大型檔案執行 `mmap`，其所有分頁也不一定會立即讀入 RAM。但仍需要有虛擬位址來放置該範圍。反過來說，即使實體記憶體的 footprint 仍然很低、`os_proc_available_memory()` 也還有餘裕，仍可能因無法取得新的位址範圍而導致 `malloc` 等失敗。

因此，要區分 {{table:memory_constraints}} 的兩種限制，需要確認是留有 jetsam report，還是在一般的當機報告中以配置失敗為起點。
即使 `os_proc_available_memory()` 回傳較大的值，也不代表虛擬位址空間有同等的餘裕。

# **兩個 Entitlement 的作用**

## **Increased Memory Limit**

Increased Memory Limit 的鍵為 `com.apple.developer.kernel.increased-memory-limit`。Apple 說明這是一個 Entitlement，用來告知系統：在支援的裝置上超過預設的 App 記憶體上限，可以改善 App 的核心功能 {{fn:increased_memory_limit}}。在 Xcode 的 Capability 清單中，也會如 {{fig:xcode_increased_memory_limit}} 所示顯示附帶「在支援的裝置上」條件的說明。

![][image1]{scale=0.8}
図: Xcode 的 Capability 清單中顯示的 Increased Memory Limit {{fig:xcode_increased_memory_limit}}

也就是說，並非只要指定就會在所有裝置上加上固定的量。Apple 的文件中也記載，只有部分裝置能利用提高後的上限，並要求 App 在無法利用額外記憶體時也必須正常運作。

### **Increased Memory Limit 的實機測量**

表: 有無 Increased Memory Limit 時 OOM 發生時的記憶體測量結果（iPhone） {{table:increased_memory_observations}}

| 搭載 RAM | 停用時 | 啟用時 |
| :---- | ----: | ----: |
| 3GB | 約 1,850MB | 約 1,850MB |
| 4GB | 約 2,050MB | 約 2,290MB |
| 6GB | 約 3GB | 約 4GB |

4GB 與 6GB 的 iPhone 記憶體各自有所增加，而 3GB 的 iPhone 在啟用與停用之間則沒有差異。
另外，iPad 在 OOM 發生時的記憶體量原本就與 iPhone 不同，因此啟用時的結果很可能也不一樣。
如果目標使用者的裝置中 3GB 的 iPhone 占比較高，就不應期待僅靠加上 Increased Memory Limit 便能解決問題。

## **Extended Virtual Addressing**

Extended Virtual Addressing 的鍵為 `com.apple.developer.kernel.extended-virtual-addressing`。Apple 說明這是一個表示可存取更大位址空間的 Boolean 值，並以遊戲作為範例 {{fn:extended_virtual_addressing}}。

![][image2]{scale=0.8}
図: Xcode 的 Capability 清單中顯示的 Extended Virtual Addressing {{fig:xcode_extended_virtual_addressing}}

此設定不會增加 RAM。對於接近實體記憶體上限而被 jetsam 的 App，僅加上 Extended Virtual Addressing 並不保證能繼續使用同樣多的 RAM。可以期待效果的情況是：由於大型的記憶體映射、碎片化的位址區域、大型的固定保留等因素，在用盡實體記憶體之前虛擬位址空間就先變得吃緊。

啟用本身幾乎沒有成本。分頁表只會為實際映射的範圍建立，因此即使擴大上限也不會增加 {{fn:xnu_pmap}}。該考慮的是是否存在能產生效果的裝置。加上 Increased Memory Limit 時位址空間也會隨之擴大，所以有些裝置即使兩者都加上，`max_address` 也不會改變。

另一方面，在像 {{table:increased_memory_observations}} 的 3GB 裝置那樣 Increased Memory Limit 不會產生效果的裝置上，Extended Virtual Addressing 便是擴大位址空間的唯一手段。Increased Memory Limit 在裝置不支援時，即使簽章正確也會失效，但 Extended Virtual Addressing 沒有這個條件。

### **虛擬位址空間的上限值**

Extended Virtual Addressing 擴大的是與 footprint 上限不同的位址空間上限。此上限由 `pmap_max_64bit_offset()` 依搭載記憶體回傳的 {{table:va_ceilings}} 其中之一的值決定 {{fn:xnu_pmap}}。

表: arm64 使用者處理程序的位址空間上限 {{table:va_ceilings}}

| 條件 | 常數 | 上限 |
| :---- | :---- | ----: |
| `max_mem` > 3GB | `ARM64_MAX_OFFSET_DEVICE_LARGE` | 17.375GB |
| `max_mem` 超過 1GB 至 3GB | `ARM64_MAX_OFFSET_DEVICE_SMALL` | 13.375GB |
| `max_mem` 1GB 以下 | `ARM64_MIN_MAX_ADDRESS` | 12.5GB |
| 以包含 entitlements 在內的任一手段擴展 | `MACH_VM_JUMBO_ADDRESS` | 63GB |

`max_mem` 是核心所認知的實體記憶體，會比規格表上的 RAM 容量小。
這些都是核心的編譯期常數，因此可能隨 OS 版本而改變。

## **在 App Store 上的營運實績**

採用這些 Entitlement 時，令人在意的是會不會影響 App Store 的審查。就筆者所知的多款營運中的遊戲，Increased Memory Limit 與 Extended Virtual Addressing 都有在啟用設定的狀態下通過 App Store 審查並上架的實績。

Apple 自己也在面向遊戲開發者的「Bring your high-end game to iPhone 15 Pro」{{fn:apple_game_memory}} 中，指引開發者啟用 Increased Memory Limit 以增加可用記憶體。這兩者是可以考慮在產品中採用的公開功能。雖然不能保證個別 App 整體的審查結果，但不需要因為「這是與記憶體上限相關的設定，審查時會不會被要求說明或被退回」的顧慮而將其排除在選項之外。

# **確認 App ID 與開發者帳號**

## **無法使用萬用字元的 Profile**

這兩個 Entitlement 無法在萬用字元形式的 App ID 上啟用。
若採用如 `com.example.*` 這樣將多個 App 一併簽章的配置，
就需要切換為與遊戲的 Bundle ID 一致的 Explicit App ID。

若直接嘗試簽章，Xcode 的 Signing & Capabilities 會出現 {{fig:xcode_wildcard_profile_error}} 的錯誤。請注意 Capability 名稱與 Entitlement 的鍵是分開回報的。它同時指出了 Profile 端未包含該 Capability，以及沒有對應該鍵的許可這兩點。

![][image3]
図: 嘗試以萬用字元的 Provisioning Profile 簽章時的錯誤 {{fig:xcode_wildcard_profile_error}}

在 Apple Developer 的 Certificates, Identifiers & Profiles 中選擇目標 App ID，如 {{fig:appid_increased_memory_limit}} 所示啟用所需的 Capability。Apple 也說明，App ID 上設定的 Capability 即為 App 可使用功能的許可清單，而需要 Explicit App ID 的功能無法在萬用字元上選擇 {{fn:explicit_app_id}}。

![][image4]
図: 在 App ID 設定中啟用 Increased Memory Limit {{fig:appid_increased_memory_limit}}

手動簽章時，變更 Capability 後需重新產生 Provisioning Profile，並更新建置所使用的 Profile。自動簽章時，則確認以正確的 Team 與 Bundle ID 選取了更新後的 Profile。若仍使用舊的 Profile，僅在 `.entitlements` 中加入鍵，所要求的權限與簽章端的許可就不會一致。在調查 Unity 端的設定之前，請先把這部分對齊。

## **Extended Virtual Addressing 需要付費會員資格**

Extended Virtual Addressing 無法在以免費 Apple Account 作為 Xcode Personal Team 的配置下啟用。
Apple 的 Capability 對應表中列為適用的是
Apple Developer Program 與 Apple Developer Enterprise Program 的會員資格，
並不包含免費的 Apple Account {{fn:capability_membership}}。

另一方面，Increased Memory Limit 在 Personal Team 下也可以啟用。

# **在 Unity 的建置後處理中自動設定**

Entitlements 檔案中的最小內容如 {{list:memory_entitlements}} 所示。這是同時啟用兩者的範例。

CODE START xml | 記憶體相關 Entitlement 的最小配置 {{list:memory_entitlements}}
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.developer.kernel.increased-memory-limit</key>
    <true/>
    <key>com.apple.developer.kernel.extended-virtual-addressing</key>
    <true/>
</dict>
</plist>
CODE END

雖然也可以從 Xcode 的 Signing & Capabilities 手動加入，但每次匯出都得重新加入，容易混入遺漏設定的建置。在 Unity 中可以透過建置後處理，一併更新 Entitlements 檔案以及參照該檔案的 `CODE_SIGN_ENTITLEMENTS`。此處理在簽章前的建置時執行，並不是在 App 啟動後由 C# 取得權限。

## **Unity 6000.0.11f1 以後使用專用 API**

Unity 6000.0.11f1 在 `ProjectCapabilityManager` 中新增了以下方法 {{fn:unity_memory_api}}。

* `AddIncreasedMemoryLimit()`
* `AddExtendedVirtualAddressing()`

使用這些方法，就能一併完成向 Entitlements 加入 Boolean 值以及反映到 Xcode 專案。

也提供了對應的 `PBXCapabilityType` 定義。不過這不是 C# 的 enum，而是表示 Capability 需求的類別。將此值傳給 `PBXProject.AddCapability()`，與向 Entitlements 加入所需的鍵並不相同。因此基本上使用會同時處理兩者的 `ProjectCapabilityManager`。

以下是使用 `ProjectCapabilityManager` 加入 Capability 的範例。

CODE START csharp | 在 Unity 6000.0.11f1 以後加入記憶體 Capability 的建置後處理 {{list:unity_memory_postprocess}}
#if UNITY_IOS
using UnityEditor;
using UnityEditor.Callbacks;
using UnityEditor.iOS.Xcode;

static class IosMemoryCapabilities
{
    [PostProcessBuild]
    static void OnPostprocessBuild(BuildTarget target, string buildPath)
    {
        if (target != BuildTarget.iOS) return;

        string pbxPath = PBXProject.GetPBXProjectPath(buildPath);
        var pbx = new PBXProject();
        pbx.ReadFromFile(pbxPath);
        string mainTarget = pbx.GetUnityMainTargetGuid();

        string entitlements = pbx.GetBuildPropertyForAnyConfig(
            mainTarget, "CODE_SIGN_ENTITLEMENTS");
        if (string.IsNullOrEmpty(entitlements))
            entitlements = "Unity-iPhone.entitlements";

        var capabilities = new ProjectCapabilityManager(
            pbxPath, entitlements, targetGuid: mainTarget);
        capabilities.AddIncreasedMemoryLimit();
        capabilities.AddExtendedVirtualAddressing();
        capabilities.WriteToFile();
    }
}
#endif
CODE END

`UnityEditor.iOS.Xcode` 包含在 iOS Build Support 中，因此要以 `#if UNITY_IOS` 包起來。否則在未安裝此模組的環境中，整個 Editor 將無法通過編譯，請務必注意。

此範例針對的是所有建置組態共用同一個 Entitlements 檔案，並以相對於 Xcode 專案的路徑指定該檔案的配置。若 Debug 與 Release 使用不同的檔案，或路徑中含有建置變數，請配合實際反映到各組態檔案的處理進行調整。

最後的儲存透過 `capabilities.WriteToFile()` 進行。若在其他建置後處理中也編輯 Xcode 專案，
重新儲存 `pbx` 時可能會覆寫 Capability Manager 已儲存的變更，
請注意要先重新讀取更新後的檔案再進行變更。

## **在沒有專用 API 的版本中直接更新 plist**

要在沒有專用 API 的 Editor 中執行相同處理，請將 {{list:unity_memory_postprocess}} 中從產生 `ProjectCapabilityManager` 到儲存為止的 5 行替換為 {{list:unity_memory_legacy}}。

CODE START csharp | 在沒有專用 Capability API 的 Unity 中的替代處理 {{list:unity_memory_legacy}}
string fullPath = System.IO.Path.Combine(buildPath, entitlements);
System.IO.Directory.CreateDirectory(
    System.IO.Path.GetDirectoryName(fullPath));
var plist = new PlistDocument();
if (System.IO.File.Exists(fullPath))
    plist.ReadFromFile(fullPath);

plist.root.SetBoolean(
    "com.apple.developer.kernel.increased-memory-limit", true);
plist.root.SetBoolean(
    "com.apple.developer.kernel.extended-virtual-addressing", true);
plist.WriteToFile(fullPath);

if (string.IsNullOrEmpty(pbx.FindFileGuidByProjectPath(entitlements)))
    pbx.AddFile(entitlements, entitlements);
pbx.SetBuildProperty(mainTarget, "CODE_SIGN_ENTITLEMENTS", entitlements);
pbx.WriteToFile(pbxPath);
CODE END

先讀取既有檔案再加入兩個鍵，就能在不刪除 Push 通知等 Entitlement 的情況下更新。要變更的是 `.entitlements`，而不是 `Info.plist`。此外，檔案必須由 `CODE_SIGN_ENTITLEMENTS` 參照後才會用於簽章。

僅加入 C# 程式碼並不會連帳號端的權限一併生效。
無論採用哪種方法，Apple Developer 端的 App ID 與 Provisioning Profile 都需要另行設定。

# **分兩階段確認是否套用**

設定是否已反映，分為「簽章中是否包含鍵」與「實機上核心是否已套用」兩個階段來確認。

## **確認是否包含在簽章中**

無論是手動作業還是建置後處理，僅在專案中產生了 {{list:memory_entitlements}} 的檔案，並不能證明已反映到安裝在實機上的 App。Entitlement 最終會嵌入程式碼簽章中。匯出的 `.ipa` 是 zip 格式，因此解壓縮後對 `Payload` 內的 App 本體執行 {{list:codesign_entitlements}}。

CODE START bash | 確認已簽章 App 中嵌入的 Entitlement {{list:codesign_entitlements}}
unzip -q YourApp.ipa -d ipa
codesign --display --entitlements :- ipa/Payload/YourApp.app
CODE END

確認輸出的 plist 中是否包含 {{list:memory_entitlements}} 的兩個鍵。Apple 也指引以此命令在簽章後進行確認 {{fn:diagnosing_entitlements}}。

## **在實機上確認上限是否改變**

即使在 {{list:codesign_entitlements}} 中看得到鍵，也不代表核心實際上已套用。Increased Memory Limit 在裝置端不支援的情況下，即使簽章正確也會被視為無效。

在實機上是否套用，是直接讀取核心對自身處理程序所持有的值來判斷。取得該值的是 `task_info()`，這是 iOS SDK 標準內建的 Mach API（`<mach/mach.h>`）。由於無法從 C# 呼叫，因此由放在 `Assets/Plugins/iOS/` 中的 `.mm` 讀取後回傳給 C#。

CODE START objc | 讀取套用狀況的原生外掛 {{list:task_vm_info_probe}}
#import <mach/mach.h>

extern "C" void UPT_ReadLimits(uint64_t *memlimit, uint64_t *ceiling)
{
    task_vm_info_data_t i;
    mach_msg_type_number_t c = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&i, &c) != KERN_SUCCESS) {
        return;
    }

    // footprint 上限（Increased Memory Limit）
    *memlimit = i.limit_bytes_remaining + i.phys_footprint;
    // 位址空間上限：若已擴大到 0xF_C000_0000 則表示已擴展
    *ceiling  = i.max_address;
}
CODE END

C# 端以 `DllImport("__Internal")` 宣告 `UPT_ReadLimits` 後呼叫。

應觀察的值是上限。`os_proc_available_memory()` 回傳的剩餘量會隨 footprint 增加而減少，因此僅看該值無法區分是上限沒有提高，還是單純用掉了較多。

`limit_bytes_remaining` 與該 `os_proc_available_memory()` 是在核心內由同一函式取得的值 {{fn:xnu_memorystatus}}，因此加上目前的 footprint 就能還原為套用中的上限。位址空間側的 `max_address` 也可以從同一個結構體取得，因此一次呼叫就能確認兩個 Entitlement。`max_address` 不傳入 `TASK_VM_INFO_COUNT` 就不會被填入。

不過，無法從 `max_address` 分離出 Extended Virtual Addressing 單獨的效果。加上 Increased Memory Limit 的建置，或是啟用 Xcode 的 Enhanced Security 的建置，也會得到相同的上限。能確認的只到「上限是否已擴大到 63GB」為止。

# **在 iOS 26.4 中浮現的虛擬位址保留**

## **`JS VM Gigacage (reserved)` 並不是 1GB 的 RAM 使用量**

在搭載 iOS 26.4 的 3GB iPhone 上驗證時，當機報告的 VM Region Summary 中出現了約 1GB 標示為 `JS VM Gigacage (reserved)` 的區域，而啟用 Extended Virtual Addressing 後，便能避免 OOM 發生時記憶體量的下降。
此區域並非真正的 Gigacage，而是沿用既有 VM tag 名稱顯示的 JavaScriptCore Structure Heap 保留 {{fn:apple_forum_826325}}。

## **WebKit 的修正會依裝置 RAM 縮小保留量**

JavaScriptCore 的公開 bug `315240` 中記錄了以下方針：由於在低記憶體裝置上 Structure Heap 的保留會占去虛擬位址空間的很大比例，因此依實體記憶體量縮小保留 {{fn:webkit_bug_315240}}。

在對應此問題的 WebKit commit 中，針對 iOS 系列平台，改為取基本保留大小與實體記憶體 20% 兩者中較小者，
再向下取整至 2 的冪次，以此決定 Structure Heap 的期望保留大小。
公開 commit 的說明中舉出了 3GB 裝置會變為 512MB 的例子 {{fn:webkit_commit_structure_heap}}。

在納入此對應的 iOS 26.6.1 的 iPhone SE（第 2 代，RAM 3GB）上測量，顯示上的保留從約 1GB 縮小為 512MB，
在 26.4 中下降的 OOM 發生時記憶體量，即使不啟用 Extended Virtual Addressing 也已獲得解決。

# **總結**

Increased Memory Limit 與 Extended Virtual Addressing 是即使啟用也幾乎沒有損失的設定。分頁表只會為實際映射的範圍建立，因此擴大位址空間也不會增加成本。也有通過 App Store 審查並上架的實績。筆者認為只要準備好 Explicit App ID 與 Profile，基本上兩者都啟用即可。

不過，效果取決於裝置。在筆者以 iPhone 進行的測量中，Increased Memory Limit 帶來的餘裕在 4GB 裝置上約為 240MB、在 6GB 裝置上約為 1GB，但在 3GB 裝置上則沒有差異。iPad 需要另行測量。Extended Virtual Addressing 不會增加實體記憶體，只有在虛擬位址空間先不足的情況下才有效果。iOS 26.4 中出現的 Structure Heap 保留問題已在 iOS 26.6 系列中緩解，但在支援舊版 OS 的情況下，或 App 本身的記憶體映射占用大量位址空間時，仍可期待其效果。

兩者都不會減少記憶體使用量本身。為了在沒有額外上限的裝置上也能持續運作，仍需另行進行減少紋理與暫時性 RenderTexture 的峰值、資產的常駐期間、原生外掛的大型 allocation、檔案映射的生命週期等工作。這兩個 Entitlement 是在此基礎上用來擴大剩餘餘裕的設定。

脚注: {{fn:increased_memory_limit}} Apple Developer Documentation, `com.apple.developer.kernel.increased-memory-limit` [https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.increased-memory-limit](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.increased-memory-limit)

脚注: {{fn:extended_virtual_addressing}} Apple Developer Documentation, Extended Virtual Addressing Entitlement [https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.extended-virtual-addressing](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.extended-virtual-addressing)

脚注: {{fn:os_proc_available_memory}} Apple Developer Documentation, `os_proc_available_memory` [https://developer.apple.com/documentation/os/os_proc_available_memory](https://developer.apple.com/documentation/os/os_proc_available_memory)

脚注: {{fn:diagnosing_entitlements}} Apple Developer Documentation, Diagnosing Issues with Entitlements [https://developer.apple.com/documentation/bundleresources/diagnosing-issues-with-entitlements](https://developer.apple.com/documentation/bundleresources/diagnosing-issues-with-entitlements)

脚注: {{fn:apple_forum_826325}} Apple Developer Forums, iPadOS 26.4+ significantly reduced per-app memory limit from 6GB to 3GB on 8GB iPad, breaking memory-intensive apps [https://developer.apple.com/forums/thread/826325](https://developer.apple.com/forums/thread/826325)

脚注: {{fn:webkit_bug_315240}} WebKit Bugzilla 315240, Scale down StructureAlignedMemoryAllocator reservation in proportion to phys-mem size [https://bugs.webkit.org/show_bug.cgi?id=315240](https://bugs.webkit.org/show_bug.cgi?id=315240)

脚注: {{fn:webkit_commit_structure_heap}} WebKit commit `cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b`, Scale down structure-heap reservation size with physical memory [https://github.com/WebKit/WebKit/commit/cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b](https://github.com/WebKit/WebKit/commit/cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b)

脚注: {{fn:apple_game_memory}} Apple Developer Tech Talks, Bring your high-end game to iPhone 15 Pro [https://developer.apple.com/videos/play/tech-talks/111372/](https://developer.apple.com/videos/play/tech-talks/111372/)

脚注: {{fn:explicit_app_id}} Apple Developer, Register an App ID [https://developer.apple.com/help/account/identifiers/register-an-app-id/](https://developer.apple.com/help/account/identifiers/register-an-app-id/)

脚注: {{fn:capability_membership}} Apple Developer, Supported capabilities (iOS) [https://developer.apple.com/help/account/reference/supported-capabilities-ios/](https://developer.apple.com/help/account/reference/supported-capabilities-ios/)

脚注: {{fn:unity_memory_api}} Unity 6.0 Scripting API, ProjectCapabilityManager [https://docs.unity3d.com/6000.0/Documentation/ScriptReference/iOS.Xcode.ProjectCapabilityManager.html](https://docs.unity3d.com/6000.0/Documentation/ScriptReference/iOS.Xcode.ProjectCapabilityManager.html)

脚注: {{fn:unity_capability_release}} Unity 6000.0.11f1 Release Notes, iOS: Updated capability list（UUM-65394）。API 的新增內容是透過比較官方發布的 6000.0.10f1 與 6000.0.11f1 的 iOS Build Support 中的 UnityEditor.iOS.Extensions.Xcode.dll 確認。[https://unity.com/releases/editor/whats-new/6000.0.11f1](https://unity.com/releases/editor/whats-new/6000.0.11f1)

脚注: {{fn:xnu_memorystatus}} Apple Open Source, xnu-12377.121.6, `bsd/kern/kern_memorystatus.c`（`memorystatus_available_memory_internal`） [https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/bsd/kern/kern_memorystatus.c](https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/bsd/kern/kern_memorystatus.c)

脚注: {{fn:xnu_pmap}} Apple Open Source, xnu-12377.121.6, `osfmk/arm/pmap/pmap.c`（`pmap_max_64bit_offset`） [https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/osfmk/arm/pmap/pmap.c](https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/osfmk/arm/pmap/pmap.c)

[image1]: assets/kim-minhyuk-2/media/image1.png
[image2]: assets/kim-minhyuk-2/media/image2.png
[image3]: assets/kim-minhyuk-2/media/image3.png
[image4]: assets/kim-minhyuk-2/media/image4.png
