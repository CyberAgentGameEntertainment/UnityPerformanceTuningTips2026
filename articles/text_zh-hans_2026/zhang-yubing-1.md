# **应当使用 RenderGraph 的 UnsafePass 的场合**

姓名：ZHANG YUBING  
所属：株式会社 CyberAgent SGE 核心技术本部

# **前言**

在 Unity 6 中，使用 Render Graph 构建渲染管线时，需要根据处理内容分别使用 RasterRenderPass、ComputePass 和 UnsafePass。本章介绍应当使用 UnsafePass 的场合。

Render Graph 具备依赖关系分析和 SubPass 合并等优化。因此，基本原则是：常规绘制优先使用 RasterRenderPass，使用 Compute Shader 的处理优先使用 ComputePass。

应当选择 UnsafePass 的场合主要有以下 4 种。

* 存在 RasterRenderPass 和 ComputePass 无法实现的处理时  
* 需要 CommandBuffer 本身时  
* 希望快速从 Compatibility Mode 迁移到 Render Graph 时  
* 一个绘制处理由多个无法合并的 Pass 组成时

前 3 种是不使用 UnsafePass 就无法实现、或实现成本很高的情况。最后 1 种并非必须，但将多个 Pass 合并为 1 个 UnsafePass，有望降低 CPU 开销并改善 Render Graph Viewer 的可读性。

下面依次讲解这些场合。

# **应当选择 UnsafePass 的场合**

UnsafePass 并不是指 C\# 的 unsafe 功能。它是一种抑制了 Render Graph 的部分自动管理和校验、从而可以使用接近 CommandBuffer 的 API 的 Pass。

## **存在 RasterRenderPass 和 ComputePass 无法实现的处理时**

RasterRenderPass 和 ComputePass 使用的 RasterCommandBuffer／ComputeCommandBuffer，是只公开了 CommandBuffer 中各 Pass 职责所需 API 的包装器，无法使用以下这类 API。

* SetRenderTarget：在指定 Load/Store Action 的同时，精细指定渲染目标、MRT、Mip 级别和数组切片  
* CopyTexture：指定 Mip 级别、数组元素和矩形区域来复制纹理  
* GenerateMips：生成渲染目标的 Mipmap  
* SetRandomWriteTarget/ClearRandomWriteTargets：设置或清除像素着色器的随机写入目标  
* RequestAsyncReadback：记录 GPU 资源的异步回读请求

这些 API 可以在 UnsafePass 的 UnsafeCommandBuffer 中使用。此外，通过 CommandBufferHelpers.GetNativeCommandBuffer 获取原生 CommandBuffer，还可以使用更高级的 API。

## **需要 CommandBuffer 本身时**

引擎回调、IMGUI、SDK 的绘制处理等由外部代码接收 CommandBuffer 并记录任意命令的处理，其记录内容直到执行时才能确定。RasterRenderPass 和 ComputePass 要求在记录时声明处理内容，因此无法纳入这类黑盒处理。通过 UnsafePass 获取原生 CommandBuffer 并传递给外部代码，即可将其纳入 Render Graph。

## **希望快速从 Compatibility Mode 迁移到 Render Graph 时**

在 Compatibility Mode 下写在 Execute 方法中的 CommandBuffer 处理，其命令序列几乎可以原样移到 UnsafePass 的 SetRenderFunc 内。因此，在希望以较少工时推进 Render Graph 迁移和运行验证时，这种做法很有效。

但这只是为了快速迁移的临时手段。在时间充裕的情况下，建议根据处理内容分类移植到 RasterRenderPass 或 ComputePass，并将 UnsafePass 仅限用于声明式 API 无法表达的部分。

## **由多个无法合并的 Pass 组成时**

将连续且无法合并的 Pass 合并为 1 个 UnsafePass，可以降低 CPU 开销并改善 Render Graph Viewer 的可读性。

### **典型案例**

* Ping-pong 方式的 Blur  
* Bloom 的 Mip Pyramid 生成  
* 多次降采样与升采样  
* 在下一步处理中采样上一步输出的滤波处理

在这些处理中，由于每个 Pass 都要切换渲染目标，或者需要对上一个 Pass 的输出进行邻域采样，SubPass 合并无法成立。

### **合并为 UnsafePass 可降低 CPU 开销**

每注册 1 个 RasterRenderPass，都会发生以下处理。

* 注册 Pass 并准备 PassData  
* 通过 UseTexture 和 SetRenderAttachment 注册资源依赖关系  
* 分析 Pass 的依赖关系并确认合并候选  
* Profiling 作用域的开始与结束  
* 设置渲染目标并调用执行回调

RasterRenderPass 越多，CPU 侧的成本也越高。将无法合并的 Pass 链注册为 RasterRenderPass，既得不到合并的收益，注册和管理的成本还会不断累积。而将它们合并为 1 个 UnsafePass 后，在 Render Graph 上只注册为 1 个 Pass。绘制命令和渲染目标切换的成本不变，但可以削减 Render Graph 以 Pass 为单位进行的注册/管理成本（{{fig:unsafe_pass_pass_chain}}）。

![][image1]  
図: 无法合并的 Pass 链在 Render Graph 上的 Pass 数与绘制次数 {{fig:unsafe_pass_pass_chain}}

### **CPU 开销削减效果的测量示例**

本项目在 Editor 上进行的测量中，针对 256×256 的 Render Texture 的 4-tap Ping-pong Blur，比较了为每次 Blur 处理注册 1 个 RasterRenderPass 的方式，与在 1 个 UnsafePass 内执行整个 Blur 处理序列的方式。测量条件为 Apple M4 Max（macOS 26.5）、Unity 6000.3.19f1、URP 17.3。每种组合采集 500 帧，排除切换后的 60 帧，取中位数。测量值为 Render Graph 的记录、哈希计算、编译和执行相关 CPU marker 的合计。

表: Ping-pong Blur 中 Render Graph 相关 CPU 时间的比较 {{table:unsafe_pass_pingpong_cpu}}

| Blur 次数 | RasterRenderPass×N（µs） | UnsafePass×1（µs） | 差值（µs） |
| :---- | :---- | :---- | :---- |
| 5 | 286.4 | 280.0 | 6.4 |
| 10 | 334.8 | 313.6 | 21.2 |
| 20 | 427.8 | 389.1 | 38.7 |
| 50 | 675.9 | 592.9 | 83.0 |
| 100 | 1080.7 | 898.5 | 182.2 |

测量值是用于在 Editor 环境中确认趋势的数值。随着 Blur 次数增加，RasterRenderPass 方式与 UnsafePass 方式的差距不断扩大。线性拟合后，两种方式斜率之差，即 RasterRenderPass 的额外固定成本，约为每个 Pass 1.8µs。该值是 Editor 上的测量值，并不代表真机上的绝对值，但可以确认 CPU 开销的差距会随 Blur 次数增加而增大。  
这一效果来自 Render Graph 的记录、执行、Profiling 作用域、渲染目标设置等以 Pass 为单位的注册/管理成本的削减。由于绘制命令没有差异，改为 UnsafePass 并不会降低绘制开销。

### **Render Graph Viewer 的可读性也会改善**

注册大量 RasterRenderPass 后，Render Graph Viewer 中会显示同样数量的 Pass。图在横向上被拉长，各功能之间的边界变得难以辨认，资源的依赖关系也不易把握（{{fig:raster_pass_viewer}}）。

![][image2]  
図: 为每次 Blur 处理注册 RasterRenderPass 时的 Render Graph Viewer 显示 {{fig:raster_pass_viewer}}

将功能内部的 Pass 链合并为 1 个 UnsafePass 后，在 Viewer 中可以看到代表整个功能的 1 个 Pass（{{fig:unsafe_pass_viewer}}）。这样一来，各功能之间的边界更加清晰，资源的依赖关系也更容易把握。![][image3]

図: 将整个 Blur 处理序列合并为 1 个 UnsafePass 时的 Render Graph Viewer 显示 {{fig:unsafe_pass_viewer}}

除了削减 CPU 开销之外，能够减少性能分析时的噪声也是一大优点。

但是，不建议仅仅为了改善 Render Graph Viewer 的可读性，就把不同功能的处理或可以成立 SubPass 合并的处理强行合并为 UnsafePass。

# **使用 UnsafePass 时的注意事项**

## **渲染目标的设置**

RasterRenderPass 在记录时通过 SetRenderAttachment 声明渲染目标，而 UnsafePass 则在执行时调用 SetRenderTarget 进行设置。

## **声明依赖关系**

即使使用 UnsafePass，也并不意味着完全脱离 Render Graph。所使用的纹理必须通过 UseTexture 声明读写依赖关系。声明不足时，资源的生命周期管理和同步将无法正确进行。

在 RasterRenderPass 中，渲染目标的使用通过 SetRenderAttachment 声明；而在 UnsafePass 中，仅调用 SetRenderTarget 不会注册依赖关系。渲染目标同样必须通过 UseTexture 声明读写依赖关系。

## **Load/Store Action 的设置**

RenderBufferLoadAction 指定启用渲染目标时对已有内容的处理方式，RenderBufferStoreAction 指定绘制完成后对结果的处理方式。在 Tile-Based GPU 上，不必要的 Load 和 Store 会增加图块内存与主内存之间的传输，因此正确设置非常重要。

在 RasterRenderPass 中，Render Graph 会根据声明的 AccessFlags 自动决定 Load/Store Action。而在 UnsafePass 中，如果像 SetRenderTarget(target) 这样省略 Action，则会应用默认行为 Load／Store，可能因不必要的内存传输而导致性能下降。

因此，在 UnsafePass 中调用 SetRenderTarget 时，必须显式指定 Load/Store Action。

* Load 侧：整个画面都会被覆盖时使用 DontCare，只有需要读取已有内容时才使用 Load  
* Store 侧：结果会被后续处理读取时使用 Store，只有任何地方都不使用时才使用 DontCare

# **代码示例：Ping-pong Blur 的无法合并的 Pass 链**

本节介绍实际的代码示例。需要注意的是，所列代码是为了说明要点的节选，并不是可以直接运行的完整代码。

这是一个交替读写 2 张纹理、连续应用 Blur 的示例。先展示用 RasterRenderPass 实现相同处理的情况，然后展示合并为 1 个 UnsafePass 的情况。desc 是 Blur 用的 Render Texture 描述符，blurMaterial 是 Blur 用的材质。

## **RasterRenderPass 方式：为每次 Blur 处理注册 Pass**

RasterRenderPass 方式如 {{list:raster_pass_pingpong_blur}} 所示，在循环内为每次 Blur 处理调用 AddRasterRenderPass。虽然可以明确声明各 Pass 的输入和输出，但 Blur 次数越多，Render Graph 上的 Pass 数也越多。

`CODE START csharp | 用 RasterRenderPass 实现 Ping-pong Blur {{list:raster_pass_pingpong_blur}}`  
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

## **UnsafePass 方式：在 1 个 Pass 内执行整个处理序列**

UnsafePass 方式如 {{list:unsafe_pass_pingpong_blur}} 所示，只向 Render Graph 注册 1 次，并在 SetRenderFunc 内切换渲染目标。每次 Blur 处理的绘制命令仍然保留，但以 Pass 为单位的管理可以合并为 1 次。

`CODE START csharp | 将 Ping-pong Blur 合并为 1 个 UnsafePass {{list:unsafe_pass_pingpong_blur}}`  
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

# **总结**

本章介绍了在 Render Graph 中应当使用 UnsafePass 的场合以及使用时的注意事项。

应当选择 UnsafePass 的场合有以下 4 种。

* 需要 RasterCommandBuffer/ComputeCommandBuffer 未公开的 API 时  
* 需要传递给外部代码的 CommandBuffer 本身时  
* 希望快速从 Compatibility Mode 迁移到 Render Graph 时  
* 希望将无法合并的 Pass 链合并为 1 个 Pass 时

将无法合并的 Pass 链合并为 1 个 UnsafePass，可以削减以 Pass 为单位的注册/管理成本，并改善 Render Graph Viewer 的可读性。在本项目的 Editor 测量（{{table:unsafe_pass_pingpong_cpu}}）中，每个 RasterRenderPass 可削减约 1.8µs 的固定成本。但需要注意，由于绘制命令本身不变，这并不是以削减 GPU 开销为目的的手法。

另一方面，UnsafePass 也是一种放弃了 Render Graph 部分自动管理和校验的 Pass。使用时，请不要忘记通过 UseTexture 声明依赖关系，并在 SetRenderTarget 中显式指定 Load/Store Action。

基本原则是优先使用 RasterRenderPass 和 ComputePass，而将 UnsafePass 仅限用于声明式 API 无法表达的处理以及整理无法合并的 Pass 链，这样就能兼顾 Render Graph 的优化与 CommandBuffer 的自由度。

# **参考资料**

* [Unity Manual \- Use the CommandBuffer interface in a render graph (UnsafePass)](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/render-graph-unsafe-pass.html)  
* [Unity Manual \- Optimize a render graph](https://docs.unity3d.com/6000.3/Documentation/Manual/urp/render-graph-optimize.html)  
* [Unity Scripting API \- CommandBuffer](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.CommandBuffer.html)  
* [Unity Scripting API \- RenderBufferLoadAction](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.RenderBufferLoadAction.html)  
* [Unity Scripting API \- RenderBufferStoreAction](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Rendering.RenderBufferStoreAction.html)

[image1]: assets/zhang-yubing-1/media/image1.png

[image2]: assets/zhang-yubing-1/media/image2.png

[image3]: assets/zhang-yubing-1/media/image3.png