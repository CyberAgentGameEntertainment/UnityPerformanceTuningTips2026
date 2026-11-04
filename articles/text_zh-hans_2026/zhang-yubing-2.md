# **省略 URP 的 FinalBlitPass 的方法**

姓名：ZHANG YUBING  
所属：株式会社 CyberAgent SGE 核心技术本部

# **什么是 FinalBlitPass**

在 URP 的渲染管线中，实际显示到屏幕上的是 BackBuffer。  
但 BackBuffer 是只写的，无法读取。因此，当存在后处理这类需要将中间结果作为输入读取的处理时，相机会先绘制到中间纹理，最后再 Blit 到 BackBuffer。  
负责将中间纹理 Blit 到 BackBuffer 的，就是 URP 的 FinalBlitPass。  
它并不只是单纯的复制，还一并负责输出到屏幕前所需的最终处理。

* 考虑相机 Viewport 的传输  
* Linear→sRGB 转换  
* HDR Output 时的编码与叠加 UI 的合成  
* UV 原点不同时的 Y 翻转  
* 缩放时的过滤器切换

## **具备 FinalBlit 功能的 3 种 FinalBlitPass**

承担“将中间纹理输出到 BackBuffer”职责的通道，在 URP 中其实有 3 种，并根据条件只选用其中 1 种。

### **1\. UberPost**

这是后处理启用时会添加的通道，但在满足以下条件时，也会兼任 FinalBlit 的功能。

* 是相机堆栈中的最后一个相机  
* 不需要后述的 FinalPost  
* 没有在 AfterRenderingPostProcessing 之后执行的自定义通道

### **2\. FinalPost**

这是在后处理启用、且存在 UberPost 无法处理完的“最终工序”时添加的通道。

1. FXAA  
2. Linear 以外的上采样 … Point / FSR / STP 等。  
3. TAA \+ CAS … 需要在独立通道中执行 RCAS 锐化

### **3\. FinalBlitPass**

这是在上述两者都没有写入 BackBuffer 时的最后一道防线。实际出现的主要是以下几种情况。

* 后处理禁用，且存在中间纹理  
* 后处理启用，但其后还有自定义通道  
* 后处理启用，但存在 Camera.AddCommandBufferAsync 等捕获处理

### **梳理 3 种通道的关系**

表: 负责最终输出的通道及其条件 {{table:final_blit_pass_selection}}

| 负责最终输出的通道 | 条件 |
| :---- | :---- |
| UberPost | 后处理启用、不需要 FinalPost、后续既无自定义通道也无捕获 |
| FinalPost | 后处理启用，且使用 FXAA / Linear 以外的上采样 / TAA \+ CAS 之一 |
| FinalBlitPass | 上述两者都没有写入 BackBuffer，且存在中间纹理 |
| 均无 | 不存在中间纹理，或不是相机堆栈中的最后一个相机 |

# **省略 FinalBlitPass 的方法**

使用 URP 的后处理时，或编写普通的自定义后处理时，都无需在意 FinalBlitPass。  
但如果加入了相当于“最终工序”的自定义通道，URP 的 FinalBlitPass 就会沦为只是多插入 1 次 Blit 的无用通道。  
因此，从 Unity 6.3 开始，可以由自定义通道一方声明“从现在起活动目标变为 BackBuffer”，从而省略 URP 的 FinalBlitPass。  
`CODE START csharp | 切换到 BackBuffer {{list:switch_active_textures_to_backbuffer}}`  
`var resourceData = frameData.Get<UniversalResourceData>();`  
`resourceData.SwitchActiveTexturesToBackbuffer();`  
`CODE END`  
实现很简单（Runtime/FrameData/UniversalResourceData.cs）。  
`CODE START csharp | SwitchActiveTexturesToBackbuffer 的实现 {{list:switch_active_textures_implementation}}`  
`public void SwitchActiveTexturesToBackbuffer()`  
`{`  
    `activeColorID = UniversalResourceData.ActiveID.BackBuffer;`  
    `activeDepthID = UniversalResourceData.ActiveID.BackBuffer;`  
`}`  
`CODE END`  
它只是改写 frameData 上的 2 个 ID，完全不会执行 Blit、复制或添加 RenderPass。这个 API 不是“传输处理”，而是对 URP 的声明。activeColorTexture / activeDepthTexture 只是一个根据该 ID 切换所返回 TextureHandle 的 switch。  
`CODE START csharp | activeColorTexture 的切换 {{list:active_color_texture_switch}}`  
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
执行 SwitchActiveTexturesToBackbuffer() 之后，URP 的 FinalBlitPass 将不再生成。

## **使用示例：自定义 FXAA**

以自定义 FXAA 后处理为例，比较前后差异。  
以往的典型流程需要 3 次 Blit。  
`CODE START text | 以往的 FXAA 流程 {{list:custom_fxaa_legacy_flow}}`  
`CameraColor → [FXAA] → FXAABuffer → [Blit] → CameraColor → [URP FinalBlit] → BackBuffer`  
`CODE END`  
Unity 6.0 以后可以直接替换 CameraColor，因此减少到 2 次，但仍需要名为 FXAABuffer 的中间纹理，FinalBlitPass 也依然会被创建。  
`CODE START text | Unity 6.0 的 FXAA 流程 {{list:custom_fxaa_unity6_flow}}`  
`CameraColor → [FXAA] → FXAABuffer → [URP FinalBlit] → BackBuffer`  
`CODE END`  
在 Unity 6.3 中使用 BackBuffer 切换后，只需 1 次即可，也不再需要中间纹理。  
`CODE START text | Unity 6.3 的 FXAA 流程 {{list:custom_fxaa_direct_backbuffer_flow}}`  
`CameraColor → [FXAA] → BackBuffer`  
`CODE END`  
代码如下。另外，以下代码为了展示要点只摘录了一部分，无法直接运行。  
{{pagebreak}}
`CODE START csharp | 直接输出到 BackBuffer 的自定义 FXAA {{list:custom_fxaa_direct_backbuffer}}`  
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

    `// 在切换前先获取 CameraColor`  
    `var cameraColor = resourceData.activeColorTexture;`

    `// 此后 activeColorTexture 将返回 BackBufferColor`  
    `// 这样 URP 的 FinalBlitPass 就不会再生成`  
    `resourceData.SwitchActiveTexturesToBackbuffer();`  
    `var backBufferColor = resourceData.activeColorTexture;`

    `// 判断是否需要 LinearToSRGBConversion`  
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
            `// 仅当 Source 与 Target 的 UV 原点不同时才需要 Y 翻转`  
            `var yFlip = context.GetTextureUVOrigin(data.Source) != context.GetTextureUVOrigin(data.Target);`  
            `var scaleBias = yFlip ? new Vector4(1, -1, 0, 1) : new Vector4(1, 1, 0, 0);`  
            `Blitter.BlitTexture(context.cmd, data.Source, scaleBias, data.FXAAMaterial, 0);`  
        `});`  
    `}`  
`}`  
`CODE END`  
在 Unity 6.0 中，FinalBlitPass 总是会被创建。  
![][image1]  
在 Unity 6.3 中使用 BackBuffer 切换后，1 次 Blit 即可完成 FXAA 处理并直接输出到 BackBuffer。  
![][image2]

# **省略 URP 的 FinalBlit 时的注意事项**

SwitchActiveTexturesToBackbuffer() 只是一个改写内部纹理 ID 的 API。反过来说，保持一致性的责任全部在调用方。

## **只在相机的最后调用 SwitchActiveTexturesToBackbuffer()**

从调用这个 API 的那一刻起，之后所有通道的 activeColorTexture / activeDepthTexture 都指向 BackBuffer。但由于 BackBuffer 无法读取，所有读取 activeColorTexture / activeDepthTexture 的后续处理都会失效。  
此外，cameraData.resolveFinalTarget \== false（还有后续相机）时也不能切换。Overlay 相机的设计是在 CameraColor 上叠加绘制，如果中途的相机直接写入 BackBuffer，就会与后续相机的绘制结果混在一起，或被覆盖而消失。  
因此，请将调用限定在最后一个相机中、紧接在执行最终输出的自定义通道之前。

## **在最后执行的自定义通道中实现 FinalBlit 的功能**

省略 FinalBlitPass，意味着 FinalBlitPass 原本承担的最终处理也会一并消失。需要由自定义通道一方来代为承担。

### **Y 翻转**

当 Source 与 Target 的 UV 原点不一致时，如果不翻转，画面就会上下颠倒。可使用 Unity 6.3 新增的 GetTextureUVOrigin() 比较两者来判断。由于这个 API 只有 RasterGraphContext 等执行时的 GraphContext 才具备，因此必须在 SetRenderFunc 内调用，而不是在 RecordRenderGraph 的记录阶段。  
`CODE START csharp | Y 翻转的判断 {{list:y_flip_detection}}`  
`var yFlip = context.GetTextureUVOrigin(data.Source) != context.GetTextureUVOrigin(data.Target);`  
`var scaleBias = yFlip ? new Vector4(1, -1, 0, 1) : new Vector4(1, 1, 0, 0);`  
`CODE END`  
另外，SystemInfo.graphicsUVStartsAtTop 只表示图形 API 的 UV 约定，不能用于判断“本次 Blit 是否需要翻转”。

### **sRGB 转换**

需要根据 cameraData.requireSrgbConversion 设置 LinearToSRGBConversion 关键字，并在 Shader 一侧实现与该关键字对应的转换处理。  
`CODE START csharp | sRGB 转换关键字的设置 {{list:srgb_conversion_keyword}}`  
`var requireSrgbConversion = cameraData.targetTexture == null && Display.main.requiresSrgbBlitToBackbuffer;`  
`cmd.SetKeyword(ShaderGlobalKeywords.LinearToSRGBConversion, data.requireSrgbConversion);`  
`CODE END`  
`CODE START hlsl | sRGB 转换 {{list:srgb_conversion_shader}}`  
`#ifdef _LINEAR_TO_SRGB_CONVERSION`  
    `color = LinearToSRGB(color);`  
`#endif`  
`CODE END`

### **Viewport 设置**

Viewport 通常设置为全屏，但如果想设置为全屏以外的范围，就需要设置 Camera 的 Viewport。  
`CODE START csharp | Viewport 的设置 {{list:viewport_setup}}`  
`var targetViewPort = new Rect(0, 0, targetWidth, targetHeight);`  
`cmd.SetViewport(targetViewPort);`  
`CODE END`

### **HDR Output 与叠加 UI 合成**

HDR 显示器输出时的编码，以及 cameraData.rendersOverlayUI 为 true 时的叠加 UI 合成，原本也是 FinalBlitPass 的工作。需要支持 HDR 输出的游戏，必须使用 HDROutputUtils 实现同等的处理。

要求看起来很多，但只需实现项目中实际用到的功能即可。对于固定 SDR、Viewport 全屏、不使用叠加 UI 的游戏，实际需要的只有 Y 翻转和 sRGB 转换。

# **总结**

* 在 URP 中，负责向 BackBuffer 进行最终输出的通道有 UberPost / FinalPost / FinalBlitPass 3 种，并根据条件只选用其中 1 种  
* 将自定义通道注入到 AfterRenderingPostProcessing 之后时，hasPassesAfterPostProcessing 会变为 true，FinalBlitPass 几乎必定会被添加  
* 使用 Unity 6.3 的 SwitchActiveTexturesToBackbuffer()，自定义通道可以直接输出到 BackBuffer，从而省略这个 FinalBlitPass  
* 但这个 API 只是改写相机 ActiveColor/Depth Texture 的 ID 的声明。前提是只在最后一个相机的最终通道中调用，并自行实现 Y 翻转、sRGB 转换、Viewport 等原本由 FinalBlitPass 承担的处理

1 次全屏 Blit 在高分辨率、移动端环境下是不可忽视的开销。对于拥有自定义后处理的项目，很值得考虑。

# **参考资料**

* [Unity Manual: Get data from the current frame in URP](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/accessing-frame-data.html)  
* [Class UniversalResourceData | Universal Render Pipeline | 17.3](https://docs.unity3d.com/Packages/com.unity.render-pipelines.universal@17.3/api/UnityEngine.Rendering.Universal.UniversalResourceData.html)  
* [Struct RasterGraphContext | Scriptable Render Pipeline Core | 17.3](https://docs.unity3d.com/Packages/com.unity.render-pipelines.core@17.3/api/UnityEngine.Rendering.RenderGraphModule.RasterGraphContext.html)

[image1]: assets/zhang-yubing-2/media/image1.png

[image2]: assets/zhang-yubing-2/media/image2.png