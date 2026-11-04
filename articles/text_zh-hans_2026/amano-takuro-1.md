* 使用 GraphicsStateCollection 进行着色器预热  
* Amano Takuro/株式会社 CyberAgent SGE 核心技术本部

# 

当着色器首次用于绘制时，运行时会执行编译处理，有时会引起称为着色器编译卡顿的掉帧。本章面向移动端环境（Metal、Vulkan），介绍使用 GraphicsStateCollection 进行着色器预热的基本用法，以及引入后仍残留卡顿的实际案例及其对策。另外，本章内容以 Unity 6.3 LTS（6000.3）为准。

# **GraphicsStateCollection 概述**

GraphicsStateCollection（GSC）是 Unity 6 中新增可用的 API{{fn:gsc_docs}}。它记录游戏过程中使用的着色器变体与图形状态（绘制时的渲染设置）的组合，并统一进行预编译，即预热。它被定位为面向 Metal、Vulkan、D3D12 等现代图形 API 的 ShaderVariantCollection 替代方案。另外，截至 Unity 6000.3，它仍是属于 \`UnityEngine.Experimental.Rendering\` 命名空间的 Experimental API，规范在未来版本中可能发生变化。  
![][image1]  
図: GSC 中记录的信息示例 {{fig:gsc_recorded_info}}

该图是通过脚本获取 GSC 中记录的信息并显示在 EditorWindow 中的示例。记录内容的获取方法将在本章末尾（{{list:gsc_variant_list}}）介绍。

着色器无法直接在 GPU 上执行，需要编译为 GPU 的机器指令。但由于机器指令因 GPU 型号和驱动而异，无法针对所有设备预先编译后再发布。因此，着色器要经过以下流程才能在 GPU 上执行。

1\. 开发者使用 HLSL 等着色器语言编写着色器（资源）  
2\. 构建时，Unity 将着色器编译为各图形 API 对应的中间表示，并存入应用或 AssetBundle。Vulkan 的 SPIR-V、Metal 的 Metal Shading Language（MSL）即属于这种中间表示{{fn:gsc_spirv_msl}}  
3\. 应用运行期间，当着色器首次用于绘制时，从存储的中间表示生成着色器程序  
4\. 接着，将着色器程序与渲染设置组合，由 GPU 驱动编译为 GPU 的机器指令

在首次绘制之前统一完成步骤 3、4 的运行时编译，这就是着色器预热的基本思路。

另外，一个着色器会按每个通道（绘制工序），再按光照影响、是否有雾效等条件（着色器关键字）的组合，生成多个可执行程序。这个生成单位称为着色器变体。

## **Pipeline State Object**

在现代图形 API 中，绘制时进行编译的单位称为 Pipeline State Object（PSO）。PSO 是将着色器变体与渲染设置（顶点布局、颜色格式和 MSAA 等渲染通道附件、混合状态、深度测试、模板状态）作为一个整体，由 GPU 驱动编译为面向 GPU 的机器指令后得到的对象。生成的 PSO 会缓存在设备上，渲染设置相同时会被复用。也就是说，即使是同一个着色器变体，只要渲染设置不同，就会作为不同的 PSO 进行编译。

## **着色器编译卡顿的产生原因**

卡顿的原因是在运行时执行的两阶段编译处理。当 GPU 首次绘制某个着色器变体时，会在 Main Thread 上生成着色器程序（步骤 3），并在 Render Thread 上由 GPU 驱动生成 PSO（步骤 4，编译为机器指令）。

在 Unity Profiler 中，前者可作为 Main Thread 的 \`Shader.CreateGPUProgram\` 观测到，后者可作为 Render Thread 的 \`GpuProgramMetal.GetCachedPipeline\`（iOS）或 \`CreateGraphicsPipelineImpl\`（Android）观测到。尤其在 Metal（iOS）、Vulkan（Android）上，PSO 生成的开销很大，是首次绘制时卡顿的主因。使用 GSC 进行预热，就是在游戏启动后等任意时机提前完成这一 PSO 生成的手段。
  
传统的 ShaderVariantCollection 虽然可以管理“编译哪些着色器变体”，但不包含渲染设置。因此 ShaderVariantCollection 的预热仅止于步骤 3 的着色器程序生成，实际绘制时 GPU 驱动仍要重新生成 PSO，卡顿依然残留。  

![][image2]![][image3]  
図: Main Thread 与 Render Thread 上发生编译的示例 {{fig:gsc_profiler_stutter}}

该图是在 Profiler 的 Timeline 视图中显示 Vulkan（Android）上发生编译的帧的示例。上方是选中 Main Thread 的 \`Shader.CreateGPUProgram\` 的状态，可以确认目标着色器名、通道名和关键字。下方是选中 Render Thread 的 \`CreateGraphicsPipelineImpl\` 的状态，在此例中一次 PSO 生成耗时数十毫秒，可见它是卡顿的主因。

# **使用 GSC 的预热流程**

使用 GSC 的预热分为“渲染设置的收集（跟踪）”和“预热（预编译）”两个阶段。

## **渲染设置的收集**

{{list:gsc_trace_basic}} 展示了跟踪的基本形式。跟踪期间绘制的对象的渲染设置会作为 \`GraphicsStateCollection.GraphicsState\` 记录到 GSC 中。清单中将其整合在一个方法里，但实际上 \`BeginTrace()\` 和 \`EndTrace()\` 要在不同的时机调用，跨多帧收集绘制。

CODE START csharp | 渲染设置的跟踪 {{list:gsc_trace_basic}}  
private GraphicsStateCollection \_gsc;

void Trace() {  
    // 开始收集  
    \_gsc \= new GraphicsStateCollection();  
    \_gsc.BeginTrace();

    // 在此跨多帧绘制需要预热的对象  
    // 绘制时的渲染设置会作为 GraphicsState 记录下来

    // 结束收集并保存  
    \_gsc.EndTrace();  
    \_gsc.SaveToFile(path);  
}  
CODE END

生成的 \`GraphicsStateCollection.GraphicsState\` 因图形 API 而异。请在实际运行所构建应用的设备上进行跟踪。

## **预热（预编译）**

加载收集到的 GSC 并执行预热（{{list:gsc_warmup_progressive}}）。此例中，将导入项目的 GSC 作为 Addressables 资源加载。

CODE START csharp | 使用 WarmUpProgressively 进行分散预热 {{list:gsc_warmup_progressive}}  
async Task WarmUpAsync() {  
    // 在加载包含着色器的资源之后执行  
    \_gsc \= await Addressables.LoadAssetAsync\<GraphicsStateCollection\>(address).Task;

    // 分阶段 WarmUp（基于 Jobs。每帧编译 n 项）  
    StartCoroutine(WarmUpCoroutine());  
}

IEnumerator WarmUpCoroutine() {  
    JobHandle handle \= default;  
    int requestedCount \= 0;  
    // 出错时 isWarmedUp 可能不会变为 true，因此以请求数量判断结束  
    while (requestedCount \< \_gsc.totalGraphicsStateCount) {  
        handle \= \_gsc.WarmUpProgressively(4, handle);  
        handle.Complete();  
        requestedCount \+= 4;  
        yield return null;  
    }  
}  
CODE END

实现预热时的要点如下。

\* 使用 \`WarmUp(dependency)\` 也可以执行预热，但由于会一次性完成全部编译，会产生尖峰。请使用 \`WarmUpProgressively(count, dependency)\` 指定每次的编译数量，将其分散到多帧  
\* 在着色器资源加载之后执行。如果着色器实例不存在，该变体的预热会被跳过  
\* 可通过 \`isWarmedUp\` 或 \`completedWarmupCount \== totalGraphicsStateCount\` 确认是否完成  
\* 不过，发生预热错误时 \`isWarmedUp\` 可能不会变为 true（在 Unity 6000.3 中确认）。因此清单中通过比较请求数量的合计与 \`totalGraphicsStateCount\` 来判断结束，以避免无限循环

# **收集流程的实现示例**

也可以在应用启动时调用 \`BeginTrace()\`，通过手动游玩来收集渲染设置。但根据应用规模，每次版本更新都手动覆盖全部内容是很困难的。因此，这里介绍在收集专用场景中依次绘制资源、机械地执行该流程来进行收集的方法。实现示例使用 Addressables 和 UniTask{{fn:gsc_unitask}}。即使未使用这些库，也可以将资源的异步加载和等待一帧绘制替换为自身项目的机制，实现相同的流程。整体流程如下。

1\. 下载所需资源（仅用 Built-in 资源即可完成时不需要）  
2\. 从 Addressables 的内容目录（\`IResourceLocator\`）获取收集所需资源的 Address  
3\. 切换 Quality Settings（如果着色器不随 Quality Settings 变化则无需切换）  
4\. 开始收集（\`BeginTrace()\`）  
5\. 以叠加方式加载场景，依次加载并绘制需要收集的资源（模型绘制、\`ParticleSystem\` 播放、Timeline 播放）  
6\. 完成收集（\`EndTrace()\`）  
7\. 从 GSC 中删除不需要的着色器  
8\. 保存 GSC（\`SaveToFile(path)\`）  
9\. 卸载场景，一边切换 Quality Settings 一边重复步骤 3～8

从内容目录获取影响渲染设置的资源（场景）、角色和道具等模型资源、特效资源、Timeline 资源，一边切换渲染设置一边依次绘制，从而进行全面收集。之所以按 Quality Settings 分别收集，是因为 MSAA、HDR 等影响 PSO 的渲染设置会随 Quality Level 而变化。

{{list:gsc_scenario_skeleton}} 展示了处理的骨架。按 Quality Settings 重复跟踪，并以 Quality Level 名称命名保存为不同文件。运行时，选择并加载与当前 Quality Level 对应的文件进行预热。

CODE START csharp | 收集流程的骨架 {{list:gsc_scenario_skeleton}}  
private GraphicsStateCollection \_gsc \= new GraphicsStateCollection();

private async UniTaskVoid StartAsync() {  
    // 获取内容目录并下载所需资源  
    // 预先按 Prefix 获取各类 Address 列表  
    await AddressableSetupAsync();

    var qualitySettingNames \= QualitySettings.names;  
    for (var i \= 0; i \< qualitySettingNames.Length; i++) {  
        QualitySettings.SetQualityLevel(i);  
        await TraceAsync();  
    }  
}

private async UniTask TraceAsync() {  
    // 开始 GSC 收集  
    \_gsc.ClearVariants();  
    \_gsc.BeginTrace();

    // 执行收集流程  
    await ExecuteScenarioAsync();

    // 结束 GSC 收集  
    \_gsc.EndTrace();

    // 从 GSC 中删除不在收集范围内的着色器  
    RemoveUnnecessaryVariants();

    var qualityLevelName \= QualitySettings.names\[QualitySettings.GetQualityLevel()\];  
    \_gsc.SaveToFile(\_gscSavePathFormat \+ $"\_{qualityLevelName}");  
}  
CODE END

在流程主体中，按场景依次绘制模型、特效和 Timeline。{{list:gsc_scenario_batch}} 摘录了将角色模型按每批数个生成并绘制的部分。通过等待一帧绘制，该帧的渲染设置会记录到 GSC 中。

CODE START csharp | 依次绘制模型资源（摘录） {{list:gsc_scenario_batch}}  
// 收集角色模型  
for (var i \= 0; i \< \_charaAddressArray.Length; i \+= \_characterNum) {  
    // 每批生成数个角色模型  
    var batchCount \= Mathf.Min(\_characterNum, \_charaAddressArray.Length \- i);  
    for (var j \= 0; j \< batchCount; j++) {  
        var charaHandle \= Addressables.LoadAssetAsync\<GameObject\>(\_charaAddressArray\[i \+ j\]);  
        var charaPrefab \= await charaHandle.ToUniTask();  
        // 在相机视野范围内均匀布置  
        var pos \= new Vector3((j \- batchCount \* 0.5f) \* 2f, 0f, 5f);  
        \_charaObjectArray\[j\] \= Object.Instantiate(charaPrefab, pos, Quaternion.identity);  
        Addressables.Release(charaHandle);  
    }

    // 等待一帧绘制  
    await UniTask.NextFrame();

    for (var j \= 0; j \< batchCount; j++) {  
        DestroyImmediate(\_charaObjectArray\[j\]);  
        \_charaObjectArray\[j\] \= null;  
    }  
}  
CODE END

\`ParticleSystem\` 同样按每批数个生成，不同之处在于先用 \`Simulate()\` 同步地重现播放中途的状态（此例为 0.5 秒后）再绘制（{{list:gsc_scenario_particle}}）。由于无需等待实时播放，可以缩短收集时间。

CODE START csharp | ParticleSystem 的收集（摘录） {{list:gsc_scenario_particle}}  
// 收集 ParticleSystem  
for (var i \= 0; i \< \_particlesAddressArray.Length; i \+= \_effectNum) {  
    var batchCount \= Mathf.Min(\_effectNum, \_particlesAddressArray.Length \- i);  
    for (var j \= 0; j \< batchCount; j++) {  
        var particleHandle \= Addressables.LoadAssetAsync\<GameObject\>(\_particlesAddressArray\[i \+ j\]);  
        var particlePrefab \= await particleHandle.ToUniTask();  
        var pos \= new Vector3((j \- batchCount \* 0.5f) \* 3f, 0f, 5f);  
        \_particleObjectArray\[j\] \= Object.Instantiate(particlePrefab, pos, Quaternion.identity);  
        Addressables.Release(particleHandle);

        // \_effectSimulateTime 秒后的状态通过同步处理重现（无需 Play）  
        var ps \= \_particleObjectArray\[j\].GetComponentInChildren\<ParticleSystem\>();  
        if (ps \!= null) {  
            ps.Simulate(\_effectSimulateTime, withChildren: true, restart: true);  
        }  
    }

    // 等待一帧绘制  
    await UniTask.NextFrame();

    for (var j \= 0; j \< batchCount; j++) {  
        DestroyImmediate(\_particleObjectArray\[j\]);  
        \_particleObjectArray\[j\] \= null;  
    }  
}  
CODE END

使用 Timeline 的演出通过 \`PlayableDirector\` 实际播放来收集（{{list:gsc_scenario_timeline}}）。由于要等待播放时长，最好将目标 Timeline 资源限定为收集所需的部分。

CODE START csharp | Timeline 的收集（摘录） {{list:gsc_scenario_timeline}}  
// 收集 Timeline  
foreach (var timelineAddress in \_timelineAddressArray) {  
    var timelineHandle \= Addressables.LoadAssetAsync\<GameObject\>(timelineAddress);  
    var timelinePrefab \= await timelineHandle.ToUniTask();  
    var timelineObj \= Object.Instantiate(timelinePrefab, Vector3.zero, Quaternion.identity);  
    Addressables.Release(timelineHandle);  
    var director \= timelineObj.GetComponent\<PlayableDirector\>();  
    if (director \!= null) {  
        director.Play();  
        await UniTask.Delay((int)(director.duration \* 1000));  
    }  
    DestroyImmediate(timelineObj);  
}  
CODE END

收集结束后，只保留希望预热的着色器（{{list:gsc_remove_variants}}）。通过许可列表（\`\_allowShaderNames\`）指定要保留的着色器，并从 GSC 中删除范围外的着色器和引用丢失（null）的变体。

CODE START csharp | 删除收集范围外的变体 {{list:gsc_remove_variants}}  
private void RemoveUnnecessaryVariants() {  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    \_gsc.GetVariants(variants);

    foreach (var variant in variants) {  
        if (variant.shader \== null) {  
            \_gsc.RemoveVariant(null, variant.passId, variant.keywords);  
            continue;  
        }

        if (\_allowShaderNames.Contains(variant.shader.name)) {  
            continue;  
        }

        // 从 GSC 中删除不需要的着色器  
        \_gsc.RemoveVariant(variant.shader, variant.passId, variant.keywords);  
    }  
}  
CODE END

流程和资源的管理方式因项目而异，请根据自身项目调整场景组成和收集范围。

# **卡顿残留的实际案例与对策**

即使引入了 GSC 预热，有时卡顿仍会残留。这里介绍其实际案例与对策。

## **Shader Stripping 导致的变体缺失**

这是预热时日志中出现 \`variant not found\` 错误，且在运行时也观测不到该变体的编译（Profiler 的 \`Shader.CreateGPUProgram\` 等）的情况。

原因是 GSC 中包含的着色器变体未包含在应用或 AssetBundle 中。根据 \*\*Project Settings \> Graphics \> Shader Stripping\*\* 的设置，构建时未使用的变体会被剔除。这是在着色器或资源变更导致所需变体发生变化时容易出现的问题。

请确认 Shader Stripping 的设置，并在进行了影响变体的变更后重新收集 GSC。

另外，当 GSC 中虽记录了变体但 \`GraphicsState\` 为 0 项时，也会出现同样的 \`variant not found\` 错误。这种情况下运行时会发生该变体的编译，因此可以根据运行时是否发生编译来区分。

## **元数据不一致导致的 mismatch 警告**

这是执行预热时控制台输出以下警告的情况。

CODE START text  
WarmUp may be inaccurate due to GraphicsDeviceType mismatch.  
WarmUp may be inaccurate due to RuntimePlatform mismatch.  
WarmUp may be inaccurate due to Quality Level mismatch.  
CODE END

原因是 GSC 文件中保存的 \`graphicsDeviceType\` / \`runtimePlatform\` / \`qualityLevelName\` 元数据与运行环境不一致。主要的发生情况有以下两种。

\* 沿用了在不同平台上收集的 GSC（例如：将 iOS 真机上收集的 GSC 用于 Android 构建）  
\* 用编辑器脚本编辑并保存 GSC 时，元数据被编辑器的 Platform 设置覆盖

对策因发生情况而异。在不同平台上收集的 GSC，即使改写元数据，其内容中的 \`GraphicsState\` 也与运行环境不一致，因此需要在相应平台上重新收集。被编辑器的 Platform 设置覆盖时，将元数据修正为真机的值。另外，只有当影响 PSO 的渲染设置（MSAA、HDR 等）和着色器不随 Quality Level 变化时，才可以忽略 \`qualityLevelName\` 的不一致。这种情况下，可以像 {{list:gsc_quality_fix}} 那样在预热前改写它来避免警告。

CODE START csharp | 通过改写 qualityLevelName 避免警告 {{list:gsc_quality_fix}}  
// 加载 GSC  
var gsc \= LoadGsc();  
// 将 qualityLevelName 改为与运行环境一致  
gsc.qualityLevelName \= QualitySettings.names\[QualitySettings.GetQualityLevel()\];  
// 执行预热  
CODE END

## **iOS（Metal）上 AssetBundle 重复打包导致的双重实例**

这是仅在 iOS（Metal）上发生的情况。预热时日志中没有错误，Profiler 的 Main Thread 上也没有发生 \`Shader.CreateGPUProgram\`，但 Render Thread 上却发生了 \`GpuProgramMetal.GetCachedPipeline\`。

原因是 AssetBundle 的重复打包导致着色器被双重实例化。当同一着色器被重复打包到多个 AssetBundle 中时，Unity 会在内存中生成各自独立的着色器实例。预热只会为 GSC 所引用的着色器实例创建 PSO，因此即使是同一个着色器，成为另一个实例的着色器也不会被预热。另外，Vulkan 上不会发生此问题。推测是因为 Metal 与 Vulkan 的 PSO 缓存管理方式不同。

发生该问题的配置示例如下。同一个着色器 X 因依赖关系被同时打包到了两个 Bundle 中。

{{enlargepage}}

CODE START text  
\- 资源  
  \- 着色器 X  
  \- 材质 A（使用着色器 X）  
  \- 材质 B（使用着色器 X）  
  \- GraphicsStateCollection

\- AssetBundle 配置  
  \- Bundle A  
    \- GraphicsStateCollection  
    \- 材质 A（使用着色器 X）  
    \- 着色器 X ← 因依赖关系被打包  
  \- Bundle B  
    \- 材质 B（使用着色器 X）  
    \- 着色器 X ← 因依赖关系被打包  
（着色器 X 未分配 Address 的状态）  
CODE END

在此配置下加载 Bundle A 和 Bundle B 时，绘制材质 B 时会按以下流程发生卡顿。

CODE START text  
\- 运行时的流程  
  1\. 加载 Bundle A 和 Bundle B  
     \- Bundle A 的着色器 X（instanceId=100）  
     \- Bundle B 的着色器 X（instanceId=200）  
  2\. 执行 GraphicsStateCollection 的预热  
     \- 对 GSC 所引用的着色器 X（instanceId=100）进行预热  
  3\. 绘制材质 A  
     \- 着色器 X（instanceId=100）的 PSO 缓存已创建，因此无卡顿  
  4\. 绘制材质 B  
     \- 着色器 X（instanceId=200）没有 PSO 缓存，  
       Render Thread 上执行 PSO 创建，发生卡顿  
CODE END

对策是将着色器集中到单一 Bundle（着色器专用组）中，消除向内容 Bundle 的隐式打包。难以集中到单一 Bundle 时，则按 Bundle 分别准备 GSC。重复的着色器可以用 {{list:gsc_duplicate_check}} 这样的代码确认。使用 Addressables 时，也可以在 \*\*Addressables Report\*\* 窗口中确认着色器的重复。

CODE START csharp | 确认着色器实例的重复 {{list:gsc_duplicate_check}}  
using System.Linq;

// 在 AssetBundle 加载后执行，确认实例是否重复  
var all \= Resources.FindObjectsOfTypeAll\<Shader\>();  
var duplicates \= all.GroupBy(s \=\> s.name).Where(g \=\> g.Count() \> 1);  
foreach (var g in duplicates)  
    foreach (var s in g)  
        Debug.Log($"Duplicate: {s.name} | InstanceID: {s.GetInstanceID()}");  
CODE END

## **iOS（Metal）上使用 SubPass 的着色器预热失败**

到此为止都是由项目设置或 Bundle 设计失误引起的问题。接下来介绍因 GSC 的机制而产生的问题。

将一个渲染通道分割为多个绘制步骤，使后续步骤可以引用前一步骤的绘制结果而无需将其写回主内存的机制，称为 SubPass。在移动 GPU 上，这一系列步骤可以在芯片内的高速内存区域（图块内存）上完成，从而节省内存带宽。在 Metal、Vulkan 上，SubPass 作为 Native Render Pass 执行。仅在 iOS（Metal）上，存在已执行预热且日志无错误，但游戏过程中仍发生 PSO 编译的情况。原因是 iOS（Metal）的预热处理对在该 SubPass（Native Render Pass）内绘制的着色器无法按预期工作。无论是否存在读取前一 SubPass 输出的 input attachments 都会发生。另外，由于预热的内部处理属于非公开 API，这里仅介绍发生条件。

受影响的条件如下。

\* 目标为 iOS / Metal（Apple A/M 芯片的真机）  
\* URP 的绘制处理框架（RenderGraph）的设置 \`RenderGraph.nativeRenderPassesEnabled\` 已启用（Unity 6000.5 及之后此设置已移除，始终启用）  
\* 使用了在 SubPass（Native Render Pass）内绘制的着色器

无论是只写入的 SubPass（URP Deferred 的 GBuffer 通道等），还是带有 input attachments 的 SubPass（同为 Deferred 的 Lighting 通道等），都会同样发生。作为受影响的 SubPass 的编写示例，{{list:gsc_subpass_input}} 展示了设置 input attachments 的情况。

CODE START csharp | 为 SubPass 设置 input attachments 的情况 {{list:gsc_subpass_input}}  
// 在通过 ScriptableRenderContext.BeginRenderPass 开始的  
// 渲染通道内设置 SubPass 的示例  
var inputAttachments \= new NativeArray\<int\>(new\[\] { 0, 1 }, Allocator.Temp);  
context.BeginSubPass(  
    colors,  
    inputAttachments,   // 绑定前一 SubPass 的输出  
                        // 在 Metal 上会被转换为 framebuffer fetch，  
                        // 但不会反映到 GSC 预热时的 PSO 生成中  
    false  
);  
CODE END

在 Unity 6000.3 中难以从根本上解决，只能重新考虑 SubPass 的使用，或通过预渲染来避免游戏过程中的卡顿。预渲染是指在加载界面等期间实际绘制一次使用目标着色器的对象，提前创建 PSO 缓存的方法。可以理解为用与本章收集流程相同的依次绘制，在游戏内代替预热来执行。

## **卡顿残留时的诊断要点**

如前所述，使用 GSC 的着色器预热与项目的资源管理设计（Addressables 的配置、着色器关键字定义、Bundle 设计）密切相关，引入后往往需要调试。请确认预热时的日志，并使用 Profiler 反复测量，确认是否存在预热遗漏。尤其掌握以下几点可以加快诊断。

\* 预热时的日志中是否出现预热错误或 \`variant not found\`  
\* Profiler 中是否发生 Main Thread 的 \`Shader.CreateGPUProgram\`、Render Thread 的 \`GpuProgramMetal.GetCachedPipeline\`（iOS）/ \`CreateGraphicsPipelineImpl\`（Android）  
\* Shader Stripping 的设置是否合适  
\* AssetBundle 的打包是否符合预期

确认 GSC 中记录的变体时，可以使用下一节介绍的变体列表输出（{{list:gsc_variant_list}}）。

# **GSC 文件的操作示例**

最后介绍两个 GSC 文件的操作示例。两者都假定从编辑器脚本执行，可用于确认收集到的 GSC 的内容，以及合并多次收集的结果。

## **确认变体列表和 State 数量**

{{list:gsc_variant_list}} 是加载 GSC 文件并输出元信息和所记录变体列表的示例。

CODE START csharp | 确认变体列表和 State 数量 {{list:gsc_variant_list}}  
void PrintVariantInfo() {  
    // 加载 GSC  
    var gsc \= new GraphicsStateCollection();  
    if (\!gsc.LoadFromFile(path))  
        return;

    // 获取变体  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    gsc.GetVariants(variants);

    // 输出 GSC 的元信息  
    Debug.Log($"\[GSC\] Variants={variants.Count} TotalStates={gsc.totalGraphicsStateCount} "  
            \+ $"Platform={gsc.runtimePlatform} Device={gsc.graphicsDeviceType} Quality={gsc.qualityLevelName}");

    // 输出变体的详细信息  
    foreach (var v in variants) {  
        int stateCount \= gsc.GetGraphicsStateCountForVariant(v.shader, v.passId, v.keywords);  
        string kws \= v.keywords.Length \> 0  
            ? string.Join(" ", v.keywords.Select(k \=\> k.name))  
            : "\<no keywords\>";  
        Debug.Log($"  {v.shader?.name} | states={stateCount} | {kws}");  
    }  
}  
CODE END

## **合并 GSC**

{{list:gsc_merge}} 是合并两个 GSC 的示例。可以将分多次收集的结果整合为一个。

CODE START csharp | 合并两个 GSC {{list:gsc_merge}}  
void MergeGsc() {  
    // 加载两个 GSC  
    var src \= new GraphicsStateCollection();  
    var dst \= new GraphicsStateCollection();  
    if (\!src.LoadFromFile(srcPath) || \!dst.LoadFromFile(dstPath))  
        return;

    // 获取变体  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    var states \= new List\<GraphicsStateCollection.GraphicsState\>();  
    src.GetVariants(variants);

    // 从 src 的 GSC 添加到 dst 的 GSC  
    foreach (var v in variants) {  
        // 获取变体中包含的 GraphicsState  
        src.GetGraphicsStatesForVariant(v.shader, v.passId, v.keywords, states);  
        foreach (var s in states) {  
            // 添加 State  
            dst.AddGraphicsStateForVariant(v.shader, v.passId, v.keywords, s);  
        }  
    }

    dst.SaveToFile(dstPath);  
}  
CODE END

根据 AssetBundle 的配置，有时需要将收集到的 \`GraphicsState\` 移动、整合到其他 GSC 中。在双重实例的对策中介绍的按 Bundle 分别准备 GSC 的情况就是典型例子。此时请组合运用本节的操作示例。

[image1]: assets/amano-takuro-1/media/image1.png

[image2]: assets/amano-takuro-1/media/image2.png

[image3]: assets/amano-takuro-1/media/image3.png