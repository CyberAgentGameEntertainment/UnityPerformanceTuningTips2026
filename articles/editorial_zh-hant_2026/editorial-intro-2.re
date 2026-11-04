=[nonum] 2026 年的腳本與動畫動向
#@# 原稿は sources/editorial-intro-2.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

==[nonum] 腳本的動向

在 Unity 6 世代，腳本方面的情況並沒有太大變化。不過 Unity 7 即將加入 @<b>{CoreCLR} 與 @<b>{ECS for All} 這兩項會大幅改變 C# 程式碼效能調優前提的功能，Unity 6 世代也已針對此進行了部分 API 變更。本章除了介紹 Unity 6 世代的 API 之外，也會介紹 Unity 7 預定的部分變更。此外，有關 Unity 7 的資訊為撰寫本文時的內容，可能與實際發佈的內容有所不同，敬請見諒。

=== 非同步處理

@<b>{Awaitable} 是 Unity 2023 登場的 Unity 標準類別，過去需要使用 @<b>{UniTask} 等外部函式庫才能進行的高效 async / await 處理，現在只用 Unity 標準類別就能撰寫。不過 Awaitable 的功能集比 UniTask 少，因此若已在使用 UniTask，繼續使用 UniTask 也無妨。在無法依賴外部套件的情況下，使用 Awaitable 取代 Task、ValueTask 或協程，可望提升效能。此外，第 2 部第 5 章解說了非同步處理中容易出問題的 CancellationToken 管理。

=== .NET 執行階段

Unity 6 目前的 .NET 執行階段與以往相同，有 Mono 與 IL2CPP，可使用 .NET Standard 2.1 的 API 集與 C# 9。Unity 7 將淘汰 Mono，並預定在編輯器與部分桌面播放器中導入 @<b>{CoreCLR}。CoreCLR 是 .NET 本家（.NET Core / .NET）所使用的執行階段實作，在效能上領先 Mono。行動裝置與主機平台暫時不會導入 CoreCLR，IL2CPP 仍是首選，但除了編輯器將轉移到 CoreCLR 之外，執行時可用的標準 API 的新增與實作的最佳化等部分也能受益。

==== CoreCLR 的 JIT

CoreCLR 對將 C# 程式碼轉換為原生程式碼的 JIT 加入了許多最佳化。在大多數情況下，只要從 Mono 轉移到 CoreCLR，C# 程式碼的執行速度就會提升。根據 Unity 公開的基準測試結果，在不使用 GC 的工作負載下，CoreCLR 的執行速度為 Mono 的 2 到 3 倍，可獲得與 IL2CPP 或 Burst 相當的效能@<fn>{fn1}。以往，特別是在可於 Mono 與 IL2CPP 之間選擇的桌面平台上，從效能角度來看 IL2CPP 是最佳選擇，但 CoreCLR 登場之後，這個前提也可能改變。

==== .NET 10 的 BCL

以往 .NET BCL@<fn>{fn2}使用的是 Mono 隨附的實作，限制在相當於 .NET Standard 2.1 的 API 集，但隨著 CoreCLR 的導入，預計將更新為相當於 .NET 10 的 BCL。可用的 API 會大幅增加，內部實作也更加精良，可望改善效能。新的 BCL 預定也能在 IL2CPP 上使用，因此在不導入 CoreCLR 的行動裝置與主機平台上，也能在效能與功能方面受益。

==== CoreCLR 的 GC / 記憶體管理

CoreCLR 的垃圾回收器（Garbage Collector，GC）實作也與 Mono / IL2CPP 不同。以往 Unity 的 Mono 與 IL2CPP 使用的是稱為 @<b>{Boehm GC}（Boehm-Demers-Weiser GC、bdwgc）的 GC 實作，而 CoreCLR 的 GC 具有與 Boehm GC 大不相同的特性@<fn>{fn3}。

首先，CoreCLR GC 是@<b>{世代式 GC（generational GC）}。世代式 GC 將已配置的物件依稱為「世代」的群組來管理。新配置的物件首先被視為 gen0。針對 gen0 的 GC 相對頻繁地執行，但在 gen0 GC 中存活的物件會移到下一個世代 gen1，並以比 gen0 更低的頻率執行 GC。這是基於「新配置的物件大多壽命很短，而存活到一定程度的物件之後往往也會長期存活」這一普遍的觀察，目的是縮短對年輕世代頻繁執行的 GC 所花費的時間。在 CoreCLR GC 的記憶體最佳化中，已配置的物件屬於哪個世代，以及針對哪個世代的 GC 在何時執行等，將是重要的重點。

此外，CoreCLR GC 會進行@<b>{壓縮（compaction）}。Boehm GC 不會移動一旦配置的物件，因此反覆配置與釋放物件後，堆積會逐漸碎片化，容易導致堆積變得比實際所需的大小更大。CoreCLR GC 的壓縮會在碎片化的堆積上移動物件，填補物件之間的空隙。這樣可以抑制堆積的擴大。

另外，IL2CPP 仍會使用 Boehm GC，因此與以往相同的記憶體最佳化策略依然有效@<fn>{fn4}。

除了 GC 之外，CoreCLR 也積極進行 JIT 的記憶體配置最佳化。例如 @<b>{Object Stack Allocation} 是 JIT 的一項功能，會將僅在方法內使用完畢的極短命物件配置在堆疊而非堆積上。特別是在迴圈內部等熱點路徑中大量配置的短命物件，可能不會像在 Mono 或 IL2CPP 中那樣引發問題。

==== C# 14 支援

Unity 6 目前正式支援的是 C# 9，但 Unity 7 預定可使用 C# 14。近年的 C# 版本也新增了許多用於改善效能的功能，因此從效能調優的角度來看，應該了解的語言功能也會增加。

==== Unity 6 世代已先行導入的功能

CoreCLR 預定在 Unity 7 正式導入，但在部分平台上，Unity 6 系列已開始實驗性地導入 CoreCLR。在撰寫本文時最新的 Unity 6.7 alpha（6000.7.0a6）中，Windows、macOS、Linux 的獨立播放器可使用 CoreCLR。不過 API 集仍限制在 .NET Standard 2.1。同樣在 Unity 6.7 alpha 中，iOS 播放器（IL2CPP）可使用相當於 .NET 10 的 BCL 實作，但這也限制在 .NET Standard 2.1。就現階段而言，應認為它只能用於針對 Unity 7 的 CoreCLR 轉移進行效能與相容性的驗證。

=== DOTS

Unity 6 中 DOTS（Data-Oriented Technology Stack）功能群仍然有效。可使用運用 LLVM 將 C# 程式碼轉換為高度最佳化的原生程式碼的 @<b>{Burst}、安全地將程式碼多執行緒化的 @<b>{C# Job System}，以及運用它們以比 MonoBehaviour 更快的速度更新遊戲狀態的機制 @<b>{Entity Component System（ECS）}。

Unity 7 預計將透過名為 @<b>{ECS for All} 的功能群強化 ECS 與 GameObject 的整合。以往，作為 ECS 中儲存元件的容器的 Entity 與 GameObject 並不相容，ECS 的狀態是完全獨立於 GameObject 之外管理的。在 Unity 7 中，GameObject 也能作為 Entity 運作，可以對 GameObject 附加 ECS 的元件。由於更容易混合並整合 ECS 與 GameObject，容易成為瓶頸的 MonoBehaviour 的 Update 處理群也更容易移到 ECS，將有助於減輕主執行緒負載並提升影格率。

#@# この節の最後の1行だけが次のページにあふれるため、このページを1行延ばす。
//embed[latex]{
\enlargethispage{\baselineskip}
//}

==[nonum] 動畫的動向

Unity 6 世代對動畫效能進行了幾項改善。Unity 6.3 在 Animator 與 AnimationPlayableGraph 新增了 @<b>{ResetControllerState} 方法，使 Animator 能輕鬆地進行物件池化。詳情請見第 2 部第 2 章的解說。Unity 6.5 新增了 @<b>{AnimationEventInfo} 結構，使接收 Animation Event 時能避免 AnimationEvent 類別所造成的堆積記憶體配置。

//footnote[fn1][Path to CoreCLR | GDC 2026 - Unity - YouTube @<href>{https://youtu.be/_t6xVfrmEWU?si=edILni6YnObyWkoj&t=218}]
//footnote[fn2][Base Class Library。指 System 命名空間等標準類別庫的實作]
//footnote[fn3][Fundamentals of garbage collection - .NET | Microsoft Learn - @<href>{https://learn.microsoft.com/en-us/dotnet/standard/garbage-collection/fundamentals}]
//footnote[fn4][Understanding Memory Management in Unity - Unity Discussion - @<href>{https://discussions.unity.com/t/understanding-memory-management-in-unity/1700107}]
