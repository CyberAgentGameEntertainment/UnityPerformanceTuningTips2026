=[nonum] Unity Graphics Trends in 2026
#@# 原稿は sources/editorial-intro-1.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

==[nonum] The Official Release of Render Graph

Nearly two years have passed since Unity 6 was officially released, as of this writing. One of the highlights of the Unity 6 release was the arrival of Render Graph. Render Graph is a mechanism based on the idea, adopted by many modern rendering engines such as Frostbite's Frame Graph (DICE, announced in 2017) and Unreal Engine's RDG (Render Dependency Graph), of describing the resource dependencies of a single frame declaratively and optimizing the whole graph before execution.

By describing resource dependencies, you gain various benefits, such as reuse of render textures, destruction of unneeded render textures, and automatic configuration of appropriate memory load and store actions.

In Unity, it had been introduced to HDRP earlier but not to URP, and the long-awaited release for URP finally came with Unity 6.

Part 1, Chapter 1, "Unity 6 Render Graph Tile Memory Optimization", unpacks the relationship between Render Graph and GPUs with the TBDR architecture, which is necessary to get the most out of Render Graph.

Part 1, Chapter 2, "When to Use UnsafePass in Render Graph", explains Unsafe Pass, which weakens the benefits of Render Graph. Normally, Render Graph provides its optimization benefits when you use Raster Render Pass or Compute Pass, and in many cases choosing Raster Render Pass or Compute Pass is the best option. Among these, this chapter introduces the cases where you should deliberately choose Unsafe Pass: for performance improvement, for porting, and for implementing features that only Unsafe can achieve.

==[nonum] Did Render Graph Change Everything?

As described, the arrival of Render Graph has greatly changed the rendering system. Render Graph optimizes automatically, but if you use it without understanding how it works, you cannot make the most of its optimization benefits. The responsibility for understanding and handling the new rendering system lies with us developers.

On the other hand, it is not true that the arrival of Render Graph has overturned all previous optimization concepts. Render Graph is a high-layer topic in the graphics domain. At lower layers closer to the hardware, traditional knowledge is still valid. For example, it is said that transcendental functions such as sin, cos, tan, and pow are expensive in shaders. This also holds in modern mobile environments. Part 1, Chapter 4, "Mobile Shader Optimization: Measuring and Reducing the Cost of Transcendental Functions", shows an example of measurement results when using transcendental functions in a modern mobile environment. It then introduces several approximation functions to reduce their cost. I myself once wondered whether approximation functions are still needed in modern mobile environments, where computing performance has improved. This chapter offers one answer to that question. The features introduced in Part 1, Chapter 5, "Graphics Optimization with Renderer Shader User Value", and Part 1, Chapter 6, "Shader Warmup with GraphicsStateCollection", were also added to solve problems that have remained since before Unity 6.

==[nonum] The Cost of Drawing Many Objects

Renderer Shader User Value (RSUV) is a new feature added in Unity 6.3. The problem it tries to solve is the long-standing CPU cost of drawing many objects, especially the cost of Set Pass Calls. Means of solving this problem include the SRP Batcher and GPU instancing.

However, the SRP Batcher had a constraint when you wanted to change the appearance slightly for each object. The SRP Batcher does not support per-renderer property changes using MaterialPropertyBlock, so to pass individual parameters while keeping the SRP Batcher, you had to duplicate a Material for each renderer you wanted to customize. Increasing the number of Materials for slight parameter differences is undesirable in terms of both memory and management.

RSUV is a lighter option for this problem. With RSUV, each renderer holds one 32-bit unsigned integer that the shader can reference. Setting this value does not break batching, so you can change parameters while keeping the SRP Batcher. You can pass only one 32-bit value, but by bit-packing colors, indices, flags, and so on, you can cover many use cases.

==[nonum] Spikes During Shader Compilation

Shader warmup with GraphicsStateCollection solves the problem of spikes during shader compilation. GraphicsStateCollection was added in Unity 6 as Experimental and was released as an official feature in Unity 6.5.

Existing warmup APIs such as ShaderVariantCollection.WarmUp did not work as expected on Vulkan, Metal, and similar APIs. This becomes clear once you know how shader warmup works on Vulkan and Metal: the legacy API could not pass the parameters required for correct warmup, so warmup could not be performed completely there. To avoid spikes during gameplay, developers had to use workaround-style warmup, such as rendering in the background while the screen is blacked out.

GraphicsStateCollection is a new API that solves this problem. It is a mechanism for collecting, at application runtime, various pieces of information needed for shader warmup, such as vertex layouts. By collecting this information, you can warm up shaders completely. However, GraphicsStateCollection has some new problems, such as missed information. These are less problems of GraphicsStateCollection itself than problems caused by Unity's Shader Variant Stripping or by the specifications of modern graphics APIs such as Vulkan and Metal.

==[nonum] Toward Part 1

With Render Graph, the system for building rendering pipelines has changed greatly. It is a new rendering system in which defining resource dependencies lets you destroy unneeded resources, and which is highly compatible with tile-based GPUs.

However, a renovated high-layer rendering system does not mean that the mechanisms at layers closer to the hardware have changed. The problems that Render Graph, GraphicsStateCollection, and RSUV try to solve are problems we have had for a long time.

Part 1 introduces new features added in Unity 6, universal topics, and tips you can use right away, such as Part 1, Chapter 3, "How to Omit URP's FinalBlitPass".

We hope this content helps your development.
