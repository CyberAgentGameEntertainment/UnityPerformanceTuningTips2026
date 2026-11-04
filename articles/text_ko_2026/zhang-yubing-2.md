# **URP의 FinalBlitPass를 생략하는 방법**

이름：ZHANG YUBING  
소속：주식회사 사이버에이전트 SGE 코어 기술본부

# **FinalBlitPass란**

URP의 렌더링 파이프라인에서 실제로 화면에 표시되는 것은 BackBuffer입니다.  
그러나 BackBuffer는 쓰기 전용이라 읽을 수 없습니다. 따라서 포스트 프로세스처럼 중간 결과를 입력으로 읽는 처리가 있으면, 카메라는 먼저 중간 텍스처에 렌더링하고 마지막에 BackBuffer로 Blit하는 구조가 됩니다.  
이 중간 텍스처에서 BackBuffer로의 Blit을 담당하는 것이 URP의 FinalBlitPass입니다.  
단순한 복사가 아니라, 화면에 출력하기 직전에 필요한 최종 처리도 함께 담당합니다.

* 카메라의 Viewport를 고려한 전송  
* Linear→sRGB 변환  
* HDR Output 시의 인코딩과 오버레이 UI 합성  
* UV 원점이 다른 경우의 Y 반전  
* 스케일링 시의 필터 전환

## **FinalBlit 기능을 갖는 3종류의 FinalBlitPass**

'중간 텍스처를 BackBuffer로 출력한다'는 역할을 가진 패스는 URP에 세 종류가 있으며, 조건에 따라 그중 하나만 선택됩니다.

### **1\. UberPost**

포스트 프로세스가 활성화되어 있으면 추가되는 패스이며, 다음 조건을 만족하면 FinalBlit 기능도 겸합니다.

* 카메라 스택의 마지막 카메라일 것  
* 후술하는 FinalPost가 필요하지 않을 것  
* AfterRenderingPostProcessing 이후에 실행되는 커스텀 패스가 없을 것

### **2\. FinalPost**

포스트 프로세스가 활성화되어 있고, UberPost에서 처리할 수 없는 '최종 공정'이 있을 때 추가되는 패스입니다.

1. FXAA  
2. Linear 이외의 업스케일 … Point / FSR / STP 등.  
3. TAA \+ CAS … RCAS 샤프닝을 독립 패스로 실행해야 함

### **3\. FinalBlitPass**

위 두 패스 모두 BackBuffer에 쓰지 않은 경우의 마지막 보루입니다. 실제로 나타나는 경우는 주로 다음과 같습니다.

* 포스트 프로세스가 비활성화되어 있고 중간 텍스처가 존재함  
* 포스트 프로세스는 활성화되어 있지만 그 뒤에 커스텀 패스가 있음  
* 포스트 프로세스는 활성화되어 있지만 Camera.AddCommandBufferAsync 등의 캡처 처리가 있음

### **3종류의 관계 정리**

表: 최종 출력을 담당하는 패스와 조건 {{table:final_blit_pass_selection}}

| 최종 출력을 담당하는 패스 | 조건 |
| :---- | :---- |
| UberPost | 포스트 프로세스 활성, FinalPost 불필요, 후속 커스텀 패스도 캡처도 없음 |
| FinalPost | 포스트 프로세스 활성, 그리고 FXAA / Linear 이외의 업스케일 / TAA \+ CAS 중 하나 |
| FinalBlitPass | 위 두 패스가 BackBuffer에 쓰지 않았고 중간 텍스처가 있는 경우 |
| 모두 없음 | 중간 텍스처가 없거나, 카메라 스택의 마지막 카메라가 아님 |

# **FinalBlitPass를 생략하는 방법**

URP의 포스트 프로세스를 사용할 때나 일반적인 커스텀 포스트 프로세스를 만들 때는 FinalBlitPass를 신경 쓸 필요가 없습니다.  
다만 '최종 공정'에 해당하는 커스텀 패스를 넣은 경우, URP의 FinalBlitPass는 불필요한 Blit을 한 번 더 끼워 넣을 뿐인 낭비 패스가 됩니다.  
그래서 Unity 6.3부터는 커스텀 패스 측에서 '지금부터 액티브 타깃이 BackBuffer가 된다'고 선언하여 URP의 FinalBlitPass를 생략할 수 있게 되었습니다.  
`CODE START csharp | BackBuffer로 전환 {{list:switch_active_textures_to_backbuffer}}`  
`var resourceData = frameData.Get<UniversalResourceData>();`  
`resourceData.SwitchActiveTexturesToBackbuffer();`  
`CODE END`  
구현은 단순합니다(Runtime/FrameData/UniversalResourceData.cs).  
`CODE START csharp | SwitchActiveTexturesToBackbuffer 구현 {{list:switch_active_textures_implementation}}`  
`public void SwitchActiveTexturesToBackbuffer()`  
`{`  
    `activeColorID = UniversalResourceData.ActiveID.BackBuffer;`  
    `activeDepthID = UniversalResourceData.ActiveID.BackBuffer;`  
`}`  
`CODE END`  
frameData의 ID 두 개를 덮어쓸 뿐이며, Blit도 복사도 RenderPass 추가도 전혀 수행하지 않습니다. 이 API는 '전송 처리'가 아니라 URP에 대한 선언입니다. activeColorTexture / activeDepthTexture는 이 ID를 보고 반환하는 TextureHandle을 전환하는 switch일 뿐입니다.  
`CODE START csharp | activeColorTexture 전환 {{list:active_color_texture_switch}}`  
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
SwitchActiveTexturesToBackbuffer()를 실행한 후에는 URP의 FinalBlitPass가 생성되지 않습니다.

## **사용 예: 커스텀 FXAA**

커스텀 FXAA 포스트 프로세스를 예로 전후를 비교합니다.  
과거의 전형적인 흐름은 Blit이 3회 필요했습니다.  
`CODE START text | 기존 FXAA 플로우 {{list:custom_fxaa_legacy_flow}}`  
`CameraColor → [FXAA] → FXAABuffer → [Blit] → CameraColor → [URP FinalBlit] → BackBuffer`  
`CODE END`  
Unity 6.0 이후에는 CameraColor를 직접 교체할 수 있어 2회로 줄지만, FXAABuffer라는 중간 텍스처는 필요하며 FinalBlitPass도 계속 생성됩니다.  
`CODE START text | Unity 6.0의 FXAA 플로우 {{list:custom_fxaa_unity6_flow}}`  
`CameraColor → [FXAA] → FXAABuffer → [URP FinalBlit] → BackBuffer`  
`CODE END`  
Unity 6.3에서 BackBuffer 전환을 사용하면 1회로 끝납니다. 중간 텍스처도 필요하지 않습니다.  
`CODE START text | Unity 6.3의 FXAA 플로우 {{list:custom_fxaa_direct_backbuffer_flow}}`  
`CameraColor → [FXAA] → BackBuffer`  
`CODE END`  
코드는 다음과 같습니다. 이후의 코드는 요점을 보여 주기 위해 일부를 발췌한 것이며, 그대로는 동작하지 않습니다.  
{{pagebreak}}
`CODE START csharp | BackBuffer로 직접 출력하는 커스텀 FXAA {{list:custom_fxaa_direct_backbuffer}}`  
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

    `// 전환하기 전에 CameraColor를 가져옵니다`  
    `var cameraColor = resourceData.activeColorTexture;`

    `// 이후의 activeColorTexture는 BackBufferColor를 반환합니다`  
    `// 이렇게 하면 URP의 FinalBlitPass가 생성되지 않습니다`  
    `resourceData.SwitchActiveTexturesToBackbuffer();`  
    `var backBufferColor = resourceData.activeColorTexture;`

    `// LinearToSRGBConversion이 필요한지 판정`  
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
            `// Source와 Target의 UV 원점이 다른 경우에만 Y 반전이 필요합니다`  
            `var yFlip = context.GetTextureUVOrigin(data.Source) != context.GetTextureUVOrigin(data.Target);`  
            `var scaleBias = yFlip ? new Vector4(1, -1, 0, 1) : new Vector4(1, 1, 0, 0);`  
            `Blitter.BlitTexture(context.cmd, data.Source, scaleBias, data.FXAAMaterial, 0);`  
        `});`  
    `}`  
`}`  
`CODE END`  
Unity 6.0에서는 FinalBlitPass가 항상 생성되었습니다.  
![][image1]  
Unity 6.3에서 BackBuffer 전환을 사용하면 Blit 1회로 FXAA 처리가 완료되어 BackBuffer에 직접 출력됩니다.  
![][image2]

# **URP의 FinalBlit을 생략할 때의 주의 사항**

SwitchActiveTexturesToBackbuffer()는 내부의 텍스처 ID를 덮어쓰기만 하는 API입니다. 달리 말하면 정합성을 맞출 책임은 모두 호출하는 쪽에 있습니다.

## **카메라의 마지막에만 SwitchActiveTexturesToBackbuffer()를 호출할 것**

이 API를 호출한 순간부터 이후 모든 패스의 activeColorTexture / activeDepthTexture는 BackBuffer를 가리킵니다. 그러나 BackBuffer는 Read할 수 없으므로 activeColorTexture / activeDepthTexture를 읽는 후속 처리는 모두 실패합니다.  
또한 cameraData.resolveFinalTarget \== false(후속 카메라가 있음)인 경우에도 전환해서는 안 됩니다. Overlay 카메라는 CameraColor에 겹쳐서 렌더링하는 설계이므로, 중간 카메라가 BackBuffer에 직접 쓰면 후속 카메라의 렌더링 결과와 섞이거나 덮어쓰여 사라집니다.  
따라서 호출은 마지막 카메라에서 최종 출력을 수행하는 커스텀 패스 직전으로 한정하시기 바랍니다.

## **마지막에 실행하는 커스텀 패스에 FinalBlit 기능을 구현할 것**

FinalBlitPass를 생략한다는 것은 FinalBlitPass가 담당하던 최종 처리도 함께 사라진다는 뜻입니다. 커스텀 패스 측에서 대신 처리해야 합니다.

### **Y 반전**

Source와 Target의 UV 원점이 일치하지 않는 경우 반전하지 않으면 화면이 상하로 뒤집힙니다. Unity 6.3에서 추가된 GetTextureUVOrigin()으로 두 원점을 비교하여 판정합니다. 이 API는 RasterGraphContext 등 실행 시점의 GraphContext만 가지고 있으므로, RecordRenderGraph의 기록 시점이 아니라 SetRenderFunc 내에서 호출해야 합니다.  
`CODE START csharp | Y 반전 판정 {{list:y_flip_detection}}`  
`var yFlip = context.GetTextureUVOrigin(data.Source) != context.GetTextureUVOrigin(data.Target);`  
`var scaleBias = yFlip ? new Vector4(1, -1, 0, 1) : new Vector4(1, 1, 0, 0);`  
`CODE END`  
또한 SystemInfo.graphicsUVStartsAtTop은 그래픽스 API의 UV 규약을 나타낼 뿐이며, '이번 Blit에서 반전이 필요한가'를 판정하는 데는 사용할 수 없습니다.

### **sRGB 변환**

cameraData.requireSrgbConversion을 확인하여 LinearToSRGBConversion 키워드를 설정해야 하며, 셰이더 측에서 해당 키워드에 따른 변환 처리를 구현해야 합니다.  
`CODE START csharp | sRGB 변환 키워드 설정 {{list:srgb_conversion_keyword}}`  
`var requireSrgbConversion = cameraData.targetTexture == null && Display.main.requiresSrgbBlitToBackbuffer;`  
`cmd.SetKeyword(ShaderGlobalKeywords.LinearToSRGBConversion, data.requireSrgbConversion);`  
`CODE END`  
`CODE START hlsl | sRGB 변환 {{list:srgb_conversion_shader}}`  
`#ifdef _LINEAR_TO_SRGB_CONVERSION`  
    `color = LinearToSRGB(color);`  
`#endif`  
`CODE END`

### **Viewport 설정**

Viewport는 보통 전체 화면으로 설정되어 있지만, 전체 화면 이외로 설정하려는 경우 Camera의 Viewport를 설정해야 합니다.  
`CODE START csharp | Viewport 설정 {{list:viewport_setup}}`  
`var targetViewPort = new Rect(0, 0, targetWidth, targetHeight);`  
`cmd.SetViewport(targetViewPort);`  
`CODE END`

### **HDR Output과 오버레이 UI 합성**

HDR 디스플레이 출력 시의 인코딩이나, cameraData.rendersOverlayUI가 true일 때의 오버레이 UI 합성도 FinalBlitPass의 역할이었습니다. HDR 출력 대응이 필요한 타이틀에서는 HDROutputUtils를 사용하여 동등한 처리를 구현해야 합니다.

요구 사항이 많아 보이지만, 프로젝트에서 실제로 사용하는 기능만 구현하면 충분합니다. SDR 고정, Viewport 전체 화면, 오버레이 UI 미사용인 타이틀이라면 실질적으로 필요한 것은 Y 반전과 sRGB 변환뿐입니다.

# **정리**

* URP에서 BackBuffer로의 최종 출력을 담당하는 패스는 UberPost / FinalPost / FinalBlitPass의 3종류이며, 조건에 따라 그중 하나만 선택됩니다  
* 커스텀 패스를 AfterRenderingPostProcessing 이후에 주입하면 hasPassesAfterPostProcessing이 true가 되어 FinalBlitPass가 거의 반드시 추가됩니다  
* Unity 6.3의 SwitchActiveTexturesToBackbuffer()를 사용하면 커스텀 패스가 BackBuffer에 직접 출력할 수 있으며, 이 FinalBlitPass를 생략할 수 있습니다  
* 다만 이 API는 카메라의 Active Color/Depth Texture ID를 덮어쓰는 선언에 불과합니다. 마지막 카메라의 최종 패스에서만 호출할 것, 그리고 Y 반전·sRGB 변환·Viewport 등 FinalBlitPass가 담당하던 처리를 직접 구현할 것이 전제가 됩니다

풀스크린 Blit 1회는 고해상도·모바일 환경에서는 무시할 수 없는 비용입니다. 커스텀 포스트 프로세스를 가진 프로젝트라면 검토할 가치가 충분합니다.

# **참고 자료**

* [Unity Manual: Get data from the current frame in URP](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/accessing-frame-data.html)  
* [Class UniversalResourceData | Universal Render Pipeline | 17.3](https://docs.unity3d.com/Packages/com.unity.render-pipelines.universal@17.3/api/UnityEngine.Rendering.Universal.UniversalResourceData.html)  
* [Struct RasterGraphContext | Scriptable Render Pipeline Core | 17.3](https://docs.unity3d.com/Packages/com.unity.render-pipelines.core@17.3/api/UnityEngine.Rendering.RenderGraphModule.RasterGraphContext.html)

[image1]: assets/zhang-yubing-2/media/image1.png

[image2]: assets/zhang-yubing-2/media/image2.png