=[nonum] 2026 年脚本与动画动向
#@# 原稿は sources/editorial-intro-2.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

==[nonum] 脚本的动向

在 Unity 6 时代，脚本方面的状况变化不大。不过 Unity 7 将新增 @<b>{CoreCLR} 和 @<b>{ECS for All} 这两项会大幅改变 C# 代码性能调优前提的功能，Unity 6 时代也已为此对部分 API 做了变更。本章在介绍 Unity 6 时代 API 的同时，也介绍 Unity 7 中计划的部分变更。请注意，关于 Unity 7 的信息为撰写时的信息，可能与实际发布有所不同。

=== 异步处理

@<b>{Awaitable} 是 Unity 2023 中登场的 Unity 标准类，使得以往需要借助 @<b>{UniTask} 等外部库才能实现的高效 async / await 处理，现在只用 Unity 标准类即可编写。不过 Awaitable 的功能集比 UniTask 少，因此已在使用 UniTask 的情况下，继续使用 UniTask 也未尝不可。在无法依赖外部包的情况下，用 Awaitable 代替 Task、ValueTask 和协程，有望提升性能。此外，第 2 部第 5 章讲解了异步处理中容易出问题的 CancellationToken 管理。

=== .NET 运行时

Unity 6 时的 .NET 运行时仍与以往一样存在 Mono 和 IL2CPP，可使用 .NET Standard 2.1 的 API 集和 C# 9。Unity 7 将废弃 Mono，并计划在编辑器和部分桌面播放器中引入 @<b>{CoreCLR}。CoreCLR 是 .NET 本家（.NET Core / .NET）所使用的运行时实现，在性能上领先于 Mono。移动端和主机平台在一段时期内不会引入 CoreCLR，IL2CPP 仍是首选，但除了编辑器迁移到 CoreCLR 之外，运行时可用标准 API 的增加和实现的优化等也会带来一部分收益。

==== CoreCLR 的 JIT

CoreCLR 对将 C# 代码转换为原生代码的 JIT 做了大量优化。在大多数场景下，仅仅从 Mono 迁移到 CoreCLR，C# 代码的执行速度就会提升。根据 Unity 公布的基准测试结果，在不使用 GC 的工作负载下，CoreCLR 的执行速度相比 Mono 提升 2 至 3 倍，可获得与 IL2CPP 和 Burst 相当的性能@<fn>{fn1}。此前，尤其是在可以在 Mono 和 IL2CPP 之间选择的桌面平台上，从性能角度看 IL2CPP 是最佳选择，但 CoreCLR 登场之后，这一前提也可能发生变化。

==== .NET 10 的 BCL

此前 .NET BCL@<fn>{fn2}使用的是 Mono 附带的实现，仅限于相当于 .NET Standard 2.1 的 API 集，而随着 CoreCLR 的引入，预计将更新为相当于 .NET 10 的 BCL。可用的 API 将大幅增加，内部实现也更加精炼，有望改善性能。新的 BCL 计划也可用于 IL2CPP，因此在不引入 CoreCLR 的移动端和主机平台上，同样能在性能和功能方面获益。

==== CoreCLR 的 GC / 内存管理

CoreCLR 的垃圾回收器（GC）实现也与 Mono / IL2CPP 不同。此前 Unity 的 Mono 和 IL2CPP 使用的是名为 @<b>{Boehm GC}（Boehm-Demers-Weiser GC，bdwgc）的 GC 实现，而 CoreCLR 的 GC 具有与 Boehm GC 截然不同的特点@<fn>{fn3}。

首先，CoreCLR GC 是@<b>{分代 GC（generational GC）}。分代 GC 将已分配的对象按称为“代”的分组进行管理。新分配的对象首先作为 gen0 处理。针对 gen0 的 GC 相对频繁，而在 gen0 GC 中存活下来的对象会移动到下一代 gen1，其 GC 的执行频率低于 gen0。这基于一个普遍的观察：“新分配的对象大多寿命很短，而存活了一定时间的对象之后往往也会存活很久”，目的是缩短针对年轻代而频繁执行的 GC 所需的时间。在 CoreCLR GC 的内存优化中，已分配的对象属于哪一代、针对哪一代的 GC 在何时执行等，将成为重要的关注点。

此外，CoreCLR GC 会进行@<b>{压缩（compaction）}。Boehm GC 不会移动已分配的对象，因此反复分配和释放对象后，堆会逐渐碎片化，容易出现堆比实际所需大小更大的问题。CoreCLR GC 的压缩会在碎片化的堆上移动对象，填补对象之间的空隙，从而抑制堆的增长。

另外，IL2CPP 仍然使用 Boehm GC，因此与以往相同的内存优化策略依然有效@<fn>{fn4}。

除 GC 之外，CoreCLR 还通过 JIT 积极进行内存分配的优化。例如 @<b>{Object Stack Allocation} 是 JIT 的一项功能，它将完全在方法内使用的极短寿命对象分配在栈上而非堆上。尤其是循环内部等热点路径中大量分配的短寿命对象，引发的问题可能不会像在 Mono 或 IL2CPP 中那样严重。

==== C# 14 支持

Unity 6 时官方支持的是 C# 9，而 Unity 7 计划可使用 C# 14。近年的 C# 版本新增了许多用于改善性能的功能，从性能调优角度需要了解的语言特性也会随之增加。

==== Unity 6 时代提前引入的功能

CoreCLR 计划在 Unity 7 中正式引入，但部分平台已在 Unity 6 系列中开始实验性引入 CoreCLR。在撰写时最新的 Unity 6.7 alpha（6000.7.0a6）中，Windows、macOS、Linux 的独立播放器可使用 CoreCLR。不过 API 集仍限于 .NET Standard 2.1。同样在 Unity 6.7 alpha 中，iOS 播放器（IL2CPP）已可使用相当于 .NET 10 的 BCL 实现，但同样限于 .NET Standard 2.1。现阶段应认为它只能用于为迁移到 Unity 7 的 CoreCLR 而进行的性能和兼容性验证。

=== DOTS

在 Unity 6 中，DOTS（Data-Oriented Technology Stack）功能群依然有效。可以使用借助 LLVM 将 C# 代码转换为高度优化的原生代码的 @<b>{Burst}、安全地对代码进行多线程化的 @<b>{C# Job System}，以及利用它们以比 MonoBehaviour 更快的速度更新游戏状态的机制 @<b>{Entity Component System（ECS）}。

在 Unity 7 中，预计将通过名为 @<b>{ECS for All} 的功能群加强 ECS 与 GameObject 的协作。此前，作为在 ECS 中存放组件的容器的 Entity 与 GameObject 不兼容，ECS 的状态完全独立于 GameObject 进行管理。在 Unity 7 中，GameObject 也将能作为 Entity 工作，可以对 GameObject 附加 ECS 的组件。ECS 与 GameObject 的混用与协作将比以往更容易，因此更容易把容易成为瓶颈的 MonoBehaviour 的 Update 处理群迁移到 ECS，有助于减轻主线程开销并提高帧率。

#@# この節の最後の1行だけが次のページにあふれるため、このページを1行延ばす。
//embed[latex]{
\enlargethispage{\baselineskip}
//}

==[nonum] 动画的动向

Unity 6 时代对动画的性能做了若干改进。Unity 6.3 为 Animator 和 AnimationPlayableGraph 添加了 @<b>{ResetControllerState} 方法，使 Animator 可以轻松池化。详情见第 2 部第 2 章。Unity 6.5 添加了 @<b>{AnimationEventInfo} 结构体，使得接收 Animation Event 时可以避免 AnimationEvent 类造成的堆内存分配。

//footnote[fn1][Path to CoreCLR | GDC 2026 - Unity - YouTube @<href>{https://youtu.be/_t6xVfrmEWU?si=edILni6YnObyWkoj&t=218}]
//footnote[fn2][Base Class Library。指 System 命名空间等标准类库的实现]
//footnote[fn3][Fundamentals of garbage collection - .NET | Microsoft Learn - @<href>{https://learn.microsoft.com/en-us/dotnet/standard/garbage-collection/fundamentals}]
//footnote[fn4][Understanding Memory Management in Unity - Unity Discussion - @<href>{https://discussions.unity.com/t/understanding-memory-management-in-unity/1700107}]
