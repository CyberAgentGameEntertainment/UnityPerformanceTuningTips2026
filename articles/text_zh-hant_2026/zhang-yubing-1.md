# **RenderGraph 的 UnsafePass 適用場景**

姓名：ZHANG YUBING  
所屬：株式會社 CyberAgent SGE 核心技術本部

# **前言**

在 Unity 6 中，使用 Render Graph 建構渲染管線時，會依處理內容分別使用 RasterRenderPass、ComputePass 與 UnsafePass。本章介紹應使用 UnsafePass 的場景。

Render Graph 具備相依性分析與 SubPass 合併等最佳化。因此，一般繪製應優先使用 RasterRenderPass，使用 Compute Shader 的處理則應優先使用 ComputePass。

應選擇 UnsafePass 的場景主要有以下 4 種。

* 有 RasterRenderPass 或 ComputePass 無法實現的處理時  
* 需要 CommandBuffer 本身時  
* 想快速從 Compatibility Mode 移轉至 Render Graph 時  
* 一項繪製處理由許多無法合併的 Pass 組成時

前 3 種是不使用 UnsafePass 便無法實現，或實作成本過高的情況。最後 1 種雖非必要，但將多個 Pass 整合為 1 個 UnsafePass，可望降低 CPU 負載並改善 Render Graph Viewer 的可讀性。

接下來依序說明這些場景。

# **應選擇 UnsafePass 的場景**

UnsafePass 並不是指 C\# 的 unsafe 功能。它是一種抑制 Render Graph 部分自動管理與驗證，並可使用接近 CommandBuffer 之 API 的 Pass。

## **有 RasterRenderPass 或 ComputePass 無法實現的處理時**

RasterRenderPass 與 ComputePass 所使用的 RasterCommandBuffer／ComputeCommandBuffer，是只公開 CommandBuffer 中各 Pass 職責所需 API 的包裝類別，因此無法使用以下 API。

* SetRenderTarget：在指定 Load/Store Action 的同時，細緻指定渲染目標、MRT、Mip 與陣列切片  
* CopyTexture：指定 Mip、陣列元素與矩形區域來複製紋理  
* GenerateMips：產生渲染目標的 Mipmap  
* SetRandomWriteTarget/ClearRandomWriteTargets：設定或解除像素著色器的隨機寫入目標  
* RequestAsyncReadback：記錄 GPU 資源的非同步回讀要求

這些 API 可在 UnsafePass 的 UnsafeCommandBuffer 中使用。此外，若透過 CommandBufferHelpers.GetNativeCommandBuffer 取得原生 CommandBuffer，還能使用更進階的 API。

## **需要 CommandBuffer 本身時**

引擎回呼、IMGUI、SDK 的繪製處理等，由外部程式碼接收 CommandBuffer 並記錄任意命令的處理，在執行前無法得知記錄的內容。需在記錄時宣告處理內容的 RasterRenderPass 與 ComputePass，無法納入這類黑箱處理。透過 UnsafePass 取得原生 CommandBuffer 並傳給外部程式碼，即可將其納入 Render Graph。

## **想快速從 Compatibility Mode 移轉至 Render Graph 時**

在 Compatibility Mode 下寫在 Execute 方法中的 CommandBuffer 處理，可將命令序列幾乎原封不動地搬到 UnsafePass 的 SetRenderFunc 內。因此，想以較少工時推進 Render Graph 移轉與運作驗證時相當有效。

不過，這終究只是為了快速移轉的暫時手段。若能確保時間，建議依處理內容分類並移植至 RasterRenderPass 或 ComputePass，UnsafePass 僅限於無法以宣告式 API 表達的部分。

## **由許多無法合併的 Pass 組成時**

將連續且無法合併的 Pass 整合為 1 個 UnsafePass，可降低 CPU 負載並改善 Render Graph Viewer 的可讀性。

### **典型案例**

* Ping-pong 方式的 Blur  
* Bloom 的 Mip Pyramid 產生  
* 多次降取樣與升取樣  
* 在下一個處理中取樣前一個處理輸出的濾波處理

這些處理會在每個 Pass 切換渲染目標，或對前一個 Pass 的輸出進行周邊取樣，因此 SubPass 合併無法成立。

### **整合為 UnsafePass 可降低 CPU 負載**

每註冊 1 個 RasterRenderPass，就會發生以下處理。

* Pass 的註冊與 PassData 的準備  
* 透過 UseTexture 或 SetRenderAttachment 註冊資源相依性  
* Pass 的相依性分析與合併候選的確認  
* Profiling 範圍的開始與結束  
* 渲染目標的設定與執行回呼的呼叫

RasterRenderPass 越多，CPU 端的成本也隨之增加。即使將無法合併的 Pass 鏈註冊為 RasterRenderPass，也無法獲得合併的好處，註冊與管理的成本卻不斷累積。另一方面，若將它們整合為 1 個 UnsafePass，在 Render Graph 上就只會註冊為 1 個 Pass。繪製命令與渲染目標切換的成本不變，但可削減 Render Graph 以 Pass 為單位進行的註冊/管理成本（{{fig:unsafe_pass_pass_chain}}）。

![][image1]  
図: 無法合併的 Pass 鏈在 Render Graph 上的 Pass 數與繪製次數 {{fig:unsafe_pass_pass_chain}}

### **CPU 負載削減效果的測量範例**

本專案在編輯器上進行的測量中，針對 256×256 的 Render Texture 進行 4-tap 的 Ping-pong Blur，比較了每次 Blur 處理各註冊 1 個 RasterRenderPass 的方式，以及在 1 個 UnsafePass 內執行一連串 Blur 處理的方式。測量條件為 Apple M4 Max（macOS 26.5）、Unity 6000.3.19f1、URP 17.3。各組合皆擷取 500 個影格，排除切換後的 60 個影格，採用中位數。測量值為 Render Graph 的記錄、雜湊計算、編譯與執行相關 CPU marker 的合計。

表: Ping-pong Blur 中 Render Graph 相關 CPU 時間的比較 {{table:unsafe_pass_pingpong_cpu}}

| Blur 次數 | RasterRenderPass×N（µs） | UnsafePass×1（µs） | 差異（µs） |
| :---- | :---- | :---- | :---- |
| 5 | 286.4 | 280.0 | 6.4 |
| 10 | 334.8 | 313.6 | 21.2 |
| 20 | 427.8 | 389.1 | 38.7 |
| 50 | 675.9 | 592.9 | 83.0 |
| 100 | 1080.7 | 898.5 | 182.2 |

測量值是用於確認編輯器環境中趨勢的數值。Blur 次數越多，RasterRenderPass 方式與 UnsafePass 方式的差距越大。以線性近似計算，兩種方式的斜率差，亦即 RasterRenderPass 的額外固定成本，每 1 個 Pass 約為 1.8µs。此數值為編輯器上的測量值，並不代表實機上的絕對值，但可確認 CPU 負載的差距會隨 Blur 次數增加。  
此效果來自於削減了 Render Graph 的記錄、執行、Profiling 範圍、渲染目標設定等以 Pass 為單位的註冊/管理成本。由於繪製命令沒有差異，改為 UnsafePass 並不會降低繪製負載。

### **Render Graph Viewer 的可讀性也會改善**

註冊大量 RasterRenderPass 時，Render Graph Viewer 會顯示與註冊數相同數量的 Pass。由於圖形在橫向變長，各功能之間的分界變得難以辨識，資源的相依性也不易掌握（{{fig:raster_pass_viewer}}）。

![][image2]  
図: 每次 Blur 處理各註冊 1 個 RasterRenderPass 時的 Render Graph Viewer 顯示 {{fig:raster_pass_viewer}}

將功能內部的 Pass 鏈整合為 1 個 UnsafePass 後，在 Viewer 中可將其視為代表整個功能的 1 個 Pass 來確認（{{fig:unsafe_pass_viewer}}）。如此一來，各功能之間的分界變得明確，資源的相依性也更容易掌握。![][image3]

図: 將一連串 Blur 處理整合為 1 個 UnsafePass 時的 Render Graph Viewer 顯示 {{fig:unsafe_pass_viewer}}

除了削減 CPU 負載之外，能減少效能分析時的雜訊也是一大優點。

不過，不建議僅為了改善 Render Graph Viewer 的可讀性，就勉強將不同功能的處理或可成立 SubPass 合併的處理整合為 UnsafePass。

# **使用 UnsafePass 時的注意事項**

## **渲染目標的設定**

RasterRenderPass 在記錄時以 SetRenderAttachment 宣告渲染目標，而 UnsafePass 則在執行時呼叫 SetRenderTarget 來設定。

## **宣告相依性**

即使使用 UnsafePass，也不代表完全脫離 Render Graph。所使用的紋理必須透過 UseTexture 宣告讀寫相依性。若宣告不足，資源的生命週期管理與同步將無法正確進行。

在 RasterRenderPass 中，渲染目標的使用是透過 SetRenderAttachment 宣告的，但在 UnsafePass 中，僅呼叫 SetRenderTarget 並不會註冊相依性。渲染目標也必須透過 UseTexture 宣告讀寫相依性。

## **Load/Store Action 的設定**

RenderBufferLoadAction 指定啟用渲染目標時對既有內容的處理方式，RenderBufferStoreAction 則指定繪製完成後對結果的處理方式。在 Tile-Based GPU 上，不必要的 Load 與 Store 會增加圖塊記憶體與主記憶體之間的傳輸，因此適當的設定相當重要。

在 RasterRenderPass 中，Render Graph 會根據宣告的 AccessFlags 自動決定 Load/Store Action。另一方面，在 UnsafePass 中若像 SetRenderTarget(target) 這樣省略 Action，則會套用預設行為 Load／Store，可能因不必要的記憶體傳輸而導致效能下降。

因此，在 UnsafePass 中呼叫 SetRenderTarget 時，務必明確指定 Load/Store Action。

* Load 端：若會覆寫整個畫面則用 DontCare，只有需要讀取既有內容時才用 Load  
* Store 端：若結果會在後續處理中讀取則用 Store，只有任何地方都不會使用時才用 DontCare

# **程式碼範例：Ping-pong Blur 的無法合併 Pass 鏈**

本節介紹實際的程式碼範例。需注意，所列程式碼是為了呈現重點的節錄，並非可直接執行的完整程式碼。

這是交替讀寫 2 張紋理並連續套用 Blur 的範例。先展示以 RasterRenderPass 實作相同處理的情況，接著展示整合為 1 個 UnsafePass 的情況。desc 是 Blur 用的 Render Texture 描述元，blurMaterial 是 Blur 用的材質。

## **RasterRenderPass 方式：每次 Blur 處理各註冊 1 個 Pass**

RasterRenderPass 方式如 {{list:raster_pass_pingpong_blur}} 所示，在迴圈內為每次 Blur 處理呼叫 AddRasterRenderPass。雖然能明確宣告各 Pass 的輸入與輸出，但 Blur 次數越多，Render Graph 上的 Pass 數也隨之增加。

`CODE START csharp | 以 RasterRenderPass 實作 Ping-pong Blur {{list:raster_pass_pingpong_blur}}`  
`class BlurRasterPassData`  
`{`  
    `internal TextureHandle source;`  
    `internal TextureHandle target;`  
    `internal Material material;`  
`}`

`var ping = renderGraph.CreateTexture(desc);`  
`var pong = renderGraph.CreateTexture(desc);`  
`var current = resourceData.activeColorTexture;`  
`var writePing = true;`

`for (var i = 0; i < blurIterations; i++)`  
`{`  
    `var target = writePing ? ping : pong;`

    `using (var builder = renderGraph.AddRasterRenderPass<BlurRasterPassData>(`  
        `"Ping-pong Blur Raster", out var passData))`  
    `{`  
        `passData.source = current;`  
        `passData.target = target;`  
        `passData.material = blurMaterial;`

        `builder.UseTexture(passData.source, AccessFlags.Read);`  
        `builder.SetRenderAttachment(passData.target, 0, AccessFlags.Write);`  
        `builder.SetRenderFunc((BlurRasterPassData data, RasterGraphContext context) =>`  
        `{`  
            `Blitter.BlitTexture(`  
                `context.cmd, data.source, new Vector4(1, 1, 0, 0),`  
                `data.material, 0);`  
        `});`  
    `}`

    `current = target;`  
    `writePing = !writePing;`  
`}`  
`CODE END`

## **UnsafePass 方式：在 1 個 Pass 內執行一連串處理**

UnsafePass 方式如 {{list:unsafe_pass_pingpong_blur}} 所示，只向 Render Graph 註冊 1 次，並在 SetRenderFunc 內切換渲染目標。每次 Blur 處理的繪製命令仍然保留，但以 Pass 為單位的管理可整合為 1 次。

`CODE START csharp | 將 Ping-pong Blur 整合為 1 個 UnsafePass {{list:unsafe_pass_pingpong_blur}}`  
`class PingPongBlurPassData`  
`{`  
    `internal TextureHandle source;`  
    `internal TextureHandle ping;`  
    `internal TextureHandle pong;`  
    `internal int iterations;`  
    `internal Material material;`  
`}`

`var ping = renderGraph.CreateTexture(desc);`  
`var pong = renderGraph.CreateTexture(desc);`  
`var output = blurIterations == 0`  
    `? resourceData.activeColorTexture`  
    `: blurIterations % 2 == 0 ? ping : pong;`

`using (var builder = renderGraph.AddUnsafePass<PingPongBlurPassData>(`  
    `"Ping-pong Blur", out var passData))`  
`{`  
    `passData.source = resourceData.activeColorTexture;`  
    `passData.ping = ping;`  
    `passData.pong = pong;`  
    `passData.iterations = blurIterations;`  
    `passData.material = blurMaterial;`

    `builder.UseTexture(passData.source, AccessFlags.Read);`  
    `builder.UseTexture(passData.ping, AccessFlags.ReadWrite);`  
    `builder.UseTexture(passData.pong, AccessFlags.ReadWrite);`  
    `builder.SetRenderFunc((PingPongBlurPassData data, UnsafeGraphContext context) =>`  
    `{`  
        `var cmd = CommandBufferHelpers.GetNativeCommandBuffer(context.cmd);`  
        `var current = data.source;`  
        `var next = data.pong;`  
        `var alternate = data.ping;`  
        `for (var i = 0; i < data.iterations; i++)`  
        `{`  
            `cmd.SetRenderTarget(`  
                `next,`  
                `RenderBufferLoadAction.DontCare,`  
                `RenderBufferStoreAction.Store);`  
            `Blitter.BlitTexture(`  
                `cmd, current, new Vector4(1, 1, 0, 0),`  
                `data.material, 0);`

            `current = next;`  
            `(next, alternate) = (alternate, next);`  
        `}`  
    `});`  
`}`  
`CODE END`

# **總結**

本章介紹了在 Render Graph 中應使用 UnsafePass 的場景，以及使用時的注意事項。

應選擇 UnsafePass 的場景有以下 4 種。

* 需要 RasterCommandBuffer/ComputeCommandBuffer 未公開的 API 時  
* 需要可傳給外部程式碼的 CommandBuffer 本身時  
* 想快速從 Compatibility Mode 移轉至 Render Graph 時  
* 想將無法合併的 Pass 鏈整合為 1 個 Pass 時

將無法合併的 Pass 鏈整合為 1 個 UnsafePass，可削減以 Pass 為單位的註冊/管理成本，也能改善 Render Graph Viewer 的可讀性。在本專案於編輯器上的測量（{{table:unsafe_pass_pingpong_cpu}}）中，每 1 個 RasterRenderPass 可削減約 1.8µs 的固定成本。不過，由於繪製命令本身不變，需注意這並非以削減 GPU 負載為目的的手法。

另一方面，UnsafePass 也是一種放棄部分 Render Graph 自動管理與驗證的 Pass。使用時，請勿忘記透過 UseTexture 宣告相依性，以及在 SetRenderTarget 中明確指定 Load/Store Action。

基本上優先使用 RasterRenderPass 與 ComputePass，UnsafePass 僅限用於無法以宣告式 API 表達的處理，或整理無法合併的 Pass 鏈，如此便能兼顧 Render Graph 的最佳化與 CommandBuffer 的自由度。

# **參考資料**

* [Unity Manual \- Use the CommandBuffer interface in a render graph (UnsafePass)](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/render-graph-unsafe-pass.html)  
* [Unity Manual \- Optimize a render graph](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/render-graph-optimize.html)  
* [Unity Scripting API \- CommandBuffer](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.CommandBuffer.html)  
* [Unity Scripting API \- RenderBufferLoadAction](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.RenderBufferLoadAction.html)  
* [Unity Scripting API \- RenderBufferStoreAction](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.RenderBufferStoreAction.html)

[image1]: assets/zhang-yubing-1/media/image1.png

[image2]: assets/zhang-yubing-1/media/image2.png

[image3]: assets/zhang-yubing-1/media/image3.png