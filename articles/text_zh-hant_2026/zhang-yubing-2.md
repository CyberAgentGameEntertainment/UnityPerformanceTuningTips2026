# **省略 URP 的 FinalBlitPass 的方法**

姓名：ZHANG YUBING  
所屬：株式會社 CyberAgent SGE 核心技術本部

# **什麼是 FinalBlitPass**

在 URP 的渲染管線中，實際顯示在畫面上的是 BackBuffer。  
但 BackBuffer 是唯寫的，無法讀取。因此當存在後處理這類需要讀取中間結果作為輸入的處理時，相機會先繪製到中間紋理，最後再 Blit 到 BackBuffer。  
負責將這個中間紋理 Blit 到 BackBuffer 的，就是 URP 的 FinalBlitPass。  
它並非單純的複製，還一併負責輸出到畫面前所需的最終處理。

* 考慮相機 Viewport 的傳輸  
* Linear→sRGB 轉換  
* HDR Output 時的編碼與疊加 UI 的合成  
* UV 原點不同時的 Y 翻轉  
* 縮放時的濾波切換

## **具備 FinalBlit 功能的 3 種 FinalBlitPass**

負責「將中間紋理輸出到 BackBuffer」的通道，其實在 URP 中有 3 種，會依條件只選用其中 1 種。

### **1\. UberPost**

這是只要啟用後處理就會加入的通道，但在滿足以下條件時，也會兼任 FinalBlit 的功能。

* 是相機堆疊中的最後一個相機  
* 不需要後述的 FinalPost  
* 沒有在 AfterRenderingPostProcessing 之後執行的自訂通道

### **2\. FinalPost**

這是在啟用後處理、且存在 UberPost 無法處理完的「最終工序」時才會加入的通道。

1. FXAA  
2. Linear 以外的升頻 … Point / FSR / STP 等。  
3. TAA \+ CAS … 需要以獨立通道執行 RCAS 銳化

### **3\. FinalBlitPass**

這是上述兩者都沒有寫入 BackBuffer 時的最後防線。實際出現的主要是以下情況。

* 後處理停用，且存在中間紋理  
* 後處理啟用，但其後還有自訂通道  
* 後處理啟用，但存在 Camera.AddCommandBufferAsync 等擷取處理

### **整理 3 種通道的關係**

表: 負責最終輸出的通道與條件 {{table:final_blit_pass_selection}}

| 負責最終輸出的通道 | 條件 |
| :---- | :---- |
| UberPost | 後處理啟用、不需要 FinalPost、沒有後續的自訂通道與擷取 |
| FinalPost | 後處理啟用，且使用 FXAA / Linear 以外的升頻 / TAA \+ CAS 其中之一 |
| FinalBlitPass | 上述兩者都沒有寫入 BackBuffer，且存在中間紋理 |
| 都沒有 | 沒有中間紋理，或不是相機堆疊中的最後一個相機 |

# **省略 FinalBlitPass 的方法**

使用 URP 的後處理，或製作一般的自訂後處理時，都不需要在意 FinalBlitPass。  
但若加入了相當於「最終工序」的自訂通道，URP 的 FinalBlitPass 就會變成只多插入 1 次 Blit 的無用通道。  
因此從 Unity 6.3 開始，可以由自訂通道宣告「從現在起作用中的目標變為 BackBuffer」，藉此省略 URP 的 FinalBlitPass。  
`CODE START csharp | 切換到 BackBuffer {{list:switch_active_textures_to_backbuffer}}`  
`var resourceData = frameData.Get<UniversalResourceData>();`  
`resourceData.SwitchActiveTexturesToBackbuffer();`  
`CODE END`  
實作很單純（Runtime/FrameData/UniversalResourceData.cs）。  
`CODE START csharp | SwitchActiveTexturesToBackbuffer 的實作 {{list:switch_active_textures_implementation}}`  
`public void SwitchActiveTexturesToBackbuffer()`  
`{`  
    `activeColorID = UniversalResourceData.ActiveID.BackBuffer;`  
    `activeDepthID = UniversalResourceData.ActiveID.BackBuffer;`  
`}`  
`CODE END`  
它只是改寫 frameData 上的 2 個 ID，完全不會執行 Blit、複製，也不會新增 RenderPass。這個 API 並非「傳輸處理」，而是對 URP 的宣告。activeColorTexture / activeDepthTexture 只是依據這個 ID 切換所回傳的 TextureHandle 的 switch。  
`CODE START csharp | activeColorTexture 的切換 {{list:active_color_texture_switch}}`  
`public TextureHandle activeColorTexture`  
`{`  
    `get`  
    `{`  
        `switch (activeColorID)`  
        `{`  
            `case ActiveID.Camera:     return cameraColor;`  
            `case ActiveID.BackBuffer: return backBufferColor;`  
            `...`  
        `}`  
    `}`  
`}`  
`CODE END`  
執行 SwitchActiveTexturesToBackbuffer() 之後，URP 的 FinalBlitPass 就不會再被產生。

## **使用範例：自訂 FXAA**

以自訂 FXAA 後處理為例，比較前後的差異。  
過去典型的流程需要 3 次 Blit。  
`CODE START text | 傳統的 FXAA 流程 {{list:custom_fxaa_legacy_flow}}`  
`CameraColor → [FXAA] → FXAABuffer → [Blit] → CameraColor → [URP FinalBlit] → BackBuffer`  
`CODE END`  
Unity 6.0 以後可以直接替換 CameraColor，因此減少為 2 次，但仍需要名為 FXAABuffer 的中間紋理，FinalBlitPass 也仍會被建立。  
`CODE START text | Unity 6.0 的 FXAA 流程 {{list:custom_fxaa_unity6_flow}}`  
`CameraColor → [FXAA] → FXAABuffer → [URP FinalBlit] → BackBuffer`  
`CODE END`  
在 Unity 6.3 使用 BackBuffer 切換後，只需 1 次即可。也不再需要中間紋理。  
`CODE START text | Unity 6.3 的 FXAA 流程 {{list:custom_fxaa_direct_backbuffer_flow}}`  
`CameraColor → [FXAA] → BackBuffer`  
`CODE END`  
程式碼如下。此外，以下的程式碼是為了呈現重點而部分節錄的內容，無法直接執行。  
{{pagebreak}}
`CODE START csharp | 直接輸出到 BackBuffer 的自訂 FXAA {{list:custom_fxaa_direct_backbuffer}}`  
`private class PassData`  
`{`  
    `public TextureHandle Source;`  
    `public TextureHandle Target;`  
    `public Material FXAAMaterial;`  
`}`

`public override void RecordRenderGraph(RenderGraph renderGraph, ContextContainer frameData)`  
`{`  
    `var cameraData = frameData.Get<UniversalCameraData>();`  
    `var resourceData = frameData.Get<UniversalResourceData>();`

    `// 在切換前先取得 CameraColor`  
    `var cameraColor = resourceData.activeColorTexture;`

    `// 之後的 activeColorTexture 會回傳 BackBufferColor`  
    `// 如此一來 URP 的 FinalBlitPass 就不會被產生`  
    `resourceData.SwitchActiveTexturesToBackbuffer();`  
    `var backBufferColor = resourceData.activeColorTexture;`

    `// 判斷是否需要 LinearToSRGBConversion`  
    `var requireSrgbConversion = cameraData.targetTexture == null`  
                                `&& Display.main.requiresSrgbBlitToBackbuffer;`  
    `CoreUtils.SetKeyword(_fxaaMaterial, ShaderKeywordStrings.LinearToSRGBConversion, requireSrgbConversion);`

    `using (var builder = renderGraph.AddRasterRenderPass<PassData>("Custom FXAA Pass", out var passData))`  
    `{`  
        `passData.Source = cameraColor;`  
        `passData.Target = backBufferColor;`  
        `passData.FXAAMaterial = _fxaaMaterial;`  
        `builder.SetRenderAttachment(backBufferColor, 0, AccessFlags.WriteAll);`  
        `builder.UseTexture(passData.Source, AccessFlags.Read);`  
        `builder.SetRenderFunc(static (PassData data, RasterGraphContext context) =>`  
        `{`  
            `// 只有在 Source 與 Target 的 UV 原點不同時才需要 Y 翻轉`  
            `var yFlip = context.GetTextureUVOrigin(data.Source) != context.GetTextureUVOrigin(data.Target);`  
            `var scaleBias = yFlip ? new Vector4(1, -1, 0, 1) : new Vector4(1, 1, 0, 0);`  
            `Blitter.BlitTexture(context.cmd, data.Source, scaleBias, data.FXAAMaterial, 0);`  
        `});`  
    `}`  
`}`  
`CODE END`  
在 Unity 6.0 中，FinalBlitPass 總是會被建立。  
![][image1]  
在 Unity 6.3 使用 BackBuffer 切換後，只需 1 次 Blit 即可完成 FXAA 處理，並直接輸出到 BackBuffer。  
![][image2]

# **省略 URP 的 FinalBlit 時的注意事項**

SwitchActiveTexturesToBackbuffer() 只是改寫內部紋理 ID 的 API。反過來說，維持一致性的責任全在呼叫端。

## **只在相機的最後呼叫 SwitchActiveTexturesToBackbuffer()**

從呼叫這個 API 的瞬間起，之後所有通道的 activeColorTexture / activeDepthTexture 都會指向 BackBuffer。但 BackBuffer 無法讀取，因此所有讀取 activeColorTexture / activeDepthTexture 的後續處理都會失效。  
另外，cameraData.resolveFinalTarget \== false（還有後續相機）時也不可切換。Overlay 相機的設計是疊加繪製到 CameraColor 上，若中途的相機直接寫入 BackBuffer，就會與後續相機的繪製結果混在一起，或被覆蓋而消失。  
因此，請將呼叫限定在最後一個相機中、緊接在執行最終輸出的自訂通道之前。

## **在最後執行的自訂通道中實作 FinalBlit 的功能**

省略 FinalBlitPass，意味著 FinalBlitPass 所負責的最終處理也會一併消失。需要由自訂通道來承擔。

### **Y 翻轉**

當 Source 與 Target 的 UV 原點不一致時，若不翻轉，畫面就會上下顛倒。使用 Unity 6.3 新增的 GetTextureUVOrigin() 比較兩者來判斷。這個 API 只有 RasterGraphContext 等執行時的 GraphContext 才具備，因此必須在 SetRenderFunc 內呼叫，而不是在 RecordRenderGraph 的記錄階段。  
`CODE START csharp | Y 翻轉的判斷 {{list:y_flip_detection}}`  
`var yFlip = context.GetTextureUVOrigin(data.Source) != context.GetTextureUVOrigin(data.Target);`  
`var scaleBias = yFlip ? new Vector4(1, -1, 0, 1) : new Vector4(1, 1, 0, 0);`  
`CODE END`  
另外，SystemInfo.graphicsUVStartsAtTop 只表示圖形 API 的 UV 慣例，無法用來判斷「這次的 Blit 是否需要翻轉」。

### **sRGB 轉換**

需要依據 cameraData.requireSrgbConversion 設定 LinearToSRGBConversion 關鍵字，並在 Shader 端實作對應該關鍵字的轉換處理。  
`CODE START csharp | 設定 sRGB 轉換關鍵字 {{list:srgb_conversion_keyword}}`  
`var requireSrgbConversion = cameraData.targetTexture == null && Display.main.requiresSrgbBlitToBackbuffer;`  
`cmd.SetKeyword(ShaderGlobalKeywords.LinearToSRGBConversion, data.requireSrgbConversion);`  
`CODE END`  
`CODE START hlsl | sRGB 轉換 {{list:srgb_conversion_shader}}`  
`#ifdef _LINEAR_TO_SRGB_CONVERSION`  
    `color = LinearToSRGB(color);`  
`#endif`  
`CODE END`

### **Viewport 設定**

Viewport 通常設定為全螢幕，但若想設定為全螢幕以外的範圍，就需要設定 Camera 的 Viewport。  
`CODE START csharp | Viewport 的設定 {{list:viewport_setup}}`  
`var targetViewPort = new Rect(0, 0, targetWidth, targetHeight);`  
`cmd.SetViewport(targetViewPort);`  
`CODE END`

### **HDR Output 與疊加 UI 合成**

HDR 顯示器輸出時的編碼，以及 cameraData.rendersOverlayUI 為 true 時的疊加 UI 合成，原本也是 FinalBlitPass 的工作。需要支援 HDR 輸出的遊戲，必須使用 HDROutputUtils 實作同等的處理。

看起來要求很多，但只需實作專案實際會用到的功能即可。若是固定 SDR、Viewport 全螢幕、未使用疊加 UI 的遊戲，實際需要的就只有 Y 翻轉與 sRGB 轉換。

# **總結**

* URP 中負責最終輸出到 BackBuffer 的通道有 UberPost / FinalPost / FinalBlitPass 3 種，會依條件只選用其中 1 種  
* 將自訂通道注入到 AfterRenderingPostProcessing 之後時，hasPassesAfterPostProcessing 會變為 true，FinalBlitPass 幾乎必定會被加入  
* 使用 Unity 6.3 的 SwitchActiveTexturesToBackbuffer()，自訂通道就能直接輸出到 BackBuffer，省略這個 FinalBlitPass  
* 但這個 API 只是改寫相機 ActiveColor/Depth Texture 的 ID 的宣告。前提是只在最後一個相機的最終通道中呼叫，並自行實作 Y 翻轉、sRGB 轉換、Viewport 等原本由 FinalBlitPass 負責的處理

在高解析度、行動裝置環境下，1 次全螢幕 Blit 是不可忽視的成本。對於擁有自訂後處理的專案而言，相當值得考慮。

# **參考資料**

* [Unity Manual: Get data from the current frame in URP](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/accessing-frame-data.html)  
* [Class UniversalResourceData | Universal Render Pipeline | 17.3](https://docs.unity3d.com/Packages/com.unity.render-pipelines.universal@17.3/api/UnityEngine.Rendering.Universal.UniversalResourceData.html)  
* [Struct RasterGraphContext | Scriptable Render Pipeline Core | 17.3](https://docs.unity3d.com/Packages/com.unity.render-pipelines.core@17.3/api/UnityEngine.Rendering.RenderGraphModule.RasterGraphContext.html)

[image1]: assets/zhang-yubing-2/media/image1.png

[image2]: assets/zhang-yubing-2/media/image2.png