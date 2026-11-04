=[nonum] Asset, Settings, and Build Trends in 2026
#@# 原稿は sources/editorial-intro-3.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

Performance tuning is not only about fixing your own code and assets. Even with the same code and assets, the engine version or a single setting can change build time, app size, memory usage, and execution speed. Are you still carrying, with your own mechanisms, problems the engine has already solved? Are you using newly added settings at their defaults without knowing about them? To check this, keeping track of changes in the engine itself is a prerequisite of tuning.

For Unity as well, performance is a high-priority area of the engine. The What's New of each Unity 6 release lists shorter build times, smaller player size, and improved memory management alongside new features every time. The assets, settings, and builds covered in Part 3 are the areas where these changes are especially large.

As of September 2026, when this was written, the latest long-term support version is Unity 6.3 LTS and the latest official release is Unity 6.6. Unity 6.7 is available as a beta as the version planned to become an LTS@<fn>{fn1}, and Unity 7 is scheduled for an early beta in December 2026 and an official release in the first quarter of 2027@<fn>{fn2}. Below, we follow the changes from Unity 6.0 to 6.7 in release order, and within the same version we start with the ones that have the greatest effect. For changes covered in detail in the chapters of Part 3, we give only the key points and point to the relevant chapter. Note that the Unity versions and default values mentioned in the text are current as of this writing and may change in later releases.

==[nonum] Unity 6.0

=== Setting the iOS Memory Limit with an API

Increased Memory Limit and Extended Virtual Addressing are iOS entitlements that relate to the upper limit of the memory an app can use. In earlier Unity versions as well, you could set them by manually editing the Entitlements file of the exported Xcode project or by rewriting it in a post-build process. In Unity 6000.0.11f1 and later, a dedicated API was added to @<tt>{ProjectCapabilityManager}, which adds to the Entitlements and reflects the change in the Xcode project in one step. The role of each and how to check the limit on a physical device are covered in Part 3, Chapter 6, "iOS Memory Limits and Virtual Address Space".

=== Keeping Settings per Configuration with Build Profiles

Build Profiles, introduced in Unity 6.0, is a mechanism for holding multiple build configurations for a single platform. From the start, each profile can hold a scene list, overrides of Player Settings, and additional script Define Symbols. From Unity 6.1, Graphics settings and Quality settings can be overridden per profile@<fn>{fn3}, and in 6.3 the items to override are selected and added. In 6.5, you can also create profiles from scripts with @<tt>{BuildProfile.CreateBuildProfile}.

The performance benefit is that differences in settings per platform or distribution target can be fixed as profiles, instead of using scripts that rewrite settings before a build. For example, splitting Define Symbols lets you exclude debug measurement and logging code from release profiles only, and overriding Quality settings lets you narrow down the quality levels included in the build for low-end configurations. Part 3, Chapter 3, "Switching Rendering Settings Per Device", compares designs that distribute rendering load according to device performance at runtime, in a form that switches Quality Levels and a form that rewrites the values of the Render Pipeline Asset directly. Build Profiles can be combined with them as a means of switching at build time.

=== Managed Stripping Level Low Is Deprecated

The Unity 6.0 manual states that Low of Managed Stripping Level will be removed in the future and advises using Minimal or Medium@<fn>{fn4}. Projects that used Low have to choose between raising it to Medium to make the binary smaller and lowering it to Minimal. Raising it to Medium makes code referenced through reflection more likely to be stripped, and how to deal with this is covered in Part 3, Chapter 4, "Unity Managed Code Stripping Practical Guide".

==[nonum] Unity 6.1

=== Choosing the Graphics Jobs Mode per Device

With the Vulkan Device Filter Asset added in Unity 6.1, you can specify the Graphics Jobs mode per Android device. You can avoid settings that are slow or unstable only on specific GPUs or drivers without changing the settings of other devices.

==[nonum] Unity 6.2

=== Mesh LOD Generated Automatically at Import

Mesh LOD, added in Unity 6.2, automatically generates LODs at import when you enable Generate Mesh LODs in the model import settings@<fn>{fn5}. Even for models without prepared LODs, a single setting reduces the polygon count at a distance and lowers GPU load. If you already use LOD Group, the polygon reduction effect is the same, and the difference is in memory. Mesh LOD adds no vertices; all LODs share the vertex buffer of the original mesh, and only the index buffer increases. This keeps the memory increase smaller than with LOD Group, which holds an entire mesh and GameObject per LOD.

However, the quality of the automatically generated LODs depends on the shape of the original mesh, and you cannot expect the same quality as manually created LODs. The official documentation also shows that, while it suits organic shapes such as creatures and rocks, meshes with high curvature or many separated parts can break, for example with texture misalignment or tree leaves disappearing. You need to check the generated LODs level by level in the Inspector, and for levels that break, decide whether to stop generation with Limit LODs or to return that mesh alone to LOD Group.

Also, only polygons are reduced; materials do not change. It has no effect on Particle System, VFX Graph, or Static Batching, which always use LOD0, and for Skinned Mesh Renderer, deformation is calculated on LOD0 regardless of which LOD is selected. LOD cross-fading requires GPU Resident Drawer, and using it together with LOD Group is not recommended.

==[nonum] Unity 6.3

=== Reducing Shader Variants with Settings

Shader variants are compiled for each combination of keywords, so they multiply as keywords increase. Reducing variants reduces build time, build size, shader load time, and runtime memory all together.

With Shader Build Settings, added in Unity 6.3, you can override the keyword type to @<tt>{shader_feature} or @<tt>{dynamic_branch} or exclude the code of specific keywords from the build without rewriting the shader source@<fn>{fn6}. These settings can be overridden per Build Profile, so you can strip only the variants that are unnecessary for each platform. Excluding built-in keywords can remove even necessary variants, so using it together with Strict Shader Variant Matching, which can detect missing variants, is also essential. The runtime memory of variants remaining in the build can be reduced by limiting the size and number of chunks to load with the Shader Variant Loading setting, available since 2021 LTS@<fn>{fn7}.

==[nonum] Unity 6.4

=== Project Auditor Bundled with the Editor

In Unity 6.4, Project Auditor is bundled with the Editor. It can mechanically identify bloat such as duplicates between Addressables bundles. From 6.4, the diagnostic rules are split into a separate package, so the installation steps differ by version. The investigation procedure is covered in Part 3, Chapter 2, "Asset Bloat in the Live Operation Phase and Investigation with Project Auditor".

=== Disabling the Physics Module

In Unity 6.4, you can disable the built-in Physics module from the Package Manager@<fn>{fn8}. Titles that do not use 3D physics can reduce build size accordingly. However, it must stay enabled if you use Local Volume.

==[nonum] Unity 6.5

=== Implicit Enabling of Read/Write Is Gone

When you enable Read/Write on a texture or mesh, a copy remains on the CPU side in addition to the data transferred to the GPU, and it consumes memory accordingly. Until now, Unity implicitly enabled this setting at build time when the Shape module of Particle System, Terrain details, or Mesh Collider required reads from the CPU. This implicit enabling is gone for textures in Unity 6.5 and for meshes in Unity 6.6. If the setting is not explicitly enabled on the assets that need it, a warning appears in the Inspector and the build fails.

The official documentation gives shorter build time as the reason for this change, but it is also meaningful for memory. Which assets hold a CPU-side copy becomes visible as an asset setting, so you can judge one by one whether CPU reads are really needed, which gives a clue for reducing unnecessary copies.

=== LTO on Android

Unity 6.5 added a Link Time Optimization (LTO) setting for Android. This toggle affects only the engine-side libraries and presupposes disabling Strip Engine Code, so you decide whether to adopt it by comparing the execution speed improvement with the size increase. The measured results are covered in Part 3, Chapter 5, "Unity LTO Settings and Performance Comparison on Android and iOS".

=== The Addition of mimalloc

Unity 6.5 added mimalloc as a native memory allocator. mimalloc is an allocator that lets multiple threads allocate and release memory concurrently. It was opt-in in 6.5 but is enabled by default in Unity 6.6@<fn>{fn9}. Where native memory allocations overlap in jobs and worker threads, it can be expected to reduce waiting between threads. On the other hand, a different allocator also changes the trends in memory usage and fragmentation, and no figures for the effect are officially given. When updating to 6.6, it is safer to compare the transition of native memory with that before the update. If there is a problem, you can disable it from Memory Settings.

=== Target Frame Rate by iOS Thermal State

In Unity 6.5, you can specify in Player Settings the target frame rate used when the iOS thermal state becomes Serious or Critical. Without writing your own processing to lower the frame rate when the device heats up, you can soften the sudden performance drop caused by thermal throttling.

=== Separating the TypeTree of AssetBundles

AssetBundles contain the TypeTree, which is type information of the serialized data. Thanks to the TypeTree, a bundle can be loaded even if the Unity version of the player differs from the one used to build the bundle, but the same type information is duplicated in each bundle and increases size. With Unity 6.5 and Addressables 2.9.1 or later, enabling Extract TypeTree Data separates the TypeTree into a separate file and reduces the size of AssetBundles@<fn>{fn10}. The more bundles a title distributes, the more this helps both download volume and device storage. However, this setting changes all the AssetBundles already distributed, so the official recommendation is to use it in new projects.

==[nonum] Unity 6.6

=== Content Directories, the Successor to AssetBundles

Unity 6.6 added Content Directories as a new content build mechanism that succeeds AssetBundles@<fn>{fn11}. The benefits listed in the official documentation are implicit deduplication, shorter load times, and reduced memory overhead. With AssetBundles, assets referenced from multiple bundles were duplicated in each bundle and could consume size and memory twice. The duplication between bundles covered in Part 3, Chapter 2, "Asset Bloat in the Live Operation Phase and Investigation with Project Auditor", is eliminated automatically with Content Directories. Assets are loaded and released with the @<tt>{Loadable<T>} API, which also supports async/await. Addressables 4.0 adds a schema for Content Directories and can convert existing groups@<fn>{fn12}.

However, Content Directories currently supports only building local content and does not support remote delivery. This is the same in the beta of Unity 6.7, and support for remote content is planned for the Unity 7 generation@<fn>{fn13}. In Addressables 4.0 as well, converting a group that has a remote load path adds the schema in an invalid state. For many mobile titles that add content through downloads, content to be delivered will continue to be handled with AssetBundles, and you will consider migrating from the content bundled with the app. There is also a constraint that the build fails if the total size of assets included in a reference cycle exceeds 64 MB.

Whichever mechanism you use, the design of when to load and when to release assets applies as is. Part 3, Chapter 1, "Lifetime Management of AssetBundles", centralizes loading and release in a foundation layer and decides the lifetime of an asset at the point it is loaded, which prevents release leaks and unnecessary resident assets through structure.

=== Specifying Shader Constants per Build Profile

In Unity 6.6, you can specify the values of shader constants for the whole project or per Build Profile@<fn>{fn14}. Processing that differs only in value per platform can be written separately without increasing variants with keywords.

=== Build History and Build Analysis

Unity 6.6 added the Build History and Build Analysis windows, and player build reports are now accumulated automatically. Because you can follow the transition of size for each build, it is easier to identify the change that caused bloat. However, support for AssetBundle builds is limited.

==[nonum] Unity 6.7 (Announced)

The following features were announced at Unite Seoul in July 2026. As of this writing, they are announced-stage information whose details cannot be confirmed in the documentation@<fn>{fn15}.

=== Neural Texture Compression

Neural Texture Compression (NTC) is a technology that uses a trained model to compress textures with almost no degradation. The announcement showed an example that reduced texture runtime memory by more than 50% and disk size by about 70% without visible degradation. NTC is introduced as part of Unity Neural, which will be provided progressively starting with Unity 6.7.

=== Fast Build Profile

Fast Build Profile is a new build profile planned to be added in Unity 6.7, and is said to reduce shader build time by up to 90%. A mechanism to reduce runtime stutter through PSO tracing and warmup is also introduced.

==[nonum] Toward Part 3

Following Unity 6.0 through 6.7, the changes on the engine side are heading toward making it possible to hold, as settings, what has been compensated for with your own scripts and operations. On the other hand, these changes do not take effect just by waiting with the default values. Something like mimalloc, which changes behavior simply by updating, must be verified by measuring before and after the update. Something like Content Directories cannot yet be migrated to, depending on delivery requirements. Design decisions such as when to load and release assets, how far to strip code, and what to switch per device remain even as the engine version changes.

Each chapter of Part 3 covers the design and measurement needed for those decisions, together with examples from actual projects. We hope it serves as material for judgment when you consider updating to a new version.

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
