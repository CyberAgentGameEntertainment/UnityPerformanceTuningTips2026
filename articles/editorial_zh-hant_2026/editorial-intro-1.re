=[nonum] 2026 年的 Unity 圖形動向
#@# 原稿は sources/editorial-intro-1.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

==[nonum] Render Graph 的正式發佈

Unity 6 正式發佈後，截至撰寫本文時已過了將近 2 年。Unity 6 發佈的重點之一，就是 Render Graph 的登場。Render Graph 是基於「以宣告方式描述 1 個影格的資源依賴關係，並在執行前最佳化整個圖」這一概念的機制，如同 Frostbite（DICE）在 2017 年發表的 Frame Graph 與 Unreal Engine 的 RDG（Render Dependency Graph），已被許多現代渲染引擎採用。

藉由描述資源依賴性，可以獲得各種好處，例如 Render Texture 的重複利用、不需要的 Render Texture 的銷毀，以及記憶體載入儲存的自動適當設定。

Unity 先在 HDRP 中導入了它，但 URP 一直沒有，這次終於隨 Unity 6 迎來了期待已久的發佈。

在本書第 1 部第 1 章「Unity 6 Render Graph 圖塊記憶體最佳化」中，將剖析要充分發揮 Render Graph 的威力所需的 TBDR 架構 GPU 與 Render Graph 之間的關聯。

接下來的第 1 部第 2 章「應使用 Render Graph 的 UnsafePass 的情境」，解說了 Render Graph 的好處會減弱的 Unsafe Pass。Render Graph 通常透過使用 Raster Render Pass 或 Compute Pass 來獲得 Render Graph 的最佳化效益。在多數情況下，選擇 Raster Render Pass 或 Compute Pass 是最佳的選擇。在此，我們特別介紹應該選擇 Unsafe Pass 的情境：為了提升效能、為了移植，或為了實作只有 Unsafe 才能實現的功能而適當選擇 Unsafe Pass 的案例。

==[nonum] Render Graph 的登場改變了一切嗎

如上所述，隨著 Render Graph 的登場，渲染系統有了很大的改變。Render Graph 會自動進行最佳化，但若在不理解其機制的情況下使用，就無法充分發揮最佳化的效益。理解並妥善運用新的渲染系統，是身為開發者的我們的責任。

另一方面，若問 Render Graph 的登場是否推翻了以往所有的最佳化概念，答案並非如此。Render Graph 是圖形領域中較高層級的議題。若將目光轉向更接近硬體的低層級，傳統的知識依然有效。例如在著色器中，sin、cos、tan、pow 等超越函數的運算成本很高，這個說法即使在現代的行動環境中仍然適用。第 1 部第 4 章「行動裝置著色器最佳化：超越函數的測量與輕量化」展示了在現代行動環境中使用超越函數時的一例測量結果。接著，介紹幾個作為超越函數輕量化手段的近似函數。我自己也曾想過，在運算效能已經提升的現代行動環境中，近似函數是否仍有必要。本章為這樣的疑問提供了一個答案。第 1 部第 5 章「活用 Renderer Shader User Value 的圖形最佳化」與第 1 部第 6 章「使用 GraphicsStateCollection 進行著色器預熱」所介紹的功能，也是為了解決 Unity 6 以前就存在的問題而新增的功能。

==[nonum] 大量物件繪製的負載

Renderer Shader User Value（RSUV）是 Unity 6.3 起新增的功能。這項功能所要解決的，是自古以來繪製大量物件時的 CPU 負載，特別是 Set Pass Call 的負載。解決這個問題的手段有 SRP Batcher 與 GPU Instancing 等。

然而，SRP Batcher 在「想讓每個物件的外觀稍有不同」的情境下有所限制。SRP Batcher 不支援使用 MaterialPropertyBlock 進行逐 Renderer 的屬性變更，因此若要在維持 SRP Batcher 的同時傳遞個別參數，就必須依想要客製化的 Renderer 數量複製 Material。為了些微的參數差異而增加 Material，無論在記憶體或管理上都不理想。

RSUV 是針對這個問題更輕量的選項。使用 RSUV 時，可以讓每個 Renderer 持有一個 32 位元無符號整數，並由著色器端參照。設定此值不會破壞批次處理，因此能在維持 SRP Batcher 的同時變更參數。雖然有只能傳遞一個 32 位元值的限制，但將顏色、索引、旗標等以位元打包後塞入，就能涵蓋許多用途。

==[nonum] 著色器編譯時的尖峰

使用 GraphicsStateCollection 進行著色器預熱，所解決的是著色器編譯時的尖峰問題。GraphicsStateCollection 在 Unity 6 以 Experimental（實驗性功能）新增，並在 Unity 6.5 作為正式功能發佈。

既有的 ShaderVariantCollection.WarmUp 等預熱 API，在 Vulkan/Metal 等平台上並未如預期運作。了解 Vulkan/Metal 等的著色器預熱機制後就會明白，舊有 API 無法傳遞正確預熱所需的參數。因此在 Vulkan/Metal 上無法完整進行預熱。於是開發者為了避免遊戲進行中的尖峰，必須採取在畫面變黑期間於背景繪製等權宜性的預熱作法。

GraphicsStateCollection 是解決這個問題的新 API。GraphicsStateCollection 是在應用程式執行時收集著色器預熱所需的頂點佈局等各種資訊的機制。收集這些資訊後，就能完美地預熱著色器。不過，GraphicsStateCollection 也有資訊收集遺漏等一些新的問題。這些與其說是 GraphicsStateCollection 本身的問題，不如說有的源自 Unity 的 Shader Variant Strip，有的則源自 Vulkan/Metal 等現代圖形 API 的規格，原因各式各樣。

==[nonum] 邁向第 1 部

隨著 Render Graph 的登場，建構渲染管線的系統有了很大的改變。藉由定義資源依賴性即可銷毀不需要的資源，這是與圖塊式 GPU 相容性也很高的新渲染系統。

然而，即使較高層級的渲染系統煥然一新，也不代表更接近硬體的層級的機制有所改變。Render Graph、GraphicsStateCollection、RSUV 所要解決的，都是以往就存在的問題。

第 1 部不僅介紹 Unity 6 導入的新功能與具普遍性的內容，也介紹了像第 1 部第 3 章「省略 URP 的 FinalBlitPass 的方法」這種明天就能立即運用的 Tips 類內容。

希望這些內容能對各位的開發有所幫助。
