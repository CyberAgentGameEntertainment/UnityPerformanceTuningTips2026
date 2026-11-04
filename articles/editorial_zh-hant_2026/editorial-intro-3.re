=[nonum] 2026 年的資產、設定與建置動向
#@# 原稿は sources/editorial-intro-3.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

效能調優不只是修正自己的程式碼與資產。即使程式碼與資產相同，只要引擎版本或設定有所不同，建置時間、應用程式容量、記憶體使用量與執行速度就會改變。是否仍用自製機制去承擔引擎端已經解決的問題，是否在不知道新增設定的情況下直接使用預設值。為了確認這些，掌握引擎本身的變化也是調優的前提。

對 Unity 而言，效能也是引擎中優先度很高的領域。Unity 6 世代各版本的 What's New，每次都會在新功能之外列出建置時間的縮短、播放器容量的削減與記憶體管理的改善。第 3 部探討的資產、設定與建置，正是這些變化特別大的領域。

在撰寫本文的 2026 年 9 月，長期支援版的最新版本是 Unity 6.3 LTS，正式版的最新版本是 Unity 6.6。Unity 6.7 作為預定成為 LTS 的版本已公開 Beta 版@<fn>{fn1}，Unity 7 預定在 2026 年 12 月發佈初期的 Beta 版，並在 2027 年第 1 季發佈正式版@<fn>{fn2}。以下將依發佈順序回顧 Unity 6.0 到 6.7 的變化，同一版本中則由效果較大的開始探討。在第 3 部各章中詳細探討的變化僅說明重點，並標示對應的章節。此外，正文中提到的 Unity 版本與預設值皆為撰寫本文時的內容，可能在之後的版本中改變，敬請見諒。

==[nonum] Unity 6.0

=== 以 API 設定 iOS 的記憶體上限

Increased Memory Limit 與 Extended Virtual Addressing 是與應用程式可使用的記憶體上限本身有關的 iOS Entitlement。在以前的 Unity 中，也可以透過手動編輯匯出的 Xcode 專案的 Entitlements 檔案，或在建置後處理中改寫來設定。Unity 6000.0.11f1 之後，在 @<tt>{ProjectCapabilityManager} 中新增了專用的 API，可以一併進行加入 Entitlements 與反映到 Xcode 專案的作業。各自的作用與在實機上確認上限的方法，將在第 3 部第 6 章「iOS 的記憶體上限與虛擬位址空間」中探討。

=== 以 Build Profiles 依組態保存設定

Unity 6.0 導入的 Build Profiles，是可對單一平台持有多個建置組態的機制。從導入時起，每個設定檔就可以持有場景清單、Player Settings 的覆寫，以及腳本 Define Symbols 的追加。從 Unity 6.1 起，Graphics 設定與 Quality 設定也可以依設定檔覆寫@<fn>{fn3}，6.3 改為選擇要覆寫的項目後加入的形式。在 6.5 中，也可以使用 @<tt>{BuildProfile.CreateBuildProfile} 從腳本建立設定檔。

在效能方面的優點是，各平台與發佈目標的設定差異，可以固定為設定檔，而不必靠建置前改寫設定的腳本。例如區分 Define Symbols，就能只在發行用的設定檔中排除除錯用的測量與日誌程式碼；覆寫 Quality 設定，則在低階機種用的組態中，能縮減建置所含的品質等級本身。第 3 部第 3 章「依裝置切換繪製設定」比較了在執行時依裝置效能分配繪製負載的設計，分別採用切換 Quality Level 的形式與直接改寫 Render Pipeline Asset 數值的形式。Build Profiles 可作為在建置時進行切換的手段，與其搭配使用。

=== Managed Stripping Level 的 Low 已棄用

Unity 6.0 的手冊將 Managed Stripping Level 的 Low 列為將來廢止的對象，並建議使用 Minimal 或 Medium@<fn>{fn4}。在使用 Low 的專案中，需要選擇提高到 Medium 以縮小二進位檔，或降低到 Minimal。提高到 Medium 後，由反射參照的程式碼更容易被刪除，其對策將在第 3 部第 4 章「Unity Managed Code Stripping 實踐指南」中探討。

==[nonum] Unity 6.1

=== 依裝置選擇 Graphics Jobs 的模式

Unity 6.1 新增的 Vulkan Device Filter Asset，可以依 Android 的裝置指定 Graphics Jobs 的模式。對於只在特定 GPU 或驅動程式上變慢或不穩定的設定，可以在不改變其他裝置設定的情況下避開。

==[nonum] Unity 6.2

=== 匯入時自動產生的 Mesh LOD

Unity 6.2 新增的 Mesh LOD，是在模型的匯入設定中啟用 Generate Mesh LODs 後，於匯入時自動產生 LOD 的功能@<fn>{fn5}。即使是沒有準備 LOD 的模型，只要一項設定就能減少遠景的多邊形數量，降低 GPU 負載。若已經在使用 LOD Group，減少多邊形的效果不變，差別出現在記憶體。Mesh LOD 不會新增頂點，所有 LOD 共用原始網格的頂點緩衝區，只增加索引緩衝區。相較於每個 LOD 都持有整個網格與 GameObject 的 LOD Group，可以將記憶體的增加抑制得較小。

不過，自動產生的 LOD 的品質取決於原始網格的形狀，無法期待與手工製作的 LOD 相同的品質。官方文件也指出，雖然適合生物或岩石等有機形狀，但對於曲率較高的形狀或分離部件較多的網格，會出現紋理偏移、樹木的葉子消失等破綻。需要在 Inspector 中逐級確認產生的 LOD，並判斷是否從出現破綻的等級起以 Limit LODs 停止產生，或僅讓該網格改回 LOD Group。

另外，能減少的只有多邊形，材質不會改變。Particle System、VFX Graph 與 Static Batching 一律使用 LOD0，因此沒有效果；在 Skinned Mesh Renderer 中，無論選擇哪個 LOD，變形的計算都是針對 LOD0 進行。LOD 的交叉淡化需要 GPU Resident Drawer，且不建議與 LOD Group 併用。

==[nonum] Unity 6.3

=== 以設定減少著色器變體

著色器變體會依關鍵字的組合各自編譯，因此關鍵字越多，變體數量就以乘積的方式增加。減少變體，就能同時減少建置時間、建置容量、著色器的載入時間與執行時的記憶體。

在 Unity 6.3 新增的 Shader Build Settings 中，不需改寫著色器的原始碼，就能將關鍵字的種類覆寫為 @<tt>{shader_feature} 或 @<tt>{dynamic_branch}，或是將特定關鍵字的程式碼從建置中排除@<fn>{fn6}。此設定可依 Build Profile 覆寫，因此能針對各平台只刪減不需要的變體。排除內建關鍵字有時會連必要的變體也一併消失，因此必須同時使用可偵測缺漏的 Strict Shader Variant Matching。對於建置中保留下來的變體，其執行時的記憶體可透過 2021 LTS 起即可使用的 Shader Variant Loading 設定，限制所載入區塊的大小與數量來抑制@<fn>{fn7}。

==[nonum] Unity 6.4

=== 隨編輯器內建的 Project Auditor

在 Unity 6.4 中，Project Auditor 已隨編輯器內建。它能以機械化的方式找出 Addressables 的 Bundle 之間重複等膨脹問題。6.4 之後，診斷規則已拆分為獨立的套件，因此導入步驟依版本而異。調查的步驟將在第 3 部第 2 章「營運階段的資產膨脹與以 Project Auditor 進行調查」中探討。

=== 停用 Physics 模組

在 Unity 6.4 中，可以從 Package Manager 停用內建的 Physics 模組@<fn>{fn8}。對於不使用 3D 物理的遊戲，可相應地減少建置容量。不過，若使用 Local Volume，則必須保持啟用。

==[nonum] Unity 6.5

=== 不再隱含啟用 Read/Write

啟用紋理或網格的 Read/Write 後，除了傳送到 GPU 的資料之外，CPU 端也會保留一份副本，並因此消耗記憶體。以往的 Unity 在 Particle System 的 Shape 模組、Terrain 的細節，或 Mesh Collider 需要從 CPU 讀取時，會在建置時隱含地啟用此設定。在 Unity 6.5 針對紋理、Unity 6.6 針對網格，已取消這種隱含啟用，若未在需要的資產上明確啟用，Inspector 會顯示警告，且建置會失敗。

官方文件以縮短建置時間作為這項變更的理由，但在記憶體方面也有意義。由於哪些資產持有 CPU 端的副本，會以資產設定的形式顯現，因此可以逐一判斷是否真的需要從 CPU 讀取，成為減少不必要副本的線索。

=== Android 的 LTO

Unity 6.5 在 Android 新增了連結時最佳化（LTO）的設定。這個開關只對引擎端的函式庫有效，且以停用 Strip Engine Code 為前提，因此需要比較執行速度的改善與容量的增加來判斷是否採用。實測結果將在第 3 部第 5 章「Unity 的 LTO 設定與 Android、iOS 上的效能比較」中探討。

=== 新增 mimalloc

Unity 6.5 新增了 mimalloc 作為原生記憶體的配置器。mimalloc 是可讓多個執行緒同時配置與釋放記憶體的配置器，在 6.5 中為選用，Unity 6.6 則預設啟用@<fn>{fn9}。在 Job 或工作執行緒的原生記憶體配置重疊的情境下，可望減少執行緒之間的等待。另一方面，配置器改變後，記憶體使用量與碎片化的傾向也會改變，官方也未提供效果的數值。更新到 6.6 時，事先與更新前比較原生記憶體的變化會比較安全。若有問題，可從 Memory Settings 停用。

=== 依 iOS 熱狀態調整目標影格率

在 Unity 6.5 中，當 iOS 的熱狀態變為 Serious 或 Critical 時的目標影格率，可以在 Player Settings 中指定。不需自行撰寫發熱時降低影格率的處理，就能緩和熱節流（thermal throttling）造成的急遽效能下降。

=== 分離 AssetBundle 的 TypeTree

AssetBundle 含有 TypeTree，也就是序列化資料的型別資訊。由於有 TypeTree，即使建置 Bundle 時與播放器的 Unity 版本不同也能載入，但相同型別的資訊會在每個 Bundle 中重複包含，而推高容量。在 Unity 6.5 與 Addressables 2.9.1 之後，啟用 Extract TypeTree Data 即可將 TypeTree 分到獨立檔案，減少 AssetBundle 的容量@<fn>{fn10}。發佈的 Bundle 越多的遊戲，對下載量與裝置儲存空間兩者的效果越大。不過，此設定會變更所有已經發佈的 AssetBundle，因此官方建議用於新專案。

==[nonum] Unity 6.6

=== 作為 AssetBundle 後繼的 Content Directories

Unity 6.6 新增了 Content Directories，作為 AssetBundle 後繼的新內容建置機制@<fn>{fn11}。官方文件列出的優點是隱含的重複排除、載入時間的縮短與記憶體額外負擔的削減。在 AssetBundle 中，被多個 Bundle 參照的資產會複製到各個 Bundle，容量與記憶體可能被重複消耗。第 3 部第 2 章「營運階段的資產膨脹與以 Project Auditor 進行調查」所探討的 Bundle 之間的重複，在 Content Directories 中會被自動排除。資產的載入與釋放使用 @<tt>{Loadable<T>} API 進行，也支援 async/await。Addressables 4.0 新增了 Content Directory 用的 Schema，可轉換既有的群組@<fn>{fn12}。

不過，Content Directories 目前僅支援本機內容的建置，不支援遠端發佈。Beta 版的 Unity 6.7 也是如此，對遠端內容的支援預定在 Unity 7 世代推出@<fn>{fn13}。在 Addressables 4.0 中，若轉換具有遠端載入路徑的群組，Schema 也會以無效狀態加入。對於以下載來增加內容的許多行動遊戲，發佈的內容仍將以 AssetBundle 處理，並從隨應用程式附帶的內容開始考慮轉移。另有一項限制：參照循環中所含資產的合計超過 64MB 時，建置會失敗。

無論使用哪一種機制，「何時載入資產、何時釋放」的設計都同樣適用。第 3 部第 1 章「AssetBundle 的生命週期管理」將載入與釋放集中到基礎層，並在載入時就決定資產的壽命，藉由結構來防止釋放遺漏與不需要的資產常駐。

=== 依 Build Profile 指定著色器常數

在 Unity 6.6 中，著色器常數的值可以針對整個專案或各 Build Profile 指定@<fn>{fn14}。只有數值因平台而異的處理，不必用關鍵字增加變體就能分別撰寫。

=== Build History 與 Build Analysis

Unity 6.6 新增了 Build History 與 Build Analysis 視窗，播放器的建置報告會自動累積。由於可以追蹤每次建置的容量變化，更容易找出造成膨脹的變更。不過，對 AssetBundle 建置的支援有限。

==[nonum] Unity 6.7（發表階段）

以下功能是在 2026 年 7 月的 Unite Seoul 上發表的，是撰寫本文時尚無法在文件中確認詳情的發表階段資訊@<fn>{fn15}。

=== Neural Texture Compression

Neural Texture Compression（NTC）是使用已訓練模型，幾乎無劣化地壓縮紋理的技術。在發表中，展示了在外觀沒有劣化的情況下，將紋理的執行時記憶體削減 50% 以上、磁碟容量削減約 70% 的範例。NTC 被介紹為從 Unity 6.7 起陸續提供的 Unity Neural 的一部分。

=== Fast Build Profile

Fast Build Profile 是預定在 Unity 6.7 新增的新建置設定檔，據稱可將著色器的建置時間最多縮短 90%。同時也介紹了透過 PSO 的追蹤與預熱來減少執行時卡頓的機制。

==[nonum] 邁向第 3 部

回顧 Unity 6.0 到 6.7，引擎端的變化正朝向讓以往靠自製腳本或營運來彌補的事項，能夠以設定的形式持有。另一方面，這些變化並非維持預設值等待就會產生效果。像 mimalloc 這類只要更新行為就會改變的項目，需要以更新前後的測量來確認。像 Content Directories 這樣，依發佈需求而尚無法轉移的項目也存在。何時載入並釋放資產、將程式碼削減到什麼程度、依裝置切換什麼，這些設計上的判斷，即使引擎版本改變也依然存在。

第 3 部的各章將搭配實際專案的案例，探討這些判斷所需的設計與測量。若能作為考慮更新到新版本時的判斷素材，將深感榮幸。

//footnote[fn1][Unity Discussions, Unity 6.7 Beta is now available @<href>{https://discussions.unity.com/t/unity-6-7-beta-is-now-available/1736830}]
//footnote[fn2][Unity, Unity 7 roadmap revealed at Unite Seoul @<href>{https://unity.com/news/unity-7-roadmap-revealed-at-unite-seoul}]
//footnote[fn3][Unity Manual 6000.1, What's new in Unity 6.1 @<href>{https://docs.unity3d.com/6000.1/Documentation/Manual/WhatsNewUnity61.html}]
//footnote[fn4][Unity Manual 6000.0, Configure managed code stripping @<href>{https://docs.unity3d.com/6000.0/Documentation/Manual/managed-code-stripping-configure.html}]
//footnote[fn5][Unity Manual 6000.6, Introduction to Mesh LOD @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/lod/mesh-lod-introduction.html}]
//footnote[fn6][Unity Manual 6000.3, What's new in Unity 6.3 @<href>{https://docs.unity3d.com/6000.3/Documentation/Manual/WhatsNewUnity63.html}]
//footnote[fn7][Unity Blog, Improvements to shader build times and memory usage in 2021 LTS @<href>{https://unity.com/blog/engine-platform/2021-lts-improvements-to-shader-build-times-and-memory-usage}]
//footnote[fn8][Unity Manual 6000.4, What's new in Unity 6.4 @<href>{https://docs.unity3d.com/6000.4/Documentation/Manual/WhatsNewUnity64.html}]
//footnote[fn9][Unity Manual 6000.6, mimalloc @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/performance-mimalloc.html}]
//footnote[fn10][Unity Manual 6000.5, What's new in Unity 6.5 @<href>{https://docs.unity3d.com/6000.5/Documentation/Manual/WhatsNewUnity65.html}]
//footnote[fn11][Unity Manual 6000.6, Introduction to content directories @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/content-directories-introduction.html}]
//footnote[fn12][Addressables 4.0 Manual, Content build systems @<href>{https://docs.unity3d.com/Packages/com.unity.addressables@4.0/manual/content-build-systems.html}]
//footnote[fn13][Unity Blog, Content directories: Beyond the AssetBundle @<href>{https://unity.com/blog/content-directories-beyond-the-assetbundle}]
//footnote[fn14][Unity Manual 6000.6, What's new in Unity 6.6 @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/WhatsNewUnity66.html}]
//footnote[fn15][Unity Blog, Unite Seoul 2026 keynote recap @<href>{https://unity.com/blog/unite-seoul-keynote-2026-recap}]
