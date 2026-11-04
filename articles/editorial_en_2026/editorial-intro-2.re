=[nonum] Scripting and Animation Trends in 2026
#@# 原稿は sources/editorial-intro-2.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

==[nonum] Scripting Trends

The scripting situation has not changed much in the Unity 6 generation. In Unity 7, however, the addition of @<b>{CoreCLR} and @<b>{ECS for All} is planned, features that greatly change the assumptions behind performance tuning of C# code, and some API changes toward them are already being made in the Unity 6 generation. Along with the APIs of the Unity 6 generation, this section introduces some of the changes planned for Unity 7. Note that the information on Unity 7 is current as of this writing and may differ from the actual release.

=== Asynchronous Processing

@<b>{Awaitable} is a standard Unity class introduced in Unity 2023. It lets you write efficient async / await code using only standard Unity classes, which previously required external libraries such as @<b>{UniTask}. However, Awaitable has a smaller feature set than UniTask, so if you already use UniTask, you can continue to use it. In situations where you cannot depend on external packages, using Awaitable instead of Task, ValueTask, or coroutines can be expected to improve performance. Part 2, Chapter 5 also explains the management of CancellationToken, which tends to be a problem in asynchronous processing.

=== .NET Runtime

As of Unity 6, the .NET runtimes are still Mono and IL2CPP, with the .NET Standard 2.1 API set and C# 9 available. In Unity 7, Mono will be removed, and @<b>{CoreCLR} is planned to be introduced in the Editor and some desktop players. CoreCLR is the runtime implementation used in .NET itself (.NET Core / .NET) and is ahead of Mono in performance. CoreCLR will not be introduced on mobile and console platforms for the time being, and IL2CPP will remain the first-class option there. However, the Editor will move to CoreCLR, and there will be some benefits at runtime, such as additional standard APIs and optimized implementations.

==== The CoreCLR JIT

CoreCLR has many optimizations in the JIT, which converts C# code into native code. In most cases, simply moving from Mono to CoreCLR will improve the execution speed of C# code. According to benchmark results published by Unity, for workloads that do not use the GC, CoreCLR runs 2 to 3 times faster than Mono and achieves performance comparable to IL2CPP and Burst@<fn>{fn1}. Until now, IL2CPP has been the best choice for performance, especially on desktop platforms where you can choose between Mono and IL2CPP, but this assumption may change once CoreCLR arrives.

==== The .NET 10 BCL

Until now, the .NET BCL@<fn>{fn2} has been the implementation bundled with Mono, limited to an API set equivalent to .NET Standard 2.1. With the introduction of CoreCLR, it is expected to be updated to a BCL equivalent to .NET 10. The available APIs will increase greatly, and the internal implementation will be more refined, so performance improvements can be expected. The new BCL is planned to be available in IL2CPP as well, which brings performance and functional benefits to mobile and console platforms where CoreCLR is not introduced.

==== CoreCLR GC / Memory Management

The CoreCLR garbage collector (GC) implementation also differs from Mono / IL2CPP. Unity's Mono and IL2CPP have used a GC implementation called the @<b>{Boehm GC} (Boehm-Demers-Weiser GC, bdwgc), whereas the CoreCLR GC has characteristics that differ greatly from the Boehm GC@<fn>{fn3}.

First, the CoreCLR GC is a @<b>{generational GC}. A generational GC manages allocated objects in groups called "generations". Newly allocated objects are first treated as gen0. GC on gen0 runs relatively frequently, but objects that survive a gen0 GC move to the next generation, gen1, where GC runs less frequently than for gen0. This is based on the general observation that most newly allocated objects are short-lived, while objects that have survived to some extent often survive for a long time afterward, and it aims to shorten the time taken by the frequent GCs on young generations. For memory optimization with the CoreCLR GC, which generation an allocated object belongs to and when the GC for each generation runs will be important points.

The CoreCLR GC also performs @<b>{compaction}. The Boehm GC does not move objects once allocated, so repeated allocation and release gradually fragments the heap, and the heap tends to grow larger than the size actually needed. With compaction, the CoreCLR GC moves objects within the fragmented heap and fills the gaps between them. This keeps heap growth in check.

Note that IL2CPP continues to use the Boehm GC, so the same memory optimization strategies as before remain effective@<fn>{fn4}.

Besides the GC, CoreCLR also actively optimizes memory allocation in the JIT. For example, @<b>{Object Stack Allocation} is a JIT feature that allocates very short-lived objects, whose use is confined to a method, on the stack instead of the heap. Short-lived objects allocated in large numbers on hot paths, such as inside loops, may cause fewer problems than in Mono or IL2CPP.

==== C# 14 Support

As of Unity 6, C# 9 is officially supported, but C# 14 is planned to be available in Unity 7. Recent C# releases have added many new features for performance improvement, so there will be more language features to know from a performance tuning perspective.

==== Features Introduced Ahead of Time in the Unity 6 Generation

The official introduction of CoreCLR is planned for Unity 7, but an experimental introduction of CoreCLR has started in the Unity 6 series on some platforms. In Unity 6.7 alpha (6000.7.0a6), the latest as of this writing, CoreCLR is available in standalone players for Windows, macOS, and Linux. However, the API set is still limited to .NET Standard 2.1. Also in Unity 6.7 alpha, a BCL implementation equivalent to .NET 10 is available in the iOS player (IL2CPP), but this too is limited to .NET Standard 2.1. At this stage, it should be considered usable only for verifying performance and compatibility toward the CoreCLR migration in Unity 7.

=== DOTS

In Unity 6, the DOTS (Data-Oriented Technology Stack) feature set remains available. This includes @<b>{Burst}, which converts C# code into highly optimized native code using LLVM; the @<b>{C# Job System}, which safely multithreads code; and the @<b>{Entity Component System (ECS)}, a mechanism that uses them to update game state faster than MonoBehaviour.

In Unity 7, the integration of ECS and GameObject is expected to be strengthened by a feature set called @<b>{ECS for All}. Until now, Entity, the container that stores components in ECS, has not been compatible with GameObject, and ECS state has been managed completely independently of GameObject. In Unity 7, GameObject will also function as an Entity, and you will be able to attach ECS components to a GameObject. Mixing and integrating ECS and GameObject becomes easier than before, which makes it easier to move the MonoBehaviour Update processing, which tends to become a bottleneck, to ECS. This should help reduce main thread load and improve the frame rate.

#@# この節の最後の1行だけが次のページにあふれるため、このページを1行延ばす。
//embed[latex]{
\enlargethispage{\baselineskip}
//}

==[nonum] Animation Trends

The Unity 6 generation brings several improvements to animation performance. Unity 6.3 added the @<b>{ResetControllerState} method to Animator and AnimationPlayableGraph, making it easy to pool Animators. Part 2, Chapter 2 explains this in detail. Unity 6.5 added the @<b>{AnimationEventInfo} struct, which avoids heap memory allocation by the AnimationEvent class when receiving Animation Events.

//footnote[fn1][Path to CoreCLR | GDC 2026 - Unity - YouTube @<href>{https://youtu.be/_t6xVfrmEWU?si=edILni6YnObyWkoj&t=218}]
//footnote[fn2][Base Class Library. Refers to the standard class library implementation, such as the System namespace]
//footnote[fn3][Fundamentals of garbage collection - .NET | Microsoft Learn - @<href>{https://learn.microsoft.com/en-us/dotnet/standard/garbage-collection/fundamentals}]
//footnote[fn4][Understanding Memory Management in Unity - Unity Discussion - @<href>{https://discussions.unity.com/t/understanding-memory-management-in-unity/1700107}]
