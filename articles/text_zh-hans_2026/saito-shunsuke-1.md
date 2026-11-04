# **风格指南**

# **Unity Performance Tuning Bible 2026**  **稿件撰写模板说明**

本文档是用于收集稿件的模板，目的是转换为 Re:VIEW 格式。

# **第 1 页需要填写的内容**

* 标题  
* 姓名 / 罗马字 / 所属  
* X 账号 / 网站 / GitHub 账号（可选）

# **已应用的基本样式**

* **正文：** Noto Sans JP、12pt、行距 1.8、段后间距 12pt、文字颜色 \#1f2937  
* **标题：** 使用标题 1/2/3 样式，文字颜色 \#111827，标题前间距设置得较宽，标题后间距设置得较窄。

# **面向作者的撰写规则（Re:VIEW 转换必需要素）**

**1\. 标题：** 请使用 Google 文档的“标题 1 / 标题 2 / 标题 3”样式。  
**2\. 正文：** 请使用标准文本样式撰写。  
**3\. 代码块、图片、表格、脚注：** 详情请参阅“**清单、图、表、脚注的写法**”一节，并务必使用 Re:VIEW 转换专用格式（CODE START/END、ID 引用）。  
**4\. 注意：** 请勿更改颜色、字体、缩进等。

# **清单、图、表、脚注的写法**

Re:VIEW 转换时会自动编号，因此作者请勿在正文中直接写入 **\`リスト8.1\`**、**\`図8.5\`**、**\`表8.1\`** 这样的固定编号。  
需要在正文中引用时，请使用专用 ID 而不是固定编号。ID 只能使用英文字母、数字和下划线，并取一个能体现内容的名称。  
示例：

* \`settext\_usage\`  
* \`canvas\_disable\`  
* \`ui\_toggle\_cost\`  
* \`zstring\`

# **代码清单**

代码请按以下格式书写。  
CODE START csharp | SetText 的使用示例 {{list:settext_usage}}  
label.SetText("{0}", number);  
CODE END  
不需要题注时：  
CODE START csharp  
label.text \= number.ToString();  
CODE END  
规则：

* 按 \`CODE START 言語名 | キャプション {{list:id}}\` 的格式书写。  
* C\# 的情况下使用 \`csharp\`。  
* 对于可能在正文中引用的代码，请添加 \`{{list:id}}\`。  
* 代码中请勿使用智能引号 \`“ ”\`，请使用普通的 \`"\`。  
* 请勿为代码添加颜色、粗体等装饰。

在正文中引用时：  
\`{{list:settext_usage}} のように SetText を利用すると、第2引数の値を表示できます。\`  
转换后，Re:VIEW 侧会将其替换为“清单 8.1”这样的带编号引用。

# **图**

请将图片插入 Google Docs 正文中，并在图片正下方写上题注。  
示例：  
![][image1]  
図: 调优宝典 Logo {{fig:bible_logo}}  
规则：

* 题注务必写在图片正下方。  
* 对于可能在正文中引用的图，请添加 {{fig:id}}。

在正文中引用时：  
这是调优宝典的 Logo（{{fig:bible_logo}}）。

# **表**

表格可以使用 Google Docs 的表格功能创建。请在表格正上方写上题注。  
示例：  
表: 显示切换的处理时间 {{table:ui_toggle_cost}}

| 方法 | 处理时间（显示） | 处理时间（隐藏） |
| :---- | :---- | :---- |
| SetActive | 323.79ms | 209.93ms |
| Canvas 的 enabled | 61.25ms | 61.23ms |
| CanvasGroup 的 alpha | 3.64ms | 3.40ms |

规则：

* 题注写在表格正上方。  
* 对于可能在正文中引用的表，请添加 {{table:id}}。  
* 请尽量避免合并单元格。  
* 请勿使用复杂的装饰或着色。  
* 单位请包含在列名中。

在正文中引用时：  
从 {{table:ui_toggle_cost}} 的结果可以看出，使用 CanvasGroup 的方法处理时间最短。

# **脚注**

需要插入脚注的位置，请在正文中写上 {{fn:id}}。脚注内容请统一写在章末或稿件末尾。  
正文中的示例：  
TextMeshPro 的这项功能与 ZString{{fn:zstring}} 组合使用时非常强大。  
脚注定义的示例：  
脚注: {{fn:zstring}} [\[https://github.com/Cysharp/ZString\](https://github.com/Cysharp/ZString)](https://github.com/Cysharp/ZString)  
规则：

* 脚注 ID 只能使用英文字母、数字和下划线。  
* 只包含 URL 的脚注也没有问题。

# **与正文的衔接方式**

在正文中说明清单、图、表时，请勿直接写固定编号，而应使用 ID 引用。  
应避免的示例：  
像清单 8.1 那样使用，即可显示第 2 个参数的值。图 8.5 是禁用 Canvas 的示例。从表 8.1 的结果可以看出……  
推荐的示例：  
像 {{list:settext_usage}} 那样使用，即可显示第 2 个参数的值。{{fig:canvas_disable}} 是禁用 Canvas 的示例。从 {{table:ui_toggle_cost}} 的结果可以看出……  
最终进行 Re:VIEW 转换时，这些引用会被替换为实际的“清单 8.1”“图 8.5”“表 8.1”这样的带编号引用。

# **正文**

# **Unity 3D 角色动画的调优**

* 姓名: Shunsuke Saito

* 所属: 株式会社 Colorful Palette

# **前言**

本章围绕“Unity 中 3D 角色动画功能的优化方法”，从资源设置和实现等多个角度进行介绍。

## **测量环境**

本章的实现与测量在以下环境中进行。

表: 测量环境 {{table:measurement_env}}

| 项目 | 值 |
| :---- | :---- |
| Unity | 6000.5.7f1 |
| 平台 | macOS Standalone (StandaloneOSX) |
| Scripting Backend | IL2CPP / IL2CPP Compiler Configuration \= Release / Code Generation \= OptimizeSpeed |
| Graphics API | Metal / GPU Skinning \= 启用 |
| Quality Level | PC / Skin Weights \= Four Bones |

## **使用的资源及其结构**

测量所用的角色以 unity-chan 为基础，作为 **Generic Rig** 重新导入。在应用后述的 Optimize Game Objects 前后，其规格如下。

表: 测量所用预制体的 Transform 数与顶点数 {{table:character_prefab_spec}}

| 预制体 | Transform 数 | 顶点数 |
| :---- | :---- | :---- |
| Base | 166 | 13,391 |
| Optimized（Optimize Game Objects） | 24 | 13,391 |

# **关于 Unity Animation 的基本控制**

首先，整理一下 Unity 中控制动画的 3 种方法。

## **1\. 使用 Animator 组件进行控制**

将 \`RuntimeAnimatorController\` 分配给 \`Animator\`，这是最常见的方法。状态机、过渡、混合树、参数等机制一应俱全，可以直接使用 Mecanim 的工作流。

CODE START csharp | 通过 Animator 组件进行控制 {{list:animator_control}}  
var animator \= GetComponent\<Animator\>();  
animator.runtimeAnimatorController \= controller;   // 分配状态机  
animator.SetFloat("Blend", 0.5f);                  // 通过参数控制  
CODE END

重要的是，Animator 在内部**构建并执行 Playable Graph**。状态机的评估和 Clip 的采样，在内部都是作为图的节点来处理的。接下来的 2 种方法，相当于自己动手构建这个内部图。

## **2\. 用 Playable API 构建 Playable Graph，并以自动更新进行控制**

这是自己构建 \`PlayableGraph\` 的方法。无需经过状态机即可完全控制混合结构，还能将后述的 Animation Job（\`AnimationScriptPlayable\`）插入到图中的任意位置。

CODE START csharp | 用 Playable API 构建图（自动更新） {{list:playable_auto}}  
var graph \= PlayableGraph.Create("MyGraph");  
var output \= AnimationPlayableOutput.Create(graph, "Animation", animator);

var clipA \= AnimationClipPlayable.Create(graph, waitClip);  
var clipB \= AnimationClipPlayable.Create(graph, walkClip);  
var mixer \= AnimationMixerPlayable.Create(graph, 2);  
graph.Connect(clipA, 0, mixer, 0);  
graph.Connect(clipB, 0, mixer, 1);  
mixer.SetInputWeight(0, 0.5f);  
mixer.SetInputWeight(1, 0.5f);

output.SetSourcePlayable(mixer);  
graph.Play();  
CODE END

更新模式默认为 \`DirectorUpdateMode.GameTime\`。在此模式下，图会在 Unity 的生命周期中（Update 与 LateUpdate 之间的 Animation 阶段），与 Animator 组件相同的时机被**自动评估**。调用方无需每帧做任何事。

## **3\. 用 Playable API 构建 Playable Graph，并以手动更新进行控制**

对同一个图设置 \`DirectorUpdateMode.Manual\` 后，Unity 将完全不再自动评估，只有在调用 \`Evaluate()\` 时才会评估图。

CODE START csharp | 手动更新下的图评估 {{list:playable_manual}}  
graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);  
graph.Play();

void Update()  
{  
    graph.Evaluate(Time.deltaTime);   // 只在调用时评估  
}  
CODE END

由于可以完全控制评估的时机和频率，“只停止特定角色（图）的更新”“每几帧只更新 1 次”这类控制可以直接实现。

3 种方法的关系总结如下。

表: 动画控制 3 种方法的比较 {{table:animation_control_methods}}

| 方法 | 图的构建 | 评估时机 |
| :---- | :---- | :---- |
| Animator 组件 | Unity（内部） | Unity 的 Animation 阶段 |
| Playable API \+ 自动更新 | 自己 | Unity 的 Animation 阶段 |
| Playable API \+ 手动更新 | 自己 | 调用 \`Evaluate()\` 时 |

# **资源的设置**

在介绍代码侧的优化之前，先来看资源侧设置就能应用的优化。本节的主题是**去除骨骼的 Transform 层级**。要获得同样的结果有 2 种手段。

## **Optimize Game Objects（导入时）**

启用 FBX 导入设置（Rig 选项卡）中的 \`Optimize Game Objects\` 后，运行时将不再整体生成骨骼的 GameObject / Transform，Animator 只持有内部骨架。对于本次的角色，**Transform 从 166 个减少到 24 个**（根 \+ 23 个 Renderer）。

※由于骨骼不再以 GameObject 形式存在，遍历层级来访问骨骼的代码将无法工作。如果只需要武器挂点等特定骨骼，可以通过 \`Extra Transforms to Expose\` 单独保留。

## **Optimize Hierarchy（运行时）**

在运行时执行同样操作的 API 是 \`AnimatorUtility.OptimizeTransformHierarchy\`。

CODE START csharp | 运行时的层级扁平化 {{list:optimize_hierarchy_api}}  
// go 是持有 Animator（和有效 Avatar）的根。第 2 个参数是需要暴露的骨骼名数组  
AnimatorUtility.OptimizeTransformHierarchy(go, null);

// 也有逆向操作（用于编辑器扩展或调试时临时恢复层级）  
AnimatorUtility.DeoptimizeTransformHierarchy(go);  
CODE END

即使是无法修改导入设置的资源，只要在生成时执行，也能得到同样的结构。实际确认后发现，对 Base 预制体（166 个 Transform）应用后变为 **24 个 Transform、\`hasTransformHierarchy \== false\`**，与导入时应用了 Optimize Game Objects 的预制体结构完全相同。

![][image2]

図: 优化前的 Hierarchy。\`Character1\_Hips\` 之下延续着 166 个骨骼 Transform 层级。 {{fig:hierarchy_base}}

![][image3]

図: 应用 \`AnimatorUtility.OptimizeTransformHierarchy\` 后。骨骼层级消失，根节点正下方只剩下并列的 23 个 SkinnedMeshRenderer（24 个 Transform）。 {{fig:hierarchy_optimized}}

## **实测：层级扁平化的效果**

我们在 2 种绘制开销下进行了测量。第 1 种是启用全部 Renderer 的配置（300 个角色 × 23 个 Renderer \= 6,900 个 Renderer），第 2 种是每个角色只启用顶点数最多的 1 个 SkinnedMeshRenderer 以降低绘制开销的配置。帧时间取中位数。

表: 层级扁平化的效果（300 个角色、全部 Renderer 与 600 个角色、1 个 Renderer） {{table:optimize_hierarchy_result}}

| 配置 | 变体 | 帧时间 | \`MeshSkinning.Update\` (main) | \`MeshSkinning.CalcMatrices\` (all) | \`Semaphore.WaitForSignal\` |
| :---- | :---- | :---- | :---- | :---- | :---- |
| 300 个角色、全部 Renderer | Base | 18.59 ms | 6.23 ms | 36.633 ms | 5.33 ms |
| 300 个角色、全部 Renderer | Optimize Game Objects | 18.497 ms (±0) | **1.537 ms** (−75.7 %) | **0 ms** | **10.255 ms** |
| 300 个角色、全部 Renderer | Optimize Hierarchy | 18.307 ms (±0) | **1.504 ms** (−76.2 %) | **0 ms** | **9.977 ms** |
| 600 个角色、1 个 Renderer | Base | 6.797 ms | 0.735 ms | 4.385 ms | 5.741 ms |
| 600 个角色、1 个 Renderer | Optimize Game Objects | **5.991 ms (−11.9 %)** | 0.207 ms | **0 ms** | 4.901 ms |
| 600 个角色、1 个 Renderer | Optimize Hierarchy | **6.009 ms (−11.6 %)** | 0.208 ms | **0 ms** | 5.010 ms |

在两种配置下，CPU 侧的处理都以相同比例减少。主线程的蒙皮处理从 6.321 → 约 1.5 ms（全部 Renderer）、0.735 → 约 0.21 ms（1 个 Renderer），均实现了七成以上的削减。至于蒙皮矩阵的计算（\`MeshSkinning.CalcMatrices\`），全线程合计 36.6 ms / 4.4 ms 更是**完全消失**。

不过在“全部 Renderer 配置”下，渲染线程的处理是帧时间的关键路径，因此看不到帧时间的削减。

在降低了绘制开销的 1 个 Renderer 配置中，同样的削减直接体现为**约 12 % 的帧时间缩短**。300 个角色时趋势相同，为 3.312 → 2.899 / 2.901 ms（−12.5 % / −12.4 %）。**导入时的 Optimize Game Objects 与运行时的 Optimize Hierarchy 之间的差异不足 0.1 %**，可见既然结构相同，效果也相同。

在 Profiler 的 Timeline 中也能看到同样的情况。两者都是 300 个角色、全部 Renderer，取帧时间中位数附近的帧。

![][image4]

図: Base（166 个 Transform）。帧时间 18.59 ms 中，主线程的 \`MeshSkinning.Update\` 占 6.23 ms，\`Gfx.WaitForPresentOnGfxThread\` / \`Semaphore.WaitForSignal\` 为 5.33 ms。 {{fig:profiler_ch2_base}}

![][image5]

図: 应用 \`AnimatorUtility.OptimizeTransformHierarchy\` 后。\`MeshSkinning.Update\` 的区块缩小到连标签都无法辨认，而 \`Semaphore.WaitForSignal\` 从 5.33 → 10.44 ms 变长，\`PlayerLoop\` 为 18.58 → 18.91 ms 几乎没有变化。削减下来的 CPU 时间被等待时间所取代。 {{fig:profiler_ch2_optimized}}

## **实测：禁用全部 Renderer（仅动画评估）**

完全去掉绘制和蒙皮，观察对动画评估本身的贡献。角色数为 300。

表: 禁用全部 Renderer 时的帧时间 {{table:optimize_hierarchy_norender}}

| 变体 | 帧时间 |
| :---- | :---- |
| Base | 2.702 ms |
| Optimize Game Objects | 2.687 ms (−0.6 %) |
| Optimize Hierarchy | 2.496 ms (−7.6 %) |

Clip 的采样和混合等评估处理无论有没有 Transform 层级都会运行，因此可以说对这部分几乎没有贡献。由此可知，这项优化的目的主要针对**蒙皮矩阵的计算和向 Transform 的写回**，而不是动画评估本身。

## **本章结论**

* 层级扁平化有助于削减**蒙皮矩阵计算（\`MeshSkinning.CalcMatrices\`）**

* 导入时（Optimize Game Objects）与运行时（\`AnimatorUtility.OptimizeTransformHierarchy\`）效果相同

* 选择哪一种取决于运用方式：自有资源就通过导入设置完成，有无法修改导入设置的原因时就使用运行时 API

* **在制作阶段就把模型本身的顶点数、骨骼数控制在合理范围**是最优先事项

# **充分利用 Job Thread**

## **Unity 的动画并行化**

对于 Animator 组件以及自动更新的 Playable Graph，Unity 会**将所有图统一调度，分散到 Animation Job Thread 上**。

问题在于手动更新。\`PlayableGraph.Evaluate()\` 只会**并行化该图内部的评估处理**，不会跨图并行化。因此，若以“1 个角色 \= 1 个 Playable Graph”这种直观的结构来控制 N 个角色，就要调用 N 次 \`Evaluate()\`，**N 个图将被串行处理**。

这是 Unity Playable Graph 不可避免的机制，但对策很简单：**把 N 个角色汇总到 1 个图中**。

CODE START csharp | 将全部角色集中到 1 个图 {{list:shared_graph}}  
// 在 1 个图上挂载每个角色的输出和子树  
\_sharedGraph \= PlayableGraph.Create("Shared");  
\_sharedGraph.SetTimeUpdateMode(DirectorUpdateMode.Manual);

foreach (var character in characters)  
{  
    var output \= AnimationPlayableOutput.Create(\_sharedGraph, "Animation", character.Animator);  
    output.SetSourcePlayable(BuildClipSource(\_sharedGraph));   // 各角色的混合器  
}

\_sharedGraph.Play();

// 只更新 1 次  
void Update() \=\> \_sharedGraph.Evaluate(Time.deltaTime);  
CODE END

## **实测 1：控制方法 × 角色数**

禁用全部 Renderer，只提取动画评估的开销。帧时间取中位数。

表: 各控制方法和角色数下的帧时间 {{table:driver_frame_time}}

| 控制方法 | 100 个角色 | 300 个角色 | 600 个角色 |
| :---- | :---- | :---- | :---- |
| Animator 组件 | 0.899 ms | 2.413 ms | **5.004 ms** |
| Playable API \+ 自动更新 | 0.899 ms | 2.498 ms | **5.093 ms** |
| Playable API \+ 手动更新（1 个角色 1 个图） | 1.395 ms | 3.925 ms | **8.396 ms** |
| Playable API \+ 手动更新（全体 1 个图） | 0.800 ms | 2.115 ms | **4.500 ms** |

**Animator 与 Playable 自动更新的开销几乎相同。**“Playable API 更轻量”这种说法，至少在“简单的动画控制”条件下并不成立。Animator 的状态机评估（\`AnimatorControllerPlayable.PrepareFrame\`）即使在 600 个角色时全线程合计也只有 2.624 ms，其中主线程仅 0.161 ms，可以说这部分处理成为瓶颈的可能性极低。（不过，2 ms 算大还是算小，取决于具体的游戏。）

**手动更新会因图的结构而相差 1.87 倍。**同样是 600 个角色，根据图的汇总方式不同，可能比自动更新更快，也可能更慢。

## **实测 2：线程的使用方式**

600 个角色、禁用 Renderer。这是根据 Unity Profiler 的捕获按线程汇总的结果。

表: 图粒度导致的线程使用方式差异（600 个角色） {{table:graph_granularity_threads}}

|  | 1 个角色 1 个图（600 个图） | 600 个角色 1 个图 |
| :---- | :---- | :---- |
| 帧时间 | **9.490 ms** | **4.928 ms** |
| 主线程的 \`Evaluate\` | 9.028 ms | 4.556 ms |
| **承担动画处理的工作线程数** | **5** | **15** |
| \`Animator.ProcessGraph\` 全线程合计 | 0.920 ms | **19.034 ms** |
| 每个工作线程的 \`Animator.ProcessGraph\` | 0.32 / 0.15 / 0.03 / 0.02 / 0.002 ms | 各**约 1.19 ms**（15 个线程几乎均等） |

**1 个角色 1 个图时**，动画处理几乎全部在主线程上。只是 600 次 \`Evaluate()\` 依次运行而已，工作线程几乎没有工作。

**600 个角色 1 个图时**，15 个工作线程全部几乎均等地执行 \`Animator.ProcessGraph\`。

![][image6]

図: 1 个角色 1 个图（600 个图）的 Timeline。主线程的 \`ATB.ManualEvaluate\` 占约 8.6 ms，大部分 Job 工作线程保持“Idle”。 {{fig:profiler_timeline_per_character}}

![][image7]

図: 600 个角色 1 个图的 Timeline。主线程的 \`ATB.ManualEvaluate\` 减半至约 4.3 ms，所有工作线程同时各执行约 1.2 ms 的 \`EngineJob\`。 {{fig:profiler_timeline_single_graph}}

**关于数值的解读有 1 点补充。**

本表的数值取自连接 Unity Profiler 状态下的捕获，因此比不连接 Profiler 运行的测量（4.500 ms / 8.396 ms）略重。章末的汇总表使用的是未连接 Profiler 的数值。

## **本章结论**

表: 600 个角色下的帧时间汇总 {{table:graph_summary}}

| 配置 | 600 个角色的帧时间 |
| :---- | :---- |
| 1 个角色 1 个图 \+ 手动更新 | 8.396 ms |
| Animator（自动） | 5.004 ms |
| 600 个角色 1 个图 \+ 手动更新 | **4.500 ms** |

* 对于 Animator 和自动更新的 Playable Graph，Unity 会把所有图统一分散到 Animation Job Thread 上

* 手动更新只以图为单位并行化

* 选择手动更新时，务必把图的粒度纳入设计考量

* 单纯替换为 Playable API 几乎没有优化效果。使用 Playable API 的真正价值在于“可以插入 Animation Job”“可以控制图结构和更新时机”这类**动画控制的灵活性**

# **关于 Animation C\# Jobs 的使用与 SIMD 支持**

“想以程序化方式控制动画”这种需求在游戏开发中必然会出现。例如“想让角色的脚贴合地面的形状移动”等情况。

这种情况下可采用的手段之一是 Two-Bone IK 这种方法。

实际实现 Two-Bone IK 这样的程序化动画控制时，有 3 种选择。

1. 在 \`LateUpdate\` 中直接改写 Transform

2. 向 \`AnimationScriptPlayable\` 插入 \`IAnimationJob\`

3. 在 2 的基础上用 Burst 编译

本次关注这些方法的性能。

3 种实现都执行**完全相同的运算**。求解方法抽取为 1 个 static 方法，LateUpdate 版、Job 版、Burst 版共用它，因此不同的只有执行的位置。

## **什么是 Animation C\# Jobs**

Animation C\# Jobs 是通过 \`AnimationScriptPlayable\` 和 \`IAnimationJob\` **将自己的处理插入到动画评估管线之中**的机制。Job 会在与 Clip 评估和混合相同的流程中被调用，并通过 \`AnimationStream\` 和 \`TransformStreamHandle\` 访问骨骼。

与 \`LateUpdate\` 的区别在于代码运行的位置。\`LateUpdate\` 在主线程上，直接改写动画评估结束后的 Transform。处理会按角色数在主线程上串行堆积。Animation Job 在评估管线内部（Animation Job Thread）被调用，因此 Unity 可以像其他动画评估一样将其分散到工作线程。即使是相同的运算，是否成为主线程的负担也会有所不同。

必须经由 \`AnimationStream\` 访问这一限制是其代价。Job 能看到的只有事先绑定的句柄所对应的部分，无法访问任意的 \`Transform\` 或 \`GameObject\`。

## **实现**

Job 本体只是在 \`ProcessAnimation\` 中调用求解方法。

CODE START csharp | Two-Bone IK 的 Animation Job（支持 Burst） {{list:two_bone_ik_job}}  
\[BurstCompile\]  
public struct TwoBoneIkJobBurst : IAnimationJob  
{  
    public TransformStreamHandle Root;  
    public TransformStreamHandle Mid;  
    public TransformStreamHandle Tip;  
    public NativeArray\<float3\> Target;   // 长度为 1。每帧从主线程写入  
    public float Weight;  
    public int Iterations;

    public void ProcessRootMotion(AnimationStream stream) { }

    \[BurstCompile\]  
    public void ProcessAnimation(AnimationStream stream)  
    {  
        TwoBoneIkCore.SolveStreamIterations(  
            ref stream, Root, Mid, Tip, Target\[0\], Weight, Iterations);  
    }  
}  
CODE END

骨骼的读写通过 \`TransformStreamHandle\` 和 \`AnimationStream\` 的组合进行。

CODE START csharp | 从 AnimationStream 读写骨骼 {{list:ik_solve_stream}}  
static void SolveStream(  
    ref AnimationStream stream,  
    TransformStreamHandle root, TransformStreamHandle mid, TransformStreamHandle tip,  
    float3 target, float weight)  
{  
    var rootPosition \= (float3)root.GetPosition(stream);  
    var midPosition \= (float3)mid.GetPosition(stream);  
    var tipPosition \= (float3)tip.GetPosition(stream);  
    var rootRotation \= (quaternion)root.GetRotation(stream);  
    var midRotation \= (quaternion)mid.GetRotation(stream);

    Solve(rootPosition, midPosition, tipPosition, target, rootRotation, midRotation, weight,  
        out var newRootRotation, out var newMidRotation);

    mid.SetRotation(stream, newMidRotation);  
    root.SetRotation(stream, newRootRotation);  
}  
CODE END

句柄通过 \`Animator.BindStreamTransform\` 创建，Job 作为 Clip 的后级插入到图中。

CODE START csharp | 将 Animation Job 插入图中 {{list:ik_playable_connect}}  
var job \= new TwoBoneIkJobBurst  
{  
    Root \= animator.BindStreamTransform(rootTransform),  
    Mid \= animator.BindStreamTransform(midTransform),  
    Tip \= animator.BindStreamTransform(tipTransform),  
    Target \= ikTarget,   // NativeArray\<float3\>  
    Weight \= 1f,  
    Iterations \= iterations,  
};

var ikPlayable \= AnimationScriptPlayable.Create(graph, job, 1);  
graph.Connect(clipSource, 0, ikPlayable, 0);   // clipSource \= Clip 侧的混合器  
ikPlayable.SetInputWeight(0, 1f);  
output.SetSourcePlayable(ikPlayable);  
CODE END

IK 目标这类每帧变化的输入放在 \`NativeArray\` 中，由主线程写入。

Burst 通过为类添加 \`BurstCompile\` 特性来应用。

Burst Compiler 的优化是否实际生效，可以使用 Burst Inspector 等工具确认。

## **实测**

300 个角色、禁用 Renderer。每个角色的 IK 求解次数分别设为 1 / 16 / 64。

表: 各 IK 求解次数下的帧时间（300 个角色、禁用 Renderer） {{table:ik_frame_time}}

| 实现 | 1 次 | 16 次 | 64 次 |
| :---- | :---- | :---- | :---- |
| 无 IK（基准） | 2.501 ms | 2.601 ms | 2.596 ms |
| LateUpdate | 2.699 ms | 4.400 ms | **9.094 ms** |
| Animation Job | 2.411 ms | 2.496 ms | **2.795 ms** |
| Animation Job \+ Burst | 2.494 ms | 2.496 ms | **2.792 ms** |

CPU 时间的构成（64 次时）：

表: 各 IK 实现的 CPU 时间构成（求解 64 次） {{table:ik_cpu_breakdown}}

| 实现 | IK 的 CPU 时间（全线程） | 其中主线程 | 对帧时间的增加 |
| :---- | :---- | :---- | :---- |
| LateUpdate | 6.910 ms | **6.910 ms（100 %）** | \+6.498 ms |
| Animation Job | 7.790 ms | 0.487 ms（6.3 %） | \+0.199 ms |
| Animation Job \+ Burst | 6.932 ms | 0.436 ms（6.3 %） | \+0.196 ms |

## **在 Profiler Timeline 中查看执行线程的差异**

相同的运算在哪个线程上执行，会体现在 Unity Editor 的 Profiler 中。以下是在 Profiler 窗口中打开求解 64 次、300 个角色的捕获，并选择帧时间中位数附近的帧。由于是连接 Profiler 时的捕获，与上表的数值并不完全一致。

![][image8]

図: LateUpdate 实现。帧时间 8.698 ms 中 \`LateBehaviourUpdate\` 占 6.63 ms。15 个工作线程在帧的大部分时间（约 6.8〜7.7 ms）保持 \`Idle\`。 {{fig:profiler_ik_lateupdate}}

![][image9]

図: Animation Job 实现。相同的运算作为 \`AnimationScriptPlayable.ProcessAnimation\` 分散到 15 个工作线程，每个线程各执行约 0.5 ms。帧时间为 2.796 ms。 {{fig:profiler_ik_animationjob}}

在 LateUpdate 实现中，300 个角色的 \`ATB.TwoBoneIk.LateUpdate\`（合计 6.577 ms）全部位于主线程的 \`LateBehaviourUpdate\` 之中。此时工作线程承担的只有基础动画评估（\`Animator.ProcessGraph\` 每个线程 0.12〜0.29 ms），IK 的计算一个都没有。

在 Animation Job 实现中，\`AnimationScriptPlayable.ProcessAnimation\` 的全线程合计为 7.891 ms，其中主线程为 0.498 ms。其余 7.393 ms 几乎均等地分散在 15 个工作线程上（每个线程 0.47〜0.51 ms）。总 CPU 时间虽然比 LateUpdate 实现更多，但由于分散到 16 个线程上，帧时间从 8.698 → 2.796 ms。

## **可以得出的结论**

**对于这种情况的优化，最有效的不是 Burst，而是转移执行线程。** LateUpdate 实现把 IK 的 CPU 时间全部由主线程承担，因此在 64 次时帧时间从 2.596 → 9.094 ms 膨胀。把相同运算移到 Animation Job 后，主线程的负担几乎消失，对帧时间的增加相差 **33 倍**。

另一方面，**总 CPU 时间反而是 LateUpdate 版更少**。Animation Job 对每个 \`AnimationScriptPlayable\` 节点有固定开销，300 个角色约为 2.16 ms。即便如此，并行化的收益仍远远超过这部分。

**在接近实际运用的“每个角色 1 次”条件下，3 种实现的差异基本在测量噪声范围内**。300 个角色左右的 Two-Bone IK，无论选择哪种实现都没有可感知的差异。

## **Burst 仅靠添加特性并不会生效**

认为加上 \`\[BurstCompile\]\` 就会变成 SIMD 并快上数倍，这种理解是错误的。在本次实测中，Burst 带来的总 CPU 时间削减为 11.0 %。扣除节点固定开销后，按“每次求解的边际开销”来看，

* 无 Burst：(7.790 − 2.157) / 63 \= **0.0894 ms**

* 有 Burst：(6.932 − 2.167) / 63 \= **0.0756 ms**（−15.4 %）

Burst 所做的是通过 LLVM 生成原生代码，是否生成 SIMD 指令取决于所写代码的形态。**如果不实现能被正确 SIMD 化的代码，就无法获得大的收益。**

Animation C\# Jobs 有 2 个使 SIMD 化变得困难的性质。

**\`AnimationStream\` 的读写频繁出现在处理之中。**每次求解都通过 \`GetPosition\`、\`GetRotation\` 读取骨骼，再通过 \`SetRotation\` 写回。每次迭代流的状态都会改变，迭代之间残留依赖关系，无法把多次求解合并成 1 条向量指令。此外，\`IAnimationJob\` 接收的 \`AnimationStream\` 只有 1 个角色的量，Job 内部根本没有用于“同时计算 4 个角色”的数据。

**大量使用超越函数。**根据余弦定理求弯曲角的 \`acos\`、\`quaternion.AxisAngle\` 内部调用的三角函数、\`math.length\` 的平方根。这些会变成跨 lane 的计算或标量的库调用，从而阻止向量化。

Burst 的 LLVM 也会对反复调用求解方法的循环**明确报告无法向量化**（优化备注 \`loop not vectorized: instruction return type cannot be vectorized\`）。

## **充分利用 Burst 收益的策略**

如果想充分利用 Burst 的收益，可以考虑使用 \`C\# Job System\` **不经由 \`AnimationStream\` 读写 Transform** 的方案。Transform 的读写可以通过 \`TransformAccessArray\` 和 \`IJobParallelForTransform\` 汇总到 Job 侧；也可以只把骨骼的收集和写回交给 Animation Job，而把繁重的计算抽取为汇总了 N 个角色的批处理 Job。两种方式都不以角色为单位，而是**N 个角色的数据集中在同一个 Job 中**，因此满足 SIMD 化的前提。

在此基础上，批处理 Job 的内容需要按如下方式整理。

* 采用每个分量 1 个数组的 SoA（Structure of Arrays），让角色在内存中连续排列

* 把分支替换为 \`math.select\` 或 \`math.max\`，使循环体成为 1 条直线

* 把超越函数赶出循环（用恒等式改写、查表、近似）

* 不用 \`float3\` 而用 \`float4\` 以“4 个角色各占 1 个 lane”的方式编写，不依赖自动向量化的判断

代价是设计的复杂度。\`IAnimationJob\` 最大的优点是只需在图中插入 1 个节点，就能在评估管线的正确时机访问骨骼的最新状态，这种便利性在该策略下将被舍弃。本文没有对这种结构进行实测，因此这里不展示能快多少。请把它当作一项提议：如果期待 Burst 带来数倍的效果，就要以这个方向的设计变更为前提。

## **本章结论**

* Animation Job 的价值在于**能够利用 Animation Job Thread**

* 运算量较少时，\`AnimationScriptPlayable\` 的节点固定开销反而更大。直接写在 LateUpdate 中总 CPU 时间更少

* \`\[BurstCompile\]\` 仅靠添加不会变成 SIMD。需要以 SIMD 能被应用为前提进行数据和处理的设计

# **动画 LOD 的实现**

## **机制与有效的场合**

虽然也取决于游戏规范，但基本上没有必要对远处的角色也每帧更新。若每 N 帧只更新 1 次，该角色的平均开销就变为 1/N。这就是“动画 LOD”。它把网格 LOD 按距离分级控制绘制开销的思路，应用到**动画的更新频率**上。

在以下场合效果显著。

* 画面内有数百个规模的角色，且大部分是远景或群众角色

* 远处角色相当于 30 fps / 15 fps 的动作在体感上可以接受（相机越远，降频越不容易被察觉）

* 动画评估在帧时间中占据不可忽视的比例

反之，若应用于近距离、在相机中占据大面积的角色，卡顿会立刻被察觉。根据距离或屏幕占比切换降频等级，应该是实际运用中的设想。

## **简单的实现：自己调用 \`Animator.Update()\`**

设置为 \`animator.enabled \= false\` 以停止 Unity 的自动更新，只在需要的帧显式调用 \`Animator.Update()\`。跳过的帧的时间累积后一并传入。

**这种实现有一个副作用，即会失去前面所述的 Animation Job Thread 上的并行性**。

CODE START csharp | 自己调用 Animator.Update() 的降频实现 {{list:animlod_manual_update}}  
animator.enabled \= false;   // 初始化时。停止 Unity 的自动更新

void Update()  
{  
    foreach (var character in \_characters)  
    {  
        character.PendingDeltaTime \+= Time.deltaTime;  
        if (\_frameIndex % interval \!= character.Slot) continue;   // Slot \= index % interval

        character.Animator.Update(character.PendingDeltaTime);    // 用累积的时间显式更新  
        character.PendingDeltaTime \= 0f;  
    }  
    \_frameIndex++;  
}  
CODE END

## **直接使用 Playable API 的实现示例**

如果是手动更新（\`DirectorUpdateMode.Manual\`）的 Playable Graph，只需对 \`Evaluate()\` 的调用本身进行降频即可。

CODE START csharp | 对手动更新图的 Evaluate() 进行降频的实现 {{list:animlod_manual_evaluate}}  
graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);  
graph.Play();

void Update()  
{  
    foreach (var character in \_characters)  
    {  
        character.PendingDeltaTime \+= Time.deltaTime;  
        if (\_frameIndex % interval \!= character.Slot) continue;

        character.Graph.Evaluate(character.PendingDeltaTime);  
        character.PendingDeltaTime \= 0f;  
    }  
    \_frameIndex++;  
}  
CODE END

有 1 点需要注意。在前一章推荐的“把全部角色集中到 1 个图”的结构中，整个图由 1 次 \`Evaluate()\` 评估，因此**无法表达以角色为单位的降频**。如上面的代码那样采用 1 个角色 1 个图就可以降频，但相应地会失去集中带来的并行化。

无论哪种实现，重要的都是 \`Slot\`（相位）。若所有角色在同一帧更新，开销就会集中在那一帧。通过 \`index % interval\` 错开相位，就变成每帧各更新 1/N 的角色。

## **实测**

600 个角色、禁用 Renderer。首先是 Animator 的实现。

**平均值**表示 12 秒内的总开销，**中位数与 p95 的差距**表示帧时间的波动程度。

表: 降频与错开相位的帧时间统计（Animator、600 个角色） {{table:animlod_animator_stats}}

| 方式 | 平均 | 中位数 | p95 | p99 | 最大 | 标准差 |
| :---- | :---- | :---- | :---- | :---- | :---- | :---- |
| 每帧更新（Animator 自动） | 5.037 ms | 5.000 ms | 5.303 ms | 5.707 ms | 16.901 ms | 0.433 |
| 每 2 帧 1 次、无相位 | 4.781 ms | 0.648 ms | 9.607 ms | 9.816 ms | 12.194 ms | 4.544 |
| 每 2 帧 1 次、错开相位 | 4.831 ms | 4.803 ms | 5.099 ms | 5.212 ms | 8.804 ms | 0.171 |
| 每 4 帧 1 次、无相位 | 2.613 ms | **0.382 ms** | **9.470 ms** | 9.658 ms | 11.171 ms | 3.863 |
| 每 4 帧 1 次、错开相位 | 2.578 ms | **2.593 ms** | **2.707 ms** | 2.908 ms | 11.722 ms | 0.185 |

Playable API（1 个角色 1 个图、手动更新）也呈现同样的趋势。

表: 降频与错开相位的帧时间统计（Playable API、1 个角色 1 个图） {{table:animlod_playable_stats}}

| 方式 | 平均 | 中位数 | p95 | 标准差 |
| :---- | :---- | :---- | :---- | :---- |
| 每帧更新 | 8.496 ms | 8.493 ms | 9.001 ms | 0.337 |
| 每 4 帧 1 次、无相位 | 2.438 ms | 0.369 ms | 8.855 ms | 3.570 |
| 每 4 帧 1 次、错开相位 | 2.486 ms | **2.495 ms** | **2.613 ms** | 0.173 |

## **错开相位不是“减少开销”，而是“摊平开销”**

降频到 1/4 时的平均值几乎不受有无相位的影响，**总开销基本相同**。错开相位并没有减少工作量。

另一方面，帧时间的分布出现了如下差异。

* 无相位：每 4 帧中有 3 帧什么都不做，在 1 帧中集中处理 600 个角色。中位数下降，但 p95 和标准差大幅上升

* 错开相位：每帧各处理 150 个角色。中位数与 p95 几乎一致，波动很小

如果只看中位数 0.382 ms 这个数字就为“降到了 1/13”而高兴，实际得到的会是一款每 4 帧就出现 1 次 9.5 ms 尖峰的游戏。**错开相位是面向体感品质的措施**，标准差 0.185 比每帧更新的 0.433 更稳定。

这种分布的差异会直接体现在 Profiler 的 CPU 图表中（都是连接 Profiler 时的捕获，因此比上表未连接时的数值偏重。此外，图表的纵轴刻度为自动调整，两张图并不相同）。

![][image10]

図: 每 4 帧 1 次、无相位。CPU 图表呈“几乎 0 ms 的 3 帧低谷 \+ 约 10 ms 的 1 帧尖峰”的梳状。下方的 Timeline 中，所选空帧（0.47 ms）的两侧是集中处理 600 个角色的 \`ATB.ManualAnimatorUpdate\`（10.37 ms）所在的帧。 {{fig:profiler_lod_decimated}}

![][image11]

図: 每 4 帧 1 次、错开相位。每帧各处理 150 个角色，因此波形平坦（约 3.5 ms），尖峰消失。 {{fig:profiler_lod_staggered}}

## **朴素的实现会失去并行化**

**每 2 帧 1 次的降频几乎没有削减任何东西。**更新次数减半，平均帧时间的削减却仅有 **4.1 %**。

其理由可以从 Profiler 标记的构成中看出。

表: 朴素的降频实现所失去的并行性 {{table:animlod_parallel_loss}}

| 方式 | 每帧更新的角色数 | 主线程的动画更新时间 | \`Animator.ProcessGraph\` 全线程合计 | 其中主线程 |
| :---- | :---- | :---- | :---- | :---- |
| 每帧更新（Unity 的批量更新） | 600 | \`Animators.Update\` 4.046 ms | 20.104 ms | 1.257 ms |
| 每 2 帧 1 次、错开相位 | 300 | \`ATB.ManualAnimatorUpdate\` 4.532 ms | 0.549 ms | 0.317 ms |
| 每 4 帧 1 次、错开相位 | 150 | \`ATB.ManualAnimatorUpdate\` 2.316 ms | 0.278 ms | 0.180 ms |

每帧更新时，工作中留在主线程的只有 6 %，其余分散到了工作线程。而显式调用 \`Animator.Update()\` 时**会当场同步评估**，因此工作线程几乎没有被使用。Unity 批量更新所具备的并行化，被作为降频的代价放弃了。

换算成每个角色的主线程开销，

* 每帧更新：4.046 ms / 600 个角色 \= **6.7 µs/角色**（并行）

* 每 2 帧 1 次：4.532 ms / 300 个角色 \= **15.1 µs/角色**（串行）

* 每 4 帧 1 次：2.316 ms / 150 个角色 \= **15.4 µs/角色**（串行）

串行化的瞬间，每个角色的开销就变为约 2.3 倍，此后不论降频倍率如何都保持恒定。也就是说，**2 倍降频所获得的 2 倍削减，几乎被串行化导致的 2.3 倍恶化抵消**。实测的 4.1 % 只不过是其残差。

保持朴素实现的话，要到 4 倍降频（**−48.8 %**）才终于出现实质性的效果。不过，这是可以避免的损失。

## **在保持并行性的同时降频**

\`Animator.Update()\` 是同步的，这一点无法避免。但如果改为**“每帧选择参与 Unity 批量更新的角色”**的形式，就能在降频的同时保持并行性。有 2 种实现，两者都仍然使用 Animator（及其状态机）。

### **方法 1：切换 \`Animator.enabled\`**

Unity 的动画更新会收集当时启用的 Animator 组成 1 个批次，分散到 Animation Job Thread 上。因此，不必调用 \`Animator.Update()\`，**只需让该帧想要更新的角色处于 \`enabled \= true\` 即可**。评估由 Unity 执行，并行化原样保留。

CODE START csharp | 切换 Animator.enabled 的降频实现 {{list:animlod_enable_toggle}}  
// 初始化时。跳过的帧通过 speed 补回  
foreach (var character in \_characters)  
{  
    var animator \= character.Animator;  
    animator.keepAnimatorStateOnDisable \= true;   // 禁用时不回退状态（必需）  
    animator.speed \= interval;                    // 补偿每 interval 帧只更新 1 次的部分  
}

void Update()  
{  
    foreach (var character in \_characters)  
    {  
        var scheduled \= \_frameIndex % interval \== character.Slot;  
        if (character.Animator.enabled \!= scheduled)  
        {  
            character.Animator.enabled \= scheduled;   // 只选择参与 Unity 批量更新的角色  
        }  
    }  
    \_frameIndex++;  
}  
CODE END

\`keepAnimatorStateOnDisable \= true\` 不能忘记。在默认值 \`false\` 下，每次禁用 Animator 时控制器的状态都会回退，动画无法推进。

限制在于 \`Animator.speed\` 是**速率**。无法像 \`Animator.Update(pendingDeltaTime)\` 那样“精确传入累积的 Δt”，只会按更新那一帧的 \`Time.deltaTime × interval\` 推进。在帧时间波动的环境下推进量也会波动，但平均播放速度得以保持。

### **方法 2：按相位拆分图**

前一章的结论是“把 N 个角色汇总到 1 个手动图就会被并行化”。把它与降频组合起来。**按相位的数量创建图，每帧只 \`Evaluate()\` 1 个图**。1 个图中包含 N/interval 个角色，因此在这 1 次 \`Evaluate()\` 之中并行化会生效。

只要把状态机作为 \`AnimatorControllerPlayable\` 放入图中，就会运行与分配给 Animator 组件时相同的评估。

CODE START csharp | 按相位拆分图的降频实现 {{list:animlod_bucketed_graphs}}  
// 初始化时：每个相位桶 1 个手动图  
for (var bucket \= 0; bucket \< interval; bucket++)  
{  
    var graph \= PlayableGraph.Create($"Bucket\_{bucket}");  
    graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);  
    \_bucketGraphs\[bucket\] \= graph;  
}

foreach (var character in \_characters)  
{  
    var graph \= \_bucketGraphs\[character.Slot\];  
    character.Animator.runtimeAnimatorController \= null;   // 评估交给图侧  
    var output \= AnimationPlayableOutput.Create(graph, "Animation", character.Animator);  
    output.SetSourcePlayable(AnimatorControllerPlayable.Create(graph, controller));  
}

foreach (var graph in \_bucketGraphs) graph.Play();

void Update()  
{  
    for (var bucket \= 0; bucket \< interval; bucket++) \_pendingDeltaTime\[bucket\] \+= Time.deltaTime;

    var scheduled \= \_frameIndex % interval;  
    \_bucketGraphs\[scheduled\].Evaluate(\_pendingDeltaTime\[scheduled\]);   // 每帧只处理 1 个桶  
    \_pendingDeltaTime\[scheduled\] \= 0f;  
    \_frameIndex++;  
}  
CODE END

这种方式可以**把累积的 Δt 原样传给 \`Evaluate()\`**，因此推进方式精确。代价是要持有与降频等级数相同数量的图，要动态改变 LOD 等级就需要在图之间移动角色（重新创建输出）。

### **实测：3 种实现的比较**

600 个角色、禁用 Renderer。以下是平均帧时间，以及相对于每帧更新（5.037 ms）的削减率。

表: 3 种降频实现的帧时间比较（600 个角色） {{table:animlod_impl_compare}}

| 实现 | 1/2 降频 | 1/4 降频 |
| :---- | :---- | :---- |
| 显式调用 \`Animator.Update()\`（错开相位） | 4.831 ms（−4.1 %） | 2.578 ms（−48.8 %） |
| 切换 \`Animator.enabled\` | **3.350 ms（−33.5 %）** | **1.835 ms（−63.6 %）** |
| 按相位的手动图（\`AnimatorControllerPlayable\`） | **3.082 ms（−38.8 %）** | **1.541 ms（−69.4 %）** |

**朴素的实现最不划算。**尤其是 1/2 降频时差异很大，−4.1 % 变成了 −33.5 % / −38.8 %。前一节“2 倍降频会被并行化的损失吃掉”的结论，**只要改变实现就不再成立**。

并行性得以保持这一点可以通过 Profiler 标记确认。

表: 3 种降频实现的按线程构成 {{table:animlod_impl_threads}}

| 方式 | \`Animator.ProcessGraph\` 全线程合计 | 其中主线程 | 主线程侧的更新时间 |
| :---- | :---- | :---- | :---- |
| 每帧更新（600 个角色/帧） | 20.104 ms | 1.257 ms | \`Animators.Update\` 4.046 ms |
| \`Animator.Update()\`、1/4（150 个角色/帧） | 0.278 ms | 0.180 ms | \`ATB.ManualAnimatorUpdate\` 2.316 ms |
| \`Animator.enabled\`、1/4（150 个角色/帧） | **4.424 ms** | 0.288 ms | \`Animators.Update\` 1.104 ms ＋ 切换 0.125 ms |
| 按相位的手动图、1/4（150 个角色/帧） | **4.253 ms** | 0.279 ms | \`ATB.BucketEvaluate\` 1.278 ms |

在新的 2 种实现中，**\`Animator.ProcessGraph\` 的工作量恰好是每帧更新的 1/4**（每帧只评估 150 个角色，这是理所当然的），而且这些工作仍然分散在工作线程上。而 \`Animator.Update()\` 版在此标记中几乎没有计入，工作的主体位于主线程的 \`ATB.ManualAnimatorUpdate\` 之中。

按每个角色的主线程开销排列，差异就很明显。

* 每帧更新：4.046 ms / 600 个角色 \= **6.7 µs/角色**

* 切换 \`Animator.enabled\`、1/4：1.229 ms / 150 个角色 \= **8.2 µs/角色**

* 按相位的手动图、1/4：1.278 ms / 150 个角色 \= **8.5 µs/角色**

* \`Animator.Update()\`、1/4：2.316 ms / 150 个角色 \= **15.4 µs/角色**

新的 2 种实现中每个角色的开销基本与每帧更新相同（差异是切换和 \`Evaluate\` 调用的开销）。因此降频倍率会直接接近削减率。

也确认了播放速度得以保持。测量最后生成的角色（相位槽最大的角色）指尖骨骼每秒移动的距离，每帧更新为 0.509 m/s，相比之下切换 \`Animator.enabled\` 为 0.496 m/s，按相位的手动图为 0.517 m/s。差异在降频导致动作被直线近似的误差范围内，两者速度都没有变化。

关于分布有 1 点需要注意。1/2 降频的手动图版 p99 为 9.102 ms、标准差 1.086，比切换 \`enabled\` 版（p99 4.100 ms、标准差 0.562）尾部更重。1/4 时则相反，手动图版更稳定（标准差 0.306 对 0.340）。行为会随图的数量与每个图的角色数之间的平衡而变化，因此若要采用，请不要只看平均值，也要看 p99。

选择哪一种取决于需求。如果想继续沿用 Animator 的状态机和 Inspector 工作流，切换 \`Animator.enabled\` 要简便得多（几行代码即可完成，对现有代码几乎没有影响）。如果需要推进方式的精确性，或需要插入 Animation Job 这类图侧的控制，则选择按相位的手动图。**无论哪种情况，都没有理由选择自己调用 \`Animator.Update()\` 的实现。**

## **本章结论**

* 动画 LOD 应以“更新频率的降频 \+ 错开相位”为一套来设计

* 错开相位不会减少总开销（平均 2.613 对 2.578 ms）。它减少的是**帧时间的波动**（标准差 3.863 → 0.185），这是面向体感品质的措施

* **不要选择自己调用 \`Animator.Update()\` 的实现。**由于当场同步评估，会失去 Job 并行化，每个角色的主线程开销从 6.7 → 15.4 µs，约为 2.3 倍

* 取而代之的是**每帧选择参与 Unity 批量更新的角色**。采用切换 \`Animator.enabled\`（需要通过 \`keepAnimatorStateOnDisable \= true\` 和 \`Animator.speed\` 补偿），或者按相位拆分手动图并挂载 \`AnimatorControllerPlayable\` 的形式。每个角色控制在 8.2〜8.5 µs，并行化也得以保留

* 改变实现后**即使 2 倍降频也有效**：−4.1 % 变成了 −33.5 % / −38.8 %。4 倍时为 −63.6 % / −69.4 %

* 以角色为单位的降频无法用 1 个集中图表达。不过“按相位的数量拆分图”就能兼顾并行化和更新频率控制

* 不要只凭中位数判断。总开销看平均值，体感看 p95、p99 和标准差

{{pagebreak}}

# **总结**

以下是各章的实测结果。

表: 各章实测结果汇总 {{table:overall_summary}}

| 章 | 措施 | 条件 | 结果 |
| :---- | :---- | :---- | :---- |
| 资源的设置 | Optimize Game Objects | 600 个角色、1 个 Renderer | 6.797 → 5.991 ms（**−11.9 %**） |
| 资源的设置 | Optimize Hierarchy（运行时） | 600 个角色、1 个 Renderer | 6.797 → 6.009 ms（**−11.6 %**） |
| 资源的设置 | 上述 2 种方法 | 300 个角色、全部 Renderer | 18.205 → 18.3〜18.5 ms（无改善、受绘制制约） |
| Job Thread | 把手动更新图集中为 1 个 | 600 个角色 | 8.396 → 4.500 ms（**−46.4 %**） |
| Animation C\# Jobs | 把 Two-Bone IK 改为 Animation Job | 300 个角色、求解 64 次 | 9.094 → 2.795 ms（**−69.3 %**） |
| Animation C\# Jobs | 进一步用 Burst 编译 | 300 个角色、求解 64 次 | 2.795 → 2.792 ms（帧时间无差异） |
| 动画 LOD | 1/4 降频、显式调用 \`Animator.Update()\` | 600 个角色 | 平均 5.037 → 2.578 ms（**−48.8 %**） |
| 动画 LOD | 1/4 降频、按相位的手动图 | 600 个角色 | 平均 5.037 → 1.541 ms（**−69.4 %**） |
| 动画 LOD | 1/2 降频、显式调用 \`Animator.Update()\` | 600 个角色 | 平均 5.037 → 4.831 ms（−4.1 %） |
| 动画 LOD | 1/2 降频、按相位的手动图 | 600 个角色 | 平均 5.037 → 3.082 ms（**−38.8 %**） |

## **最终的优化思路**

以下是基于本文实测的大量角色动画优化的推进方式。

**1\. 优化资源设置**

首先，把模型本身的顶点数和骨骼数规范定在合理的值。理想情况是先在作为基准的支持设备上测量基准测试，制定合适的模型规范。在此基础上，再考虑在 Unity 侧应用 Optimize Game Objects。在 3D 项目中，位于上游的模型制作很难在量产已经推进的项目后期进行大幅修改，因此最好在项目初期完成。

**2\. 提高 Animation Job Thread 上处理的并行性**

 Animator 和自动更新的 Playable Graph 会由 Unity 自动并行化，而手动更新只以图为单位并行化。在设计角色控制时，就要把“能否有效利用 Animation Job Thread”纳入考量。

**3\. 进行包括对 Animation Job 应用 Burst 在内的优化**

 IK 等后处理首先从主线程移到 Animation Job。

如果还需要更高的性能，就采用能够应用 SIMD 化的数据与处理设计。

**4\. 采用动画 LOD 等应用层面的手段**

在资源、引擎设置、代码优化都用尽之后，最后的手段是“从一开始就不处理动画”。

这需要与项目需求协商，但请在探索游戏体验可接受的界限的同时，考虑抑制处理量的机制。

{{pagebreak}}

# **脚注**

## **Unity 官方文档**

* \[Playable API\](https://docs.unity3d.com/Manual/Playables.html) — Playable Graph 的构建与 \`DirectorUpdateMode\`

* \[\`AnimatorUtility.OptimizeTransformHierarchy\`\](https://docs.unity3d.com/ScriptReference/AnimatorUtility.OptimizeTransformHierarchy.html) — 运行时的层级扁平化

* \[Model Import Settings: Rig（Optimize Game Objects）\](https://docs.unity3d.com/Manual/FBXImporter-Rig.html)

* \[\`IAnimationJob\`\](https://docs.unity3d.com/ScriptReference/Animations.IAnimationJob.html) — 插入 \`AnimationScriptPlayable\` 的 Job

* \[\`AnimationScriptPlayable\`\](https://docs.unity3d.com/ScriptReference/Animations.AnimationScriptPlayable.html)

* \[C\# Job System\](https://docs.unity3d.com/Manual/JobSystem.html) — 不经由 \`AnimationStream\` 编写并行处理的机制

## **使用的资源**

* \[Unity 酱 3D 模型数据\](https://unity-chan.com/) — © Unity Technologies Japan/UCL

[image1]: assets/saito-shunsuke-1/media/image1.png

[image2]: assets/saito-shunsuke-1/media/image2.png

[image3]: assets/saito-shunsuke-1/media/image3.png

[image4]: assets/saito-shunsuke-1/media/image4.png

[image5]: assets/saito-shunsuke-1/media/image5.png

[image6]: assets/saito-shunsuke-1/media/image6.png

[image7]: assets/saito-shunsuke-1/media/image7.png

[image8]: assets/saito-shunsuke-1/media/image8.png

[image9]: assets/saito-shunsuke-1/media/image9.png

[image10]: assets/saito-shunsuke-1/media/image10.png

[image11]: assets/saito-shunsuke-1/media/image11.png