# **RenderGraph의 UnsafePass를 사용해야 하는 상황**

이름：ZHANG YUBING  
소속：주식회사 사이버에이전트 SGE 코어 기술본부

# **들어가며**

Unity 6에서는 Render Graph로 렌더링 파이프라인을 구축할 때 처리 내용에 따라 RasterRenderPass, ComputePass, UnsafePass를 구분해서 사용합니다. 이 장에서는 UnsafePass를 사용해야 하는 상황을 소개합니다.

Render Graph에는 의존 관계 분석이나 SubPass 병합 같은 최적화가 있습니다. 따라서 일반적인 렌더링에는 RasterRenderPass를, Compute Shader를 사용하는 처리에는 ComputePass를 우선 사용하는 것이 기본입니다.

UnsafePass를 선택해야 하는 상황은 주로 다음 4가지입니다.

* RasterRenderPass나 ComputePass로는 구현할 수 없는 처리가 있는 경우  
* CommandBuffer 자체가 필요한 경우  
* Compatibility Mode에서 Render Graph로 빠르게 이전하고 싶은 경우  
* 하나의 렌더링 처리가 병합할 수 없는 Pass 여러 개로 구성되어 있는 경우

처음 3가지는 UnsafePass를 사용하지 않으면 구현할 수 없거나 구현 비용이 큰 경우입니다. 마지막 1가지는 필수는 아니지만, Pass를 하나의 UnsafePass로 묶어 CPU 부하를 줄이고 Render Graph Viewer의 가시성을 개선할 수 있는 경우입니다.

이어서 이러한 상황을 차례로 설명합니다.

# **UnsafePass를 선택해야 하는 상황**

UnsafePass는 C\#의 unsafe 기능을 의미하지 않습니다. Render Graph의 일부 자동 관리와 검증을 억제하고, CommandBuffer에 가까운 API를 사용할 수 있게 한 Pass입니다.

## **RasterRenderPass나 ComputePass로는 구현할 수 없는 처리가 있는 경우**

RasterRenderPass와 ComputePass에서 사용하는 RasterCommandBuffer/ComputeCommandBuffer는 CommandBuffer 중에서 각 Pass의 역할에 필요한 API만 공개한 래퍼이며, 다음과 같은 API는 사용할 수 없습니다.

* SetRenderTarget: Load/Store Action을 지정하면서 렌더 타깃, MRT, 밉, 배열 슬라이스를 세밀하게 지정합니다.  
* CopyTexture: 밉, 배열 요소, 사각형 영역을 지정하여 텍스처를 복사합니다.  
* GenerateMips: 렌더 타깃의 밉맵을 생성합니다.  
* SetRandomWriteTarget/ClearRandomWriteTargets: 픽셀 셰이더의 랜덤 쓰기 타깃을 설정하거나 해제합니다.  
* RequestAsyncReadback: GPU 리소스의 비동기 리드백 요청을 기록합니다.

이러한 API는 UnsafePass의 UnsafeCommandBuffer에서 사용할 수 있습니다. 또한 CommandBufferHelpers.GetNativeCommandBuffer로 네이티브 CommandBuffer를 가져오면 더 고급 API도 사용할 수 있습니다.

## **CommandBuffer 자체가 필요한 경우**

엔진 콜백, IMGUI, SDK의 렌더링 처리처럼 외부 코드가 CommandBuffer를 받아 임의의 커맨드를 기록하는 처리는 기록되는 내용을 실행 시점까지 알 수 없습니다. 기록 시점에 처리 내용을 선언하는 RasterRenderPass나 ComputePass에는 이러한 블랙박스 처리를 넣을 수 없습니다. UnsafePass에서 네이티브 CommandBuffer를 가져와 외부 코드에 전달하면 Render Graph에 포함시킬 수 있습니다.

## **Compatibility Mode에서 Render Graph로 빠르게 이전하고 싶은 경우**

Compatibility Mode에서 Execute 메서드에 작성했던 CommandBuffer 처리는 커맨드 열을 거의 그대로 UnsafePass의 SetRenderFunc 안으로 옮길 수 있습니다. 따라서 적은 공수로 Render Graph 이전과 동작 확인을 진행하고 싶을 때 유효합니다.

다만 이는 어디까지나 빠르게 이전하기 위한 임시 수단입니다. 시간을 확보할 수 있다면 처리 내용에 따라 RasterRenderPass나 ComputePass로 분류하여 이식하고, UnsafePass는 선언적 API로 표현할 수 없는 부분으로만 한정하시기 바랍니다.

## **병합할 수 없는 Pass가 많이 이어지는 경우**

연속해서 병합할 수 없는 Pass를 하나의 UnsafePass로 묶으면 CPU 부하를 줄이고 Render Graph Viewer의 가시성을 개선하는 효과가 있습니다.

### **대표적인 사례**

* Ping-pong 방식의 Blur  
* Bloom의 Mip Pyramid 생성  
* 여러 번의 다운샘플과 업샘플  
* 이전 처리의 출력을 다음 처리에서 샘플링하는 필터 처리

이러한 처리에서는 Pass마다 렌더링 타깃을 전환하거나 직전 Pass의 출력을 주변 샘플링하므로 SubPass 병합이 성립하지 않습니다.

### **UnsafePass로 묶으면 CPU 부하가 줄어든다**

RasterRenderPass를 하나 등록할 때마다 다음 처리가 발생합니다.

* Pass 등록과 PassData 준비  
* UseTexture나 SetRenderAttachment를 통한 리소스 의존 관계 등록  
* Pass의 의존 관계 분석과 병합 후보 확인  
* Profiling 스코프의 시작과 종료  
* 렌더 타깃 설정과 실행 콜백 호출

RasterRenderPass가 늘어날수록 CPU 측 비용도 증가합니다. 병합할 수 없는 Pass 체인을 RasterRenderPass로 등록하면 병합의 이점은 얻지 못한 채 등록과 관리 비용만 쌓입니다. 반면 이를 하나의 UnsafePass로 묶으면 Render Graph에서는 하나의 Pass로 등록됩니다. 드로우 커맨드와 렌더 타깃 전환 비용은 변하지 않지만, Render Graph가 Pass 단위로 수행하는 등록/관리 비용을 절감할 수 있습니다({{fig:unsafe_pass_pass_chain}}).

![][image1]  
図: 병합할 수 없는 Pass 체인에서 Render Graph의 Pass 수와 렌더링 횟수 {{fig:unsafe_pass_pass_chain}}

### **CPU 부하 경감 효과의 측정 예**

이 프로젝트에서 실시한 에디터 상의 측정에서는 256×256 Render Texture에 대한 4-tap Ping-pong Blur를 대상으로, Blur 처리마다 RasterRenderPass를 등록하는 방식과 하나의 UnsafePass 안에서 일련의 Blur 처리를 실행하는 방식을 비교했습니다. 측정 조건은 Apple M4 Max(macOS 26.5), Unity 6000.3.19f1, URP 17.3입니다. 각 조합에서 500프레임을 수집하고, 전환 후 60프레임을 제외한 중앙값을 사용했습니다. 측정값은 Render Graph의 기록, 해시 계산, 컴파일, 실행에 관한 CPU marker의 합계입니다.

表: Ping-pong Blur에서 Render Graph 관련 CPU 시간 비교 {{table:unsafe_pass_pingpong_cpu}}

| Blur 횟수 | RasterRenderPass×N(µs) | UnsafePass×1(µs) | 차이(µs) |
| :---- | :---- | :---- | :---- |
| 5 | 286.4 | 280.0 | 6.4 |
| 10 | 334.8 | 313.6 | 21.2 |
| 20 | 427.8 | 389.1 | 38.7 |
| 50 | 675.9 | 592.9 | 83.0 |
| 100 | 1080.7 | 898.5 | 182.2 |

측정값은 에디터 환경에서 경향을 확인하기 위한 값입니다. Blur 횟수가 늘어날수록 RasterRenderPass 방식과 UnsafePass 방식의 차이가 커졌습니다. 선형 근사에서 두 방식의 기울기 차이, 즉 RasterRenderPass의 추가 고정 비용은 Pass당 약 1.8µs였습니다. 이 값은 에디터에서의 측정값이며 실기기에서의 절댓값을 나타내는 것은 아니지만, Blur 횟수에 따라 CPU 부하의 차이가 커지는 것을 확인할 수 있습니다.  
이 효과는 Render Graph의 기록, 실행, Profiling 스코프, 렌더 타깃 설정 등 Pass 단위의 등록/관리 비용이 절감되기 때문입니다. 드로우 커맨드에는 차이가 없으므로 UnsafePass로 바꾼다고 해서 렌더링 부하가 줄어드는 것은 아닙니다.

### **Render Graph Viewer의 가시성도 개선된다**

RasterRenderPass를 많이 등록하면 Render Graph Viewer에는 등록한 수만큼 Pass가 표시됩니다. 그래프가 가로로 길어져 기능별 구분이 잘 보이지 않고 리소스의 의존 관계도 파악하기 어려워집니다({{fig:raster_pass_viewer}}).

![][image2]  
図: Blur 처리마다 RasterRenderPass를 등록한 경우의 Render Graph Viewer 표시 {{fig:raster_pass_viewer}}

기능 내부의 Pass 체인을 하나의 UnsafePass로 묶으면 Viewer에서는 기능 전체를 나타내는 하나의 Pass로 확인할 수 있습니다({{fig:unsafe_pass_viewer}}). 이에 따라 기능별 구분이 명확해지고 리소스의 의존 관계도 파악하기 쉬워집니다. ![][image3]

図: 일련의 Blur 처리를 하나의 UnsafePass로 묶은 경우의 Render Graph Viewer 표시 {{fig:unsafe_pass_viewer}}

CPU 부하 절감에 더해 프로파일링 시의 노이즈를 줄일 수 있다는 점도 큰 이점입니다.

다만 Render Graph Viewer의 가시성을 개선하기 위한 목적만으로, 서로 다른 기능의 처리나 SubPass 병합이 성립하는 처리를 무리하게 UnsafePass로 묶는 것은 권장하지 않습니다.

# **UnsafePass 사용 시 주의 사항**

## **렌더 타깃 설정**

RasterRenderPass에서는 기록 시점에 SetRenderAttachment로 렌더 타깃을 선언하지만, UnsafePass에서는 실행 시점에 SetRenderTarget을 호출하여 설정합니다.

## **의존 관계를 선언한다**

UnsafePass를 사용하더라도 Render Graph에서 완전히 분리되는 것은 아닙니다. 사용하는 텍스처는 반드시 UseTexture로 읽기/쓰기 의존 관계를 선언해야 합니다. 선언이 부족하면 리소스의 라이프타임 관리와 동기화가 올바르게 이루어지지 않습니다.

RasterRenderPass에서는 렌더 타깃의 사용이 SetRenderAttachment로 선언되지만, UnsafePass에서는 SetRenderTarget 호출만으로는 의존 관계가 등록되지 않습니다. 렌더 타깃도 반드시 UseTexture로 읽기/쓰기 의존 관계를 선언해야 합니다.

## **Load/Store Action 설정**

RenderBufferLoadAction은 렌더 타깃을 활성화할 때 기존 내용을 어떻게 다룰지를, RenderBufferStoreAction은 렌더링 완료 후 결과를 어떻게 다룰지를 지정합니다. Tile-Based GPU에서는 불필요한 Load나 Store가 타일 메모리와 메인 메모리 간 전송을 늘리므로 적절한 설정이 중요합니다.

RasterRenderPass에서는 선언한 AccessFlags를 바탕으로 Render Graph가 Load/Store Action을 자동으로 결정합니다. 반면 UnsafePass에서 SetRenderTarget(target)처럼 Action을 생략하면 기본 동작인 Load/Store가 적용되어, 불필요한 메모리 전송으로 퍼포먼스가 저하될 수 있습니다.

따라서 UnsafePass에서는 SetRenderTarget을 호출할 때 반드시 Load/Store Action을 명시합니다.

* Load 측: 전체 화면을 덮어쓴다면 DontCare, 기존 내용을 읽는 경우에만 Load  
* Store 측: 결과를 후속 처리에서 읽는다면 Store, 어디에서도 사용하지 않는 경우에만 DontCare

# **코드 예: Ping-pong Blur의 병합할 수 없는 Pass 체인**

이 절에서는 실제 코드 예를 소개합니다. 게재한 코드는 요점을 보여 주기 위한 발췌이며, 그대로 동작하는 완전한 코드가 아닙니다.

두 장의 텍스처를 번갈아 읽고 쓰면서 Blur를 연속해서 적용하는 예입니다. 같은 처리를 RasterRenderPass로 구현한 경우를 보이고, 이어서 하나의 UnsafePass로 묶은 경우를 보입니다. desc는 Blur용 Render Texture 기술자이고, blurMaterial은 Blur용 머티리얼입니다.

## **RasterRenderPass 방식: Blur 처리마다 Pass를 등록한다**

RasterRenderPass 방식에서는 {{list:raster_pass_pingpong_blur}}처럼 루프 안에서 Blur 처리마다 AddRasterRenderPass를 호출합니다. 각 Pass의 입력과 출력은 명확하게 선언할 수 있지만, Blur 횟수가 늘어날수록 Render Graph의 Pass 수도 증가합니다.

`CODE START csharp | RasterRenderPass로 Ping-pong Blur를 구현한다 {{list:raster_pass_pingpong_blur}}`  
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

## **UnsafePass 방식: 하나의 Pass 안에서 일련의 처리를 실행한다**

UnsafePass 방식에서는 {{list:unsafe_pass_pingpong_blur}}처럼 Render Graph에 대한 등록을 한 번으로 하고 SetRenderFunc 안에서 렌더 타깃을 전환합니다. Blur 처리마다의 드로우 커맨드는 그대로 남지만 Pass 단위의 관리를 한 번으로 묶을 수 있습니다.

`CODE START csharp | Ping-pong Blur를 하나의 UnsafePass로 합친다 {{list:unsafe_pass_pingpong_blur}}`  
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

# **정리**

이 장에서는 Render Graph에서 UnsafePass를 사용해야 하는 상황과 사용 시의 주의 사항을 소개했습니다.

UnsafePass를 선택해야 하는 상황은 다음 4가지입니다.

* RasterCommandBuffer/ComputeCommandBuffer에 공개되지 않은 API가 필요한 경우  
* 외부 코드에 전달할 CommandBuffer 자체가 필요한 경우  
* Compatibility Mode에서 Render Graph로 빠르게 이전하고 싶은 경우  
* 병합할 수 없는 Pass 체인을 하나의 Pass로 묶고 싶은 경우

병합할 수 없는 Pass 체인을 하나의 UnsafePass로 묶으면 Pass 단위의 등록/관리 비용이 절감되고 Render Graph Viewer의 가시성도 개선됩니다. 이 프로젝트의 에디터 상 측정({{table:unsafe_pass_pingpong_cpu}})에서는 RasterRenderPass 하나당 약 1.8µs의 고정 비용을 절감할 수 있었습니다. 다만 드로우 커맨드 자체는 변하지 않으므로 GPU 부하 절감을 목적으로 하는 기법이 아니라는 점에 주의해야 합니다.

한편 UnsafePass는 Render Graph의 자동 관리와 검증 일부를 포기하는 Pass이기도 합니다. 사용할 때는 UseTexture를 통한 의존 관계 선언과 SetRenderTarget에서의 Load/Store Action 명시를 잊지 않도록 하시기 바랍니다.

기본적으로 RasterRenderPass와 ComputePass를 우선 사용하고, UnsafePass는 선언적 API로 표현할 수 없는 처리와 병합할 수 없는 Pass 체인의 정리로 한정하여 사용함으로써 Render Graph의 최적화와 CommandBuffer의 자유도를 모두 확보할 수 있습니다.

# **참고 자료**

* [Unity Manual \- Use the CommandBuffer interface in a render graph (UnsafePass)](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/render-graph-unsafe-pass.html)  
* [Unity Manual \- Optimize a render graph](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/render-graph-optimize.html)  
* [Unity Scripting API \- CommandBuffer](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.CommandBuffer.html)  
* [Unity Scripting API \- RenderBufferLoadAction](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.RenderBufferLoadAction.html)  
* [Unity Scripting API \- RenderBufferStoreAction](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.RenderBufferStoreAction.html)

[image1]: assets/zhang-yubing-1/media/image1.png

[image2]: assets/zhang-yubing-1/media/image2.png

[image3]: assets/zhang-yubing-1/media/image3.png