# **UI 系统的选用 — IMGUI / uGUI / UI Toolkit**

姓名 / 罗马字：Zaitsu Hiromu  
所属：株式会社 Sumzap  
X 账号 / 网站 / GitHub 账号（可选）：@RomGameclient

前作《Unity 性能调优宝典》{{fn:tuning_bible}}讨论了 Canvas 拆分、Raycast Target 调整等针对 uGUI 开销的应对措施。本章往上游追溯一步，讨论 UI 系统的选择本身。因为选择哪一个，决定了 UI 的开销结构以及可采取的应对措施种类。  
Unity 有 IMGUI、uGUI、UI Toolkit 三套 UI 系统。本章依次讨论这三者，梳理各自目前在运行时和编辑器扩展中的定位。本章内容基于 Unity 6.5 时点的信息。

# **IMGUI**

运行时的 IMGUI 已不在游戏内 UI 的选项之列。官方手册明确指出它并非面向玩家操作的常规游戏内 UI，其预期用途是游戏内的调试显示与工具、自定义 Inspector 以及编辑器扩展。在调试叠加层和运行时日志查看器等用途上，目前仍在使用。  
另一方面，在编辑器扩展中 IMGUI 仍是官方承认的现役方案。官方的 UI 系统对比页面{{fn:ui_system_compare}}将 IMGUI 列为替代方案，并指出两点优势：可以不受限制地访问编辑器扩展功能，以及作为轻量 API 能够快速在屏幕上绘制 UI。  
官方的 IMGUI 迁移指南{{fn:imgui_migration}}明确列出了在 UI Toolkit 中没有等价物的 API。对实际工作影响较大的是 InspectorTitlebar()、Toolbar()、IntPopup()、LinkButton() 等。这些都是用于让编辑器窗口外观贴近 Unity 原生编辑器的 API，在需要它们的场合，目前仍要使用 IMGUI。  
不过，作为编辑器扩展使用时，有一些开销特性需要了解。IMGUI 是在 OnGUI() 中每次都重新描述 UI 的即时模式系统，它不保留 UI 树的信息，每次事件都会反复构建和销毁。它与保留模式的 UI Toolkit 的区别如 {{fig:render_cycle}} 所示。OnGUI() 每帧会执行多次，每次都容易产生堆分配。在持续重绘的编辑器窗口中，这会成为 GC 的开销。  
![][image1]  
図: 即时模式与保留模式的绘制周期 {{fig:render_cycle}}

# **uGUI**

这是基于 Canvas 和 GameObject 的系统。每个元素都是一个 GameObject，并拥有 Transform 层级。它仅用于运行时，不能用于编辑器扩展。  
在运行时它是事实上的标准。优势有三点。  
・可以直接在 Scene 视图中摆放，并当场预览  
・可以用 Animator 和 Timeline 驱动 UI  
・Asset Store 上有大量以 uGUI 为前提的资源包  
在开销方面，元素发生变更的 Canvas 会重新构建网格，Transform 层级的更新和 Raycast Target 的遍历开销也与元素数量成正比。前作讨论的 Canvas 拆分和 Raycast Target 调整，都是针对这一结构所导致的开销的代表性应对措施。

# **UI Toolkit**

这是一种保留模式系统：先构建一次 VisualElement 树，之后只更新发生变化的部分。逻辑用 C\# 编写，UI 结构用 UXML、样式用 USS 分别描述。它既可用于运行时，也可用于编辑器扩展，在编辑器扩展中是官方的首选推荐。不过，官方的措辞是“如果要制作复杂的编辑器工具”。如果只是排列几个字段的工具，不引入 UXML 和 USS、只用 C\# 编写也不会有问题。  
在开销方面，与 uGUI 的差异直接体现为应对措施的差异。uGUI 中改动一个元素就会重建整个 Canvas 的网格，而 UI Toolkit 只更新变化的部分，因此不需要相当于 Canvas 拆分的措施。不过元素数量增加时，布局计算的开销同样会增加，所以减少元素数量的做法仍然必要。在 Unity 6 系列中，事件分发的加速、几何体生成的 Job 化、布局计算缓存的改进等性能改进也在持续进行。

## **与 uGUI 相比的不足之处**

截至 Unity 6.5，尚无法实现的功能有以下几项。这些都是 Unity 声明正在开发中的项目{{fn:uitk_status}}，但如果现在就要决定是否采用，需要按照无法实现的前提来考虑。  
・在 Animation 窗口中处理 VisualElement 关键帧动画的功能  
・在 Scene 视图中编辑 UI  
・从 MonoBehaviour 脚本引用 UXML 中定义的 VisualElement 的机制  
・面向自定义着色器的顶点数据支持  
生态系统也依然薄弱。Asset Store 上的资源大多以 GameObject 层级为前提，无法直接使用。TextMeshPro 也无法在 UI Toolkit 中使用，UI Toolkit 使用专用的文本系统（在 6.5 中成为默认的 Advanced Text Generator）。DOTween 虽然也添加了面向 VisualElement 的快捷方法，但仅支持 Unity 6.3 及以后版本{{fn:dotween}}。  
从 uGUI 迁移时，锚点和轴心没有直接的等价物。UI Toolkit 的布局方式是将子元素依次流入父元素中排列。与为每个元素指定位置和尺寸的 uGUI 相比，组装方式本身发生了变化。

# **官方推荐的变迁**

官方的 UI 系统对比页面按手册版本分别公开，对照阅读会发现运行时 UI 的推荐理由已有所调换{{fn:ui_system_compare_60}}。具体内容见 {{table:recommendation_shift}}。  
表: 运行时 UI 推荐理由的变化 {{table:recommendation_shift}}

| 需求、用途 | Unity 2022.3〜6.1 | Unity 6.5 |
| :---- | :---- | :---- |
| 处理大量 UI | UI Toolkit | UI Toolkit |
| 需要面向美术和设计师的制作工作流 | UI Toolkit | UI Toolkit |
| 无纹理的 UI 绘制 | UI Toolkit | UI Toolkit |
| 放置在 3D 空间中并接受光照的 UI | uGUI | UI Toolkit |
| 通过自定义着色器和材质实现的表现 | uGUI | UI Toolkit |
| 易于从 MonoBehaviour 引用 | uGUI | uGUI |

从 2022.3 到 6.1 一直归于 uGUI 一侧的两项{{fn:ui_system_compare_2022}}，随着世界空间 UI 在 6.2、自定义着色器在 6.3 中面向 URP 实现，已转移到 UI Toolkit 一侧。剩下的“易于从 MonoBehaviour 引用”，其引用 UXML 中定义的元素的机制也已列入开发中的项目。不过，对比页面的结构本身在 Unity 6.5 中并未改变。运行时 UI 仍以 uGUI 作为默认方案介绍，并将 UI Toolkit 作为其替代方案并列。调换的只是推荐的理由，以哪一个为默认并没有变动。同一页面上按用途划分的列表，其标题也从 2022.3 到 6.2 的“Recommendation（推荐）”改为 6.3 以后的“Often used for（常用于）”。Unity 自身已不再将其称为推荐。

# **并用多个 UI 系统时的限制**

在一个项目中使用多个 UI 系统本身是可行的，但组合使用时会出现一些问题。下面分运行时和编辑器扩展两方面来看。

## **运行时**

uGUI 和 UI Toolkit 可以在同一项目中使用，但不能将一方嵌入另一方之中。导航和样式的不一致，官方也列为限制。  
绘制顺序只能在各自的系统内部控制（{{fig:sort_order}}）。要用 UI Toolkit 将 UI 显示到屏幕上，需要给 GameObject 挂载 Panel Renderer 组件。在 Unity 6.4 及以前，这一职责由 UIDocument 承担，6.5 中被替换{{fn:ui_document_component}}。如果使用的是 LTS 的 6.3，则为 UIDocument。这个 Panel Renderer 是排序的单位：面板之间由 Panel Settings 的 Sort Order 决定顺序，同一面板内的 Panel Renderer 之间由 PanelRenderer.sortingOrder 决定。uGUI 则按根 Canvas 的 Sort Order 排序。没有可以跨这两者指定顺序的属性。  
从 uGUI 迁移的指南{{fn:ugui_migration}}也提到了用渲染纹理合成绘制结果的方法，但明确指出输入事件无法传递。由于不能用于需要操作的 UI，请避免将 uGUI 和 UI Toolkit 叠加使用的设计。  
![][image2]  
図: UI Toolkit 与 uGUI 绘制顺序的决定方式 {{fig:sort_order}}

## **编辑器扩展**

在编辑器扩展中，两个系统能够嵌套的方向只限于一个方向（{{fig:propertydrawer_nesting}}）。在 UI Toolkit 的树中放置 IMGUI 的绘制可以通过 IMGUIContainer 实现，但反过来，在 IMGUI 的绘制中放置 UI Toolkit 的树则无法实现。  
这会成为问题的场合是自行实现 Inspector 显示的时候。Inspector 的自定义分为两层：负责整个组件的自定义 Inspector，以及按字段替换外观的 PropertyDrawer。内层的 PropertyDrawer 无法选择自己被哪个系统的 Inspector 调用。仅用 UI Toolkit 实现的 PropertyDrawer，如果被 IMGUI 编写的自定义 Inspector 或 IMGUI 的 PropertyDrawer 调用，该字段的值栏会被替换为“No GUI Implemented”（未实现 GUI）的显示。Unity 2022.2 及以后 Inspector 的默认是 UI Toolkit，因此多数情况下不会发生，但在仍保留 IMGUI 自定义 Inspector 或 PropertyDrawer 的项目中，需要在 PropertyDrawer 一侧也准备 IMGUI 版的绘制。  
![][image3]  
図: PropertyDrawer 能否嵌套 {{fig:propertydrawer_nesting}}

# **结语**

调查之后令人印象深刻的是，官方的推荐理由在 Unity 6.5 中几乎都转移到了 UI Toolkit 一侧。运行时 uGUI 仍是标准，没有理由去迁移已在正常运作的 UI，但在新项目开发中回避 UI Toolkit 的理由确实在减少。如果犹豫不决，可以根据对 Animator 和 Timeline 动画表现以及 Asset Store 资源的依赖程度来判断。编辑器扩展方面，复杂工具用 UI Toolkit、需要贴近 Unity 原生编辑器外观的场合用 IMGUI，按这样的划分在当前阶段不会有困扰。UI 系统的选择决定了前作所讨论的应对措施是否必要，是最上游的调优。

脚注: {{fn:tuning_bible}} https://cyberagentgameentertainment.github.io/UnityPerformanceTuningBible/ja/preface.html  
脚注: {{fn:ui_system_compare}} https://docs.unity3d.com/6000.5/Documentation/Manual/UI-system-compare.html （2026 年 8 月 6 日查阅）  
脚注: {{fn:imgui_migration}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-IMGUI-migration.html  
脚注: {{fn:uitk_status}} https://discussions.unity.com/t/ui-toolkit-development-status-and-next-milestones-november-2025/1698009  
脚注: {{fn:dotween}} https://dotween.demigiant.com/download.php  
脚注: {{fn:ui_system_compare_60}} https://docs.unity3d.com/6000.0/Documentation/Manual/UI-system-compare.html （2026 年 8 月 6 日查阅）  
脚注: {{fn:ui_system_compare_2022}} https://docs.unity3d.com/2022.3/Documentation/Manual/UI-system-compare.html （2026 年 8 月 6 日查阅）  
脚注: {{fn:ui_document_component}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-create-ui-document-component.html  
脚注: {{fn:ugui_migration}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-Transitioning-From-UGUI.html

[image1]: assets/zaitsu-hiromu-2/media/image1.png

[image2]: assets/zaitsu-hiromu-2/media/image2.png

[image3]: assets/zaitsu-hiromu-2/media/image3.png