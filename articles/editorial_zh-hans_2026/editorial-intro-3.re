=[nonum] 2026 年资源、设置与构建动向
#@# 原稿は sources/editorial-intro-3.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

性能调优不仅仅是修改自己的代码和资源。即使代码和资源相同，仅凭引擎版本或某一项设置，构建时间、应用体积、内存使用量和运行速度也会发生变化。是否仍在用自己的机制承担引擎侧已经解决的问题，是否在不了解新增设置的情况下一直使用默认值。为了确认这些，持续关注引擎自身的变化，也是调优的前提。

对 Unity 而言，性能同样是引擎的高优先级领域。Unity 6 时代每次发布的 What's New 中，除新功能外，每次都会列出构建时间的缩短、播放器体积的削减和内存管理的改进。第 3 部涉及的资源、设置与构建，正是这些变化尤其显著的领域。

在撰写本文的 2026 年 9 月，长期支持版的最新版本是 Unity 6.3 LTS，正式版的最新版本是 Unity 6.6。Unity 6.7 作为预定成为 LTS 的版本已发布 Beta 版@<fn>{fn1}，Unity 7 计划在 2026 年 12 月发布初期 Beta 版，并在 2027 年第 1 季度发布正式版@<fn>{fn2}。下文按发布顺序梳理 Unity 6.0 到 6.7 的变化，同一版本内按效果从大到小的顺序介绍。第 3 部各章详细涉及的变化只列出要点，并标明对应章节。另外，正文中提到的 Unity 版本和默认值均为撰写时的情况，可能在之后的版本中发生变化，敬请谅解。

==[nonum] Unity 6.0

=== 通过 API 设置 iOS 的内存上限

Increased Memory Limit 和 Extended Virtual Addressing 是与应用可使用的内存上限本身相关的 iOS entitlement。在以往的 Unity 中，也可以通过手动编辑导出的 Xcode 项目的 Entitlements 文件，或在构建后处理中改写来进行设置。在 Unity 6000.0.11f1 及之后，@<tt>{ProjectCapabilityManager} 中新增了专用 API，可以一并完成向 Entitlements 的添加和向 Xcode 项目的反映。各自的作用以及在真机上确认上限的方法，见第 3 部第 6 章“iOS 的内存上限与虚拟地址空间”。

=== 通过 Build Profiles 按配置保存设置

Unity 6.0 中引入的 Build Profiles 是可以为一个平台持有多个构建配置的机制。自引入时起，每个配置文件就可以持有场景列表、Player Settings 的覆盖，以及脚本 Define Symbols 的追加。从 Unity 6.1 开始，可以按配置文件覆盖 Graphics 设置和 Quality 设置@<fn>{fn3}，6.3 中变为选择要覆盖的项目并添加的形式。在 6.5 中，还可以通过 @<tt>{BuildProfile.CreateBuildProfile} 从脚本创建配置文件。

性能方面的优点在于，可以把各平台和各发布渠道的设置差异固定为配置文件，而不必用在构建前改写设置的脚本。例如分开 Define Symbols，就可以只在发布用的配置文件中排除调试用的测量和日志代码；覆盖 Quality 设置，则可以在低端机配置中把构建所包含的画质等级本身精简掉。第 3 部第 3 章“按设备切换绘制设置”比较了在运行时根据设备性能分配绘制开销的设计，包括切换 Quality Level 的形式和直接改写 Render Pipeline Asset 值的形式。Build Profiles 可作为在构建时切换的手段与之组合使用。

=== Managed Stripping Level 的 Low 被弃用

在 Unity 6.0 的手册中，Managed Stripping Level 的 Low 被列为将来的废弃对象，并建议使用 Minimal 或 Medium@<fn>{fn4}。使用 Low 的项目需要选择提升到 Medium 以缩小二进制，或者降到 Minimal。提升到 Medium 后，通过反射引用的代码更容易被裁剪，其应对方法见第 3 部第 4 章“Unity Managed Code Stripping 实践指南”。

==[nonum] Unity 6.1

=== 按设备选择 Graphics Jobs 的模式

Unity 6.1 新增的 Vulkan Device Filter Asset 可以按 Android 设备指定 Graphics Jobs 的模式。可以在不改变其他设备设置的情况下，避开仅在特定 GPU 或驱动上变慢或不稳定的设置。

==[nonum] Unity 6.2

=== 导入时自动生成的 Mesh LOD

Unity 6.2 新增的 Mesh LOD 是一项功能：在模型的导入设置中启用 Generate Mesh LODs，即可在导入时自动生成 LOD@<fn>{fn5}。即使是没有准备 LOD 的模型，也只需一项设置就能减少远景的多边形数量，降低 GPU 开销。如果已经在使用 LOD Group，减少多边形的效果不变，差异体现在内存上。Mesh LOD 不会增加顶点，所有 LOD 共享原始网格的顶点缓冲区，只增加索引缓冲区。与为每个 LOD 持有完整网格和 GameObject 的 LOD Group 相比，可以将内存的增加控制得更小。

不过，自动生成的 LOD 的质量取决于原始网格的形状，无法期待与手工制作的 LOD 同等的质量。官方文档中也指出，它适合生物和岩石这类有机形状，而对于曲率高的形状或分离部件较多的网格，会出现纹理错位、树木叶片消失等破损示例。需要在 Inspector 中逐级确认生成的 LOD，对于出现破损的级别，可以通过 Limit LODs 停止生成，或仅将该网格改回 LOD Group，由此做出判断。

另外，能减少的只有多边形，材质不会改变。Particle System、VFX Graph、Static Batching 始终使用 LOD0，因此没有效果；对于 Skinned Mesh Renderer，无论选择了哪个 LOD，变形的计算都针对 LOD0 进行。LOD 的交叉淡化需要 GPU Resident Drawer，不建议与 LOD Group 同时使用。

==[nonum] Unity 6.3

=== 通过设置减少着色器变体

着色器变体按关键字的组合分别编译，因此关键字越多，数量呈乘法增长。减少变体后，构建时间、构建体积、着色器加载时间和运行时内存都会一并减少。

Unity 6.3 新增的 Shader Build Settings 可以在不改写着色器源码的情况下，将关键字的种类覆盖为 @<tt>{shader_feature} 或 @<tt>{dynamic_branch}，或者将特定关键字的代码从构建中排除@<fn>{fn6}。该设置可按 Build Profile 覆盖，因此可以只削减各平台上不需要的变体。排除内置关键字可能连必要的变体也一并消失，因此还需要与可以检测缺失的 Strict Shader Variant Matching 配合使用。对于保留在构建中的变体的运行时内存，可以通过 2021 LTS 起可用的 Shader Variant Loading 设置，限制所加载分块的大小和数量来抑制@<fn>{fn7}。

==[nonum] Unity 6.4

=== 编辑器内置的 Project Auditor

在 Unity 6.4 中，Project Auditor 被内置到编辑器中。它可以机械地找出 Addressables 的 Bundle 之间的重复等膨胀问题。6.4 之后诊断规则被拆分到单独的包中，因此引入步骤因版本而异。调查的步骤见第 3 部第 2 章“运营阶段的资源膨胀与使用 Project Auditor 进行调查”。

=== 禁用 Physics 模块

在 Unity 6.4 中，可以从 Package Manager 禁用内置的 Physics 模块@<fn>{fn8}。不使用 3D 物理的游戏可以相应减小构建体积。不过，使用 Local Volume 时需要保持启用。

==[nonum] Unity 6.5

=== 取消了 Read/Write 的隐式启用

启用纹理或网格的 Read/Write 后，除传输到 GPU 的数据外，CPU 侧也会保留一份副本，并占用相应的内存。此前的 Unity 在 Particle System 的 Shape 模块、Terrain 的细节以及 Mesh Collider 需要从 CPU 读取时，会在构建时隐式启用该设置。Unity 6.5 取消了纹理的这种隐式启用，Unity 6.6 取消了网格的隐式启用，如果没有在需要的资源上显式启用，Inspector 中会显示警告，并且构建会失败。

官方文档给出的这一变更的理由是缩短构建时间，但在内存方面同样有意义。哪些资源在 CPU 侧持有副本，现在可以通过资源的设置看到，因此可以逐个判断是否真的需要从 CPU 读取，为减少不必要的副本提供了线索。

=== Android 的 LTO

Unity 6.5 为 Android 添加了链接时优化（LTO）的设置。该开关只对引擎侧的库有效，并以禁用 Strip Engine Code 为前提，因此需要比较运行速度的改善与体积的增加来判断是否采用。实测结果见第 3 部第 5 章“Unity 的 LTO 设置及 Android 与 iOS 上的性能对比”。

=== 新增 mimalloc

Unity 6.5 新增了 mimalloc 作为原生内存的分配器。mimalloc 是允许多个线程同时分配和释放内存的分配器，在 6.5 中为可选项，而在 Unity 6.6 中默认启用@<fn>{fn9}。在作业和工作线程的原生内存分配相互重叠的场景下，有望减少线程间的等待。另一方面，分配器改变后，内存使用量和碎片化的趋势也会改变，官方也没有给出效果的数值。更新到 6.6 时，建议事先与更新前对比原生内存的变化。如有问题，可以从 Memory Settings 中禁用。

=== 随 iOS 热状态变化的目标帧率

在 Unity 6.5 中，可以在 Player Settings 中指定 iOS 热状态变为 Serious 或 Critical 时的目标帧率。无需自己编写发热时降低帧率的处理，就能缓和热降频导致的性能骤降。

=== 分离 AssetBundle 的 TypeTree

AssetBundle 中包含 TypeTree，即序列化数据的类型信息。有了 TypeTree，即使构建 Bundle 时与播放器的 Unity 版本不同也可以加载，但相同类型的信息会在每个 Bundle 中重复包含，从而推高体积。在 Unity 6.5 和 Addressables 2.9.1 及之后，启用 Extract TypeTree Data 后，可以将 TypeTree 拆分到单独的文件中，减小 AssetBundle 的体积@<fn>{fn10}。发布的 Bundle 越多的游戏，对下载量和设备存储空间两方面的效果越明显。不过，该设置会改变已经发布的所有 AssetBundle，因此官方建议用于新项目。

==[nonum] Unity 6.6

=== AssetBundle 的后继 Content Directories

Unity 6.6 新增了 Content Directories，作为 AssetBundle 后继的新内容构建机制@<fn>{fn11}。官方文档列出的优点有隐式的去重、加载时间的缩短和内存开销的削减。在 AssetBundle 中，被多个 Bundle 引用的资源会被复制到各个 Bundle 中，可能重复消耗体积和内存。第 3 部第 2 章“运营阶段的资源膨胀与使用 Project Auditor 进行调查”涉及的 Bundle 间的重复，在 Content Directories 中会被自动排除。资源的加载和释放通过 @<tt>{Loadable<T>} API 进行，并支持 async/await。Addressables 4.0 新增了面向 Content Directory 的 Schema，可以转换现有的 Group@<fn>{fn12}。

不过，Content Directories 目前仅支持本地内容的构建，不支持远程分发。Beta 版的 Unity 6.7 也是如此，对远程内容的支持计划在 Unity 7 时代提供@<fn>{fn13}。在 Addressables 4.0 中，如果转换具有远程加载路径的 Group，Schema 也会以无效状态被添加。对于许多通过下载来追加内容的移动端游戏，分发的内容仍将继续使用 AssetBundle 处理，可以考虑从随应用打包的内容开始迁移。另外还有一个限制：引用循环中包含的资源总计超过 64MB 时，构建会失败。

无论使用哪种机制，何时加载资源、何时释放资源的设计都同样适用。第 3 部第 1 章“AssetBundle 的生命周期管理”将加载和释放集中到基础层，并在加载时就确定资源的寿命，从而在结构上防止释放遗漏和不必要资源的常驻。

=== 按 Build Profile 指定着色器常量

在 Unity 6.6 中，可以对整个项目或按 Build Profile 指定着色器常量的值@<fn>{fn14}。仅数值因平台而异的处理，不必使用关键字增加变体就能分别编写。

=== Build History 与 Build Analysis

Unity 6.6 新增了 Build History 和 Build Analysis 窗口，播放器的构建报告会被自动累积。可以追踪每次构建的体积变化，因此更容易确定导致膨胀的变更。不过对 AssetBundle 构建的支持有限。

==[nonum] Unity 6.7（发布阶段）

以下功能是 2026 年 7 月在 Unite Seoul 上发布的，撰写时无法在文档中确认详细信息，属于发布阶段的信息@<fn>{fn15}。

=== Neural Texture Compression

Neural Texture Compression（NTC）是利用训练好的模型，几乎无损地压缩纹理的技术。发布中展示了在外观无劣化的情况下，将纹理的运行时内存削减 50% 以上、磁盘体积削减约 70% 的示例。NTC 被介绍为从 Unity 6.7 起陆续提供的 Unity Neural 的一部分。

=== Fast Build Profile

Fast Build Profile 是计划在 Unity 6.7 中新增的构建配置文件，据称可将着色器的构建时间最多缩短 90%。同时还介绍了通过 PSO 的跟踪和预热来减少运行时卡顿的机制。

==[nonum] 迈向第 3 部

回顾 Unity 6.0 到 6.7，引擎侧的变化正朝着一个方向发展：将以往靠自己的脚本和运营来弥补的事情，变成可以通过设置来实现。另一方面，这些变化并非保持默认值等待就能见效。像 mimalloc 这样只要更新就会改变行为的功能，需要通过更新前后的测量来确认。像 Content Directories 这样，根据分发需求还无法迁移的功能也存在。何时加载和释放资源、代码裁剪到什么程度、按设备切换什么，这类设计上的判断，即使引擎版本改变也依然存在。

第 3 部的各章结合实际项目中的案例，介绍做出这些判断所需的设计与测量。如果能在考虑更新到新版本时为各位提供判断依据，我们将深感荣幸。

//footnote[fn1][Unity Discussions, Unity 6.7 Beta is now available @<href>{https://discussions.unity.com/t/unity-6-7-beta-is-now-available/1736830}]
//footnote[fn2][Unity, Unity 7 roadmap revealed at Unite Seoul @<href>{https://unity.com/news/unity-7-roadmap-revealed-at-unite-seoul}]
//footnote[fn3][Unity Manual 6000.1, What's new in Unity 6.1 @<href>{https://docs.unity3d.com/6000.1/Documentation/Manual/WhatsNewUnity61.html}]
//footnote[fn4][Unity Manual 6000.0, Configure managed code stripping @<href>{https://docs.unity3d.com/6000.0/Documentation/Manual/managed-code-stripping-configure.html}]
//footnote[fn5][Unity Manual 6000.6, Introduction to Mesh LOD @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/lod/mesh-lod-introduction.html}]
//footnote[fn6][Unity Manual 6000.3, What's new in Unity 6.3 @<href>{https://docs.unity3d.com/6000.3/Documentation/Manual/WhatsNewUnity63.html}]
//footnote[fn7][Unity Blog, Improvements to shader build times and memory usage in 2021 LTS @<href>{https://unity.com/blog/engine-platform/2021-lts-improvements-to-shader-build-times-and-memory-usage}]
//footnote[fn8][Unity Manual 6000.4, What's new in Unity 6.4 @<href>{https://docs.unity3d.com/6000.4/Documentation/Manual/WhatsNewUnity64.html}]
//footnote[fn9][Unity Manual 6000.6, mimalloc @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/performance-mimalloc.html}]
//footnote[fn10][Unity Manual 6000.5, What's new in Unity 6.5 @<href>{https://docs.unity3d.com/6000.5/Documentation/Manual/WhatsNewUnity65.html}]
//footnote[fn11][Unity Manual 6000.6, Introduction to content directories @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/content-directories-introduction.html}]
//footnote[fn12][Addressables 4.0 Manual, Content build systems @<href>{https://docs.unity3d.com/Packages/com.unity.addressables@4.0/manual/content-build-systems.html}]
//footnote[fn13][Unity Blog, Content directories: Beyond the AssetBundle @<href>{https://unity.com/blog/content-directories-beyond-the-assetbundle}]
//footnote[fn14][Unity Manual 6000.6, What's new in Unity 6.6 @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/WhatsNewUnity66.html}]
//footnote[fn15][Unity Blog, Unite Seoul 2026 keynote recap @<href>{https://unity.com/blog/unite-seoul-keynote-2026-recap}]
