//raw[|latex|\reviewcompactschapterhead]
=[nonum] 前言

前作《Unity Performance Tuning Bible》@<fn>{preface_prev_book}自 2022 年出版以来获得了许多反响。它为了让任何人都能从零开始系统地掌握性能调优，除了讲解划分原因的工作流程及其前提的基础知识外，还介绍了从图形到脚本等广泛领域的调优实例。

//footnote[preface_prev_book][@<href>{https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningBible}]

时至 2026 年，前作所述的内容仍有许多部分可以直接参考。另一方面，距出版已过去 4 年，从 Unity 6 时代的到来到 AI 带来的开发体验本身的变化，各个领域都发生了巨大变化。为此，我们希望停下脚步确认性能调优的现状，汇集公司内部的专家，编写了本书《Unity 性能调优技巧 2026》。

本书从 Render Graph、GraphicsStateCollection、面向 Android 的 Link Time Optimization 等 Unity 6 中新增的可选功能，到开发现场可以立即尝试的实践性主题，结合实测值介绍广泛的内容。

调优的判断最终取决于各项目的测量结果。本书提供的是判断的依据：看哪里、与什么比较、在哪里收手。如果本书能为拿起它的读者的开发工作提供哪怕一点帮助，我们将深感荣幸。

==[nonum] 关于本书

本书以面向智能手机的应用为前提。部分讲解可能不适用于其他平台，敬请留意。

验证使用的是 Unity 6 系列（6000.x），但所用版本因章而异。除非另有说明，请参照各章中记载的版本。除涉及 Unity 6 之后新增功能的章节外，其余内容大多也适用于更早的版本。此外，本书所载数值均为各章所述测量环境下的实测值或估算值。环境不同，数值和趋势都会改变，因此请关注的不是数值本身，而是什么因素产生了怎样的效果。

==[nonum] 本书的结构

本书由 4 部 21 章构成。各章相互独立，因此只阅读所需的部分也可以。卷首的“按问题分类的反向查询指南”汇总了典型症状及对应的章节。如果已经明确了问题，从那里查起是最快的途径。“序章”则梳理了自前作以来的几年间 Unity 和移动端环境发生了什么变化。

第 1 部“图形优化”涉及绘制相关内容，包括利用 Render Graph 节省图块内存、UnsafePass 的适用场合、省略 FinalBlitPass、着色器的轻量化与预热等。

第 2 部“脚本与动画优化”介绍角色动画的调优、通过 VAT 减轻骨骼开销、ParticleSystem 的尖峰、异步处理的生命周期管理，以及 UI 系统的选用。

第 3 部“资源、设置与构建优化”介绍 AssetBundle 的生命周期管理、资源膨胀的调查、按设备切换绘制设置、Code Stripping 与 LTO，以及 iOS 的内存上限。

第 4 部“测量与自动化”介绍 iOS 标准的测量功能、将每日构建的真机性能分析自动化的架构，以及将本书的经验转换为 AI 编程智能体技能的尝试。

==[nonum] GitHub

本书的仓库@<fn>{preface_repo}已公开。我们计划随时补充和修订。此外，也可以通过 PR 或 Issue 指出需要修正之处或提出补充建议。欢迎使用。

//footnote[preface_repo][@<href>{https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026/}]
