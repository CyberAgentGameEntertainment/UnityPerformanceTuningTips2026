=[nonum] 2026 年 Unity 图形动向
#@# 原稿は sources/editorial-intro-1.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

==[nonum] Render Graph 的正式发布

Unity 6 正式发布以来，截至撰写本文时已过去将近 2 年。Unity 6 发布的一大亮点是 Render Graph 的登场。Render Graph 是基于以下思路的机制：以声明方式描述 1 帧中的资源依赖关系，并在执行前对整个图进行优化。这种思路已被许多现代渲染引擎采用，例如 Frostbite（DICE）在 2017 年发表的 Frame Graph 和 Unreal Engine 的 RDG（Render Dependency Graph）。

通过描述资源依赖，可以获得渲染纹理的复用、不需要的渲染纹理的销毁、内存加载/存储操作的自动适当设置等各种好处。

在 Unity 中，它曾先于 URP 在 HDRP 中引入，URP 一直没有，而随着 Unity 6 的发布终于迎来了期待已久的版本。

本书第 1 部第 1 章“Unity 6 Render Graph 图块内存优化”将解读为了充分发挥 Render Graph 的能力所必需的 TBDR 架构 GPU 与 Render Graph 之间的关系。

接下来的第 1 部第 2 章“应使用 RenderGraph UnsafePass 的场景”讲解了 Render Graph 优化效果较弱的 Unsafe Pass。Render Graph 通常通过使用 Raster Render Pass 或 Compute Pass 来获得 Render Graph 的优化收益。在许多场景下，选择 Raster Render Pass 或 Compute Pass 是最佳选择。本章则介绍其中应特意选择 Unsafe Pass 的场景，即为了提升性能、为了移植，以及为了实现只有 Unsafe 才能实现的功能，而恰当选用 Unsafe Pass 的情况。

==[nonum] Render Graph 的登场改变了一切吗

如上所述，Render Graph 的登场使渲染系统发生了很大变化。Render Graph 会自动进行优化，但如果不理解其机制就使用，就无法充分发挥优化的收益。理解并驾驭新的渲染系统，是我们开发者的责任。

然而，Render Graph 的登场并没有推翻以往所有的优化概念。Render Graph 在图形领域属于较高层的内容。面向更接近硬件的较低层时，传统的知识依然有效。例如在着色器中，sin、cos、tan、pow 等超越函数的运算开销较高，这一说法在现代移动端环境中同样成立。第 1 部第 4 章“移动端着色器优化：超越函数的测量与轻量化”展示了在现代移动端环境中使用超越函数时的测量结果示例，并在此基础上介绍了几种用于超越函数轻量化的近似函数。我本人也曾怀疑，在运算性能已经提升的现代移动端环境中，是否仍需要近似函数。这一章为这样的疑问给出了一个答案。第 1 部第 5 章“利用 Renderer Shader User Value 优化图形”和第 1 部第 6 章“使用 GraphicsStateCollection 进行着色器预热”介绍的功能，同样是为了解决 Unity 6 之前就遗留的问题而新增的功能。

==[nonum] 大量对象绘制的开销

Renderer Shader User Value（RSUV）是 Unity 6.3 新增的功能。该功能要解决的问题，是长期存在的绘制大量对象时的 CPU 开销，尤其是 Set Pass Call 的开销。解决这一问题的手段有 SRP Batcher 和 GPU Instancing 等。

但是，在“希望每个对象的外观略有不同”的场景下，SRP Batcher 存在限制。SRP Batcher 不支持使用 MaterialPropertyBlock 按渲染器修改属性，因此要在保持 SRP Batcher 的同时传递各自的参数，就需要按想要定制的渲染器数量复制 Material。仅为了少量参数的差异而增加 Material，无论从内存还是管理角度都不可取。

RSUV 是应对这一问题的更轻量的选项。使用 RSUV，可以让每个渲染器持有一个 32bit 无符号整数，并从着色器端引用。设置该值不会破坏批处理，因此可以在保持 SRP Batcher 的同时更改参数。虽然存在只能传递一个 32bit 值的限制，但通过将颜色、索引、标志等按位打包，可以覆盖许多用途。

==[nonum] 着色器编译时的尖峰

使用 GraphicsStateCollection 进行着色器预热所解决的，是着色器编译时的尖峰问题。GraphicsStateCollection 在 Unity 6 中作为 Experimental（实验性功能）添加，并在 Unity 6.5 中作为正式功能发布。

以往的 ShaderVariantCollection.WarmUp 等预热 API 在 Vulkan/Metal 等 API 上无法按预期工作。了解 Vulkan/Metal 等的着色器预热机制就会明白，旧 API 无法传递正确预热所需的参数，因此在 Vulkan/Metal 上无法完全预热。于是开发者为了避免游戏过程中的尖峰，不得不采用在画面变暗期间于后台进行绘制等变通性的预热做法。

GraphicsStateCollection 是解决这一问题的新 API。GraphicsStateCollection 是在应用运行时收集着色器预热所需的顶点布局等各种信息的机制。收集这些信息后，就可以完美地预热着色器。不过，GraphicsStateCollection 也存在信息收集遗漏等一些新的问题。这些与其说是 GraphicsStateCollection 本身的问题，不如说各有其因，有的源于 Unity 的 Shader Variant Strip，有的源于 Vulkan/Metal 等现代图形 API 的规范。

==[nonum] 迈向第 1 部

Render Graph 的登场使构建渲染管线的系统发生了很大变化。通过定义资源依赖可以销毁不需要的资源，这是一个与基于图块的 GPU 契合度也很高的新渲染系统。

然而，较高层的渲染系统焕然一新，并不意味着更接近硬件层的机制也发生了变化。Render Graph、GraphicsStateCollection 和 RSUV 所要解决的，都是以往就存在的问题。

第 1 部既介绍了 Unity 6 中引入的新功能和具有普遍性的内容，也介绍了第 1 部第 3 章“省略 URP 的 FinalBlitPass 的方法”这类明天就能立即使用的技巧。

希望这些内容能为各位的开发提供帮助。
