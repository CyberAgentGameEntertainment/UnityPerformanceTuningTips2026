# **运营阶段的资源膨胀与基于 Project Auditor 的调查**

姓名 / 罗马字：Zaitsu Hiromu  
所属：株式会社 Sumzap  
X 账号 / 网站 / GitHub 账号（可选）：@RomGameclient

在持续运营的项目中，应用的包体大小和内存占用会逐渐增加。本章讨论的是：即使团队知道对策，膨胀仍会复发的原因、如何制止，以及找出持续增长的资源的手段。本章不涉及单个资源的轻量化本身。

# **运营阶段膨胀加剧的原因**

膨胀是在日常运营中一点点累积起来的。为季节活动追加的高分辨率 Banner、限时角色的语音与 Cut-in 演出、联动企划中接收的外部制作素材、宣传结束后仍然残留的活动用资源。每一项单独看都在可接受范围内，但随着版本不断发布，就会累积成不可忽视的量。而且累积得越多，重新审视的成本越高，整理的优先级也就随之下降。  
原因与其说在技术，不如说在运营规则的松动。在运营阶段，以下 4 种情况会反复出现。  
・规范过时：设备或需求发生变化，规范与实际脱节后，没有人去重写规范。以“仅此一次”为由放行的例外不断累积。  
・检查松懈：临近截止日期时省略对交付素材的检查，负责人更替时交接也不彻底。  
・临时处理长期残留：为赶进度放入了未压缩纹理或单独导出的 Sprite，之后却没有进行还原作业。  
・判断标准没有记录：设置的依据没有留在文档中，负责人更换后无法再现相同的标准。  
这些都不是仅靠引入工具就能解决的。

# **让规范持续运转**

确定要达成的指标、选定保证运行的设备、在开销最大的场景中验证。规范本身的制定方法，前作《Unity 性能调优宝典》第 1 章已有介绍。在运营阶段会成为问题的，是规范确定之后。  
规范从制定之时起就会不断与实际脱节。因此，要事先把保证运行设备的变更、Unity 或渲染管线的更新、测量方法的变更等时间点，定为重新审视规范的契机。尤其是测量方法一旦改变，同一资源的数值也会变化，所以测量方法也要与规范一起保留下来。  
规范中的数值，要做成在导出、导入或自动构建中的某个环节运行检查的形式。是拦下还是放行按条目逐项决定；即使放行，也要把实测值作为构建日志或报告保留在全体成员都能看到的地方。只在本人屏幕上弹出警告的形式会被忽略掉。  
修正超出规范的资源，并不是改完数值就结束了。如果更改压缩设置，需要确认外观是否仍在可接受范围内；如果重组 Bundle，需要确认引用是否断开，而且要在与正式环境相同的条件下确认。正如前作所警告的，量产后的返工是一个包含发包、质量检查直至调试的完整工序。理想的做法不是攒在一起修，而是每次发布时都进行测量，并逐次修正增加的部分。

# **膨胀的模式分类**

如果按素材种类归纳对策，虽然能知道哪些素材较重，却无法知道它为什么变重。原因若保留下来，同样的膨胀就会再次发生。因此，这里跨越素材种类，按原因分为 6 类。  
表: 膨胀的 6 种模式 {{table:bloat_taxonomy}}

| 模式 | 发生了什么 |
| :---- | :---- |
| 压缩设置错误 | 导入设置保持未压缩或默认值不变，占用了所需数倍的空间 |
| 分辨率过高 | 导入时未指定最大分辨率，占用随面积成比例增加 |
| 多余的附带数据 | 启用了不使用的辅助数据并一同打包，在内存中也持续保留 |
| 格式选择错误 | 选择了不适合用途或平台的格式，要么多占用空间，要么在运行时被解压而消耗内存 |
| 打包方式不匹配 | Sprite Atlas 或 Bundle 的打包方式与实际使用单位不一致，例如显示 1 张 Sprite 就要读入整个 Atlas，把不使用的内容也一并加载 |
| Bundle 间的重复 | 共享资源未登记便被引用，于是按引用方 Bundle 的数量重复打包 |

前 4 种可以在资源导入时判定，而后 2 种取决于与其他资源的关系，在该时间点无法判定。这一区别正是下一节将对策分为两个阶段的理由。

# **导入时自动应用与构建前检查**

要制止复发，需要在资源导入时应用正确的设置，然后在构建前找出漏网的资源。  
自动应用的手段有两种。如果要根据条件改变数值，使用可在导入前后插入处理的 AssetPostprocessor。如果只是固定每种类型的初始值，用 Preset 就足够了。  
不过，仅靠导入时应用仍会有漏网之鱼。连同 .meta 文件一起引入的资源会保留原有设置进入项目，而且上一节的后 2 种在这一阶段无法判定。  
因此，要在构建前或自动构建阶段检查是否混入了违反规范的资源。只要把纹理最大分辨率、网格多边形数等限制规范写成测试，编辑器和自动构建就能使用相同的标准。像 AssetRegulationManager 这样面向此用途的 OSS 也已公开{{fn:asset_regulation_manager}}。  
不把这类检查做成机制，而只是把规范写进文档并周知的运营方式，会直接导致开头提到的规范过时和判断标准未留存的状态。制定规则是人的工作，但让规则得到遵守的部分请交给机器。

# **Bundle 间重复产生的机制**

6 种模式中最难察觉的是 Bundle 间的重复。因为原因不在单个资源的设置，而在资源之间的引用关系。Addressables 以 Group 为单位管理资源，构建时按 Group 打包为各自的 Bundle。此时，未登记为 Addressable 的资源会被视为隐式依赖，并被复制到每一个引用它的 Bundle 中。  
![][image1]  
図: 未登记的共享资源被复制（上）与登记到专用 Group 后的情况（下） {{fig:dup_mechanism}}  
在图中的示例里，仅仅是 2 个 Prefab 引用同一张 4MB 的纹理，Bundle 总大小就达到 10MB。把这张纹理登记到专用 Group、使其成为显式依赖后，重复即被消除，总大小降至 6MB。引用方越多，复制的份数就越多；若同时加载，内存中也会展开同样的份数，因此放任不管的时间越久，影响越大。另外，实际的 Bundle 会经过压缩，这些数值只是用于说明机制的参考值。  
这类重复无法靠目视完整追踪，因此用工具来检测。对于 Addressables，可以使用 Addressables Analyze{{fn:addressables_doc}}（在 2.0 系列中一度被移除，于 2.3.1 恢复）。它也提供自动消除所检测到的重复的功能，但由于会改写 Group 配置，应用后请一直确认到构建结果。

# **调查已膨胀的资源**

规范与检查可以制止新的膨胀，但已经堆积起来的资源需要另行找出。用于资源性能分析的工具有多种，用途各不相同。  
表: 用于调查包体大小与内存的工具 {{table:tools}}

| 工具 | 能了解的内容 |
| :---- | :---- |
| Build Report / Editor Log | 构建大小按类别的明细 |
| Addressables Analyze | 检测并修正 Addressables 的 Bundle 重复依赖 |
| Project Auditor | 整个项目的导入设置与静态配置问题 |
| Memory Profiler | 运行时的内存占用与对象的引用关系 |

其中 Memory Profiler 已在前作第 3 章“性能分析工具”中介绍。本章介绍前作未曾涉及的 Project Auditor。

# **用 Project Auditor 排查**

Project Auditor 是对项目内的资源、代码和设置进行静态分析，并将有问题的地方以列表形式输出的工具。即使在还没有余力把规则落实为规范的阶段，也能借此掌握哪些地方可能成为问题。它长期处于实验性包的状态，2025 年 2 月发布了 1.0，成为正式版。以下的界面和条目是在 Unity 6.4 与 Rules 包 1.0.3 上确认的。

## **引入方法因 Unity 版本而异**

在 Unity 6.4 之前，需要从 Package Manager 添加 com.unity.project-auditor{{fn:pa_package_doc}}。6.4 及之后已随编辑器内置，无需添加{{fn:pa_manual}}，但用于判定的规则被分离到了 com.unity.project-auditor-rules，不引入该包就无法分析（{{fig:pa_rules_package}}）。  
![][image2]  
図: Rules 包。分析本体作为编辑器的模块存在 {{fig:pa_rules_package}}  
把规则从本体中分离出来后，无需升级编辑器版本也能只更新规则。

## **Assets 视图的用法**

打开 Window \> Analysis \> Project Auditor 并执行分析后，结果分为 Summary、Code、Assets、Shaders、Project、Build 等视图（视图可能随 Unity 版本增加）。调查包体大小与内存时使用的是 Assets 视图，它又分为 Asset Issues、Textures、Sprite Atlases 等 9 个。  
首先打开的是 Asset Issues（{{fig:pa_asset_issues}}）。检测到的问题按种类归纳，并显示各自发现了多少项。  
![][image3]  
図: Asset Issues 视图。在为验证准备的项目中的检测示例 {{fig:pa_asset_issues}}  
展开种类后会显示各个资源，选中后右侧会显示详情和推荐的处理方式。掌握整体情况后，再在 Textures 等单独的视图中查看其明细（{{fig:pa_textures}}）。  
![][image4]  
図: Textures 视图。设置与结果大小并排显示 {{fig:pa_textures}}  
这个视图的优点在于设置与结果显示在同一行。同样是 2048×2048 的纹理，Compressed 的行占 2.7MB，Uncompressed 的行占 16.0MB。仅 1 处设置的差异就会带来近 6 倍的差距，这一点无需打开 Inspector 就能看出。将 Size 列降序排列，从上往下确认 Compression、Resolution、Format，就能找出压缩设置错误、分辨率过高和格式选择错误。

## **减少检测结果**

在运营中的项目上执行分析，检测结果可能超过数千项。在这种状态下想从上往下逐个解决是推进不了的。请用前面{{fig:pa_asset_issues}}中的 Filters 缩小显示范围。勾选 Only Major/Critical 后只保留严重程度高的条目，在 Areas 中选择目标领域则可以只显示与包体大小相关的问题。  
检测到的问题会标注严重程度。文档中说明为 Major、Moderate、Minor、Ignored 4 种，但在定义上还存在 Critical。Ignored 并不是严重程度的等级，而是指被指定为忽略的状态。处理从 Major 开始依次进行，如果检测到 Critical 则最优先处理。静态分析也会指出有意为之的设置，因此逐项确认后再修改比较安全。  
对判断为误报的问题标记 Ignore。Ignore 是记录在当前打开的报告中的状态，不会写入设置文件，重新分析后就会失效。检测阈值等判定参数保存在 ProjectSettings/ProjectAuditorSettings.asset 中，提交后即可让全员保持一致。如果想长期抑制某些问题，可在自动构建侧的脚本中向 AnalysisParams.WithAdditionalDiagnosticRules 传入禁用规则。

## **集成到自动构建**

Project Auditor 也可以从命令行执行{{fn:pa_cli}}。集成到自动构建并按每次发布保留检测数量，就能追踪是在哪个版本增加的。  
Preferences 中有在构建后将检测数量输出到日志的设置，以及将检测视为错误的设置。在自己编写失败判定之前，请先确认这些是否已足够。另一方面，由于未提供比较两份已保存报告的 API，要获取与上次的差异需要自行实现处理。  
另外，也有构建后的自动分析无法完成而一直停住、分析过程中耗尽内存等报告{{fn:pa_github_issues}}。请设置超时，避免分析失败导致整个构建停止。

# **结语**

膨胀对策这一主题中，比起为单个资源减重的技术，建立团队层面防止复发的机制所占的比重更大。规范在运营中逐渐松动这件事本身，在任何项目中都会发生。正因如此，我认为把人的时间用在制定标准和重新审视上，把让规则得到遵守的部分交给机器，这种形式终究是最能长久持续的。  
Project Auditor 虽然是刚成为正式版的工具，但作为尚无余力制定规范的项目掌握现状的入口，已经足够实用。请先执行一次分析，从观察自己项目的倾向开始。

脚注: {{fn:asset_regulation_manager}} https://github.com/CyberAgentGameEntertainment/AssetRegulationManager/blob/main/README\_JA.md  
脚注: {{fn:pa_package_doc}} https://docs.unity3d.com/Packages/com.unity.project-auditor@1.0/manual/index.html  
脚注: {{fn:pa_manual}} https://docs.unity3d.com/6000.4/Documentation/Manual/project-auditor/project-auditor-introduction.html  
脚注: {{fn:pa_cli}} https://docs.unity3d.com/6000.4/Documentation/Manual/project-auditor/run-from-command-line.html  
脚注: {{fn:pa_github_issues}} https://github.com/Unity-Technologies/ProjectAuditor/issues  
脚注: {{fn:addressables_doc}} https://docs.unity3d.com/Packages/com.unity.addressables@latest

[image1]: assets/zaitsu-hiromu-1/media/image1.png

[image2]: assets/zaitsu-hiromu-1/media/image2.png

[image3]: assets/zaitsu-hiromu-1/media/image3.png

[image4]: assets/zaitsu-hiromu-1/media/image4.png