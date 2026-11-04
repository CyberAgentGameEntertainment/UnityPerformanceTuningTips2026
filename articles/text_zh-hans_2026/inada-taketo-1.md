# **UniTask 与取消处理的生命周期管理**

姓名 / 罗马字：Inada Taketo

所属：株式会社 CyberAgent SGE 核心技术本部

X 账号 / 网站 / GitHub 账号（可选）：

在使用 UniTask 的异步处理中，如果没有妥善管理结束条件和取消，可能会留下无法完成的处理或不必要的引用。

前作《Unity 性能调优宝典》介绍了 MonoBehaviour 被销毁后 UniTask 仍持续等待的案例。本节介绍因遗漏销毁 CancellationTokenSource 而导致不必要的对象持续被引用的案例。

需要注意的是，CancellationTokenSource 和 CreateLinkedTokenSource 并非 UniTask 特有的功能，而是构成 .NET 取消处理的 API。

本节涉及的代码中，using 声明（using var）是 C\# 8 的语法，可在 Unity 2020.2 及更高版本中使用。另外，后文提到的 CreateLinkedTokenSource 内部实现参考了 Unity Technologies 公开的面向 Unity 2022.3 的 Mono 实现（unity-2022.3-mbe 分支），可能会随 Unity 版本而变化。

### **CancellationTokenSource 的 Dispose**

CancellationToken 是用于向处理传递取消请求的值类型。CancellationToken 本身并未实现 IDisposable，因此无需调用 Dispose。

不过，如果通过 CancellationToken.Register 注册了回调，其返回值 CancellationTokenRegistration 则实现了 IDisposable{{fn:cancellation_token_registration_dispose}}。取消前注册的回调会被发行方的 CancellationTokenSource 持有，因此在不再需要注册时若不调用 Dispose 解除注册，回调所引用的对象可能无法释放。

另一方面，CancellationTokenSource 实现了 IDisposable。使用完毕的 CancellationTokenSource 由其所有者调用 Dispose。

CODE START csharp | 对 CancellationTokenSource 调用 Dispose 的示例 {{list:cts_dispose}}

private async UniTask LoadCancelableAsync()

{

	using var cts \= new CancellationTokenSource();

	

	// 省略发出取消请求的处理

	await LoadAsync(cts.Token);

}

CODE END

{{list:cts_dispose}} 通过 using 声明管理 CancellationTokenSource。不仅在异步处理正常结束时，在因取消或异常而结束时，退出方法时也会自动调用 cts.Dispose()。

Cancel 与 Dispose 的职责不同。Cancel 向处理请求取消，而 Dispose 解除并释放 CancellationTokenSource 持有的计时器和回调注册等。仅调用 Dispose 不会向处理通知取消请求。

需要注意的是，即使忘记调用 Dispose，只要 CancellationTokenSource 及其关联对象都不再被外部引用，它们就会被 GC 回收{{fn:managed_memory}}。不过，由于向长生命周期的 CancellationTokenSource 注册回调等情况可能导致引用残留，原则上应在使用完毕时调用 Dispose。

### **CancellationTokenSource.CreateLinkedTokenSource**

CancellationTokenSource.CreateLinkedTokenSource 会创建一个监视多个源 Token 的新 CancellationTokenSource。当任一源 Token 被取消时，所创建的 CancellationTokenSource 的 Token 也会进入取消状态。

源 Token 之间并不会相互取消，取消是从源 Token 向链接目标传播的。

下文将 tokenA 和 tokenB 的发行方 CancellationTokenSource 分别称为 A 和 B。

在 Unity Technologies 于 GitHub 公开的面向 Unity 2022.3 的 Mono 实现{{fn:mono_cancellation_token_source}}中，CreateLinkedTokenSource 会向源 Token 注册取消回调。将与引用关系相关的部分简化后，如 {{list:linked_source_internal}} 所示。

CODE START csharp | CreateLinkedTokenSource 内部实现的要点 {{list:linked_source_internal}}

linkedCts.m\_linkingRegistrations\[0\] \=

	tokenA.InternalRegisterWithoutEC(

    	s\_LinkedTokenCancelDelegate,

    	linkedCts);

 

linkedCts.m\_linkingRegistrations\[1\] \=

	tokenB.InternalRegisterWithoutEC(

    	s\_LinkedTokenCancelDelegate,

    	linkedCts);

CODE END

注册到源 Token 的回调会引用 linkedCts，而 linkedCts 也持有对 A 和 B 的注册信息。

因此，如果 B 是在整个应用程序中使用的长生命周期 CancellationTokenSource，并且长时间不会被取消，那么若不对 linkedCts 调用 Dispose，可能会残留以下引用。

长生命周期的 B → 回调注册 → linkedCts → 对 A 的注册信息 → 短生命周期的 A

仅凭这种引用关系并不一定会导致内存泄漏。只要所有对象都不再被外部引用，它们就会一并被 GC 回收。但是，在 B 存活且用于链接的回调尚未解除期间，linkedCts 会持续被 B 引用，短生命周期的 A 也会经由它保持可达状态。

此外，如果注册到 linkedCts.Token 的回调引用了其他对象，这些对象也可能被一并持有。

对 linkedCts 调用 Dispose 后{{fn:cancellation_token_source_dispose}}，内部会销毁对 A 和 B 的 CancellationTokenRegistration，并解除注册到源 Token 的回调。

建议在使用链接 Token 的处理作用域内，通过 using 声明等方式管理 CreateLinkedTokenSource 的返回值。

CODE START csharp | 对链接的 CancellationTokenSource 调用 Dispose 的示例 {{list:linked_source_using}}

private async UniTask LoadAsync(

	CancellationToken tokenA,

	CancellationToken tokenB)

{

	using var linkedCts \=

        CancellationTokenSource.CreateLinkedTokenSource(

        	tokenA,

        	tokenB);

 

	await LoadAssetAsync(linkedCts.Token);

}

CODE END

{{list:linked_source_using}} 中，在 LoadAssetAsync 因正常结束、取消或异常而结束后会调用 linkedCts.Dispose()，解除对源 Token 的注册。

即使链接的 CancellationTokenSource 已处于取消状态，对其他源 Token 的注册也可能仍然残留。CreateLinkedTokenSource 的返回值在不再需要时务必调用 Dispose。

### **在团队中防止遗漏 IDisposable 对象的 Dispose**

到目前为止，我们以 CancellationTokenSource 为例分析了遗漏销毁 IDisposable 的问题。对策的前提是对象的所有者在适当的时机调用 Dispose。然而，随着项目规模扩大，仅靠代码审查持续确认所有创建位置变得困难。尤其是 CreateLinkedTokenSource 只需简短的代码即可创建，因此很容易忽略其返回值是 IDisposable。

要在团队中防止问题复发，除了个人的注意之外，还应结合以下做法：将需要销毁的对象收敛到易于使用的 API 中、通过 Analyzer 尽早检测问题，以及根据需要在 CI 中检测新的违规。不要把职责集中在 Analyzer 上，而是从设计、静态分析和运维三个方面共同支撑同一方针，效果会更好。

此处介绍的通过 using 销毁和通过 Analyzer 检测的方法，不仅适用于 CancellationTokenSource，也可应用于实现 IDisposable 的其他资源。

在这些机制化手段中，本节介绍使用可自动检查 C\# 代码的 Roslyn Analyzer 的方法。

### **什么是 Roslyn Analyzer**

Roslyn Analyzer 是一种在编译时或 IDE 中对 C\# 代码进行静态分析，并在可能存在问题的位置显示警告或建议的机制。它可以基于语法、类型信息、执行路径等检测 bug 征兆和编码规范违规，如果准备了 Code Fix，还能提供修复候选。

Analyzer 将哪些内容检测为问题由诊断规则定义，每条规则都会被赋予诸如 CA2000 之类的用于识别的诊断 ID。CA2000 是 .NET 提供的现有规则，后文提到的 UTK001 和 UTK002 则是项目自行添加的规则方案。

在 Unity 项目中，将 Analyzer 的 DLL 放置在 Assets 下并添加 RoslynAnalyzer 标签即可引入{{fn:roslyn_analyzers}}。也可以利用 asmdef 限定适用的程序集。

### **将现有 Analyzer 作为第一道防线**

首先值得考虑的是 .NET 代码质量规则中包含的 CA2000{{fn:ca2000}}。CA2000 可以检测在局部作用域内创建的 IDisposable 对象在未被销毁的情况下离开作用域的路径。由于 CancellationTokenSource 也是 IDisposable，因此可以将其用作检测 new CancellationTokenSource() 遗漏销毁的第一道防线。

不过，根据所使用的 Analyzer 版本和分析设置，对于像 CreateLinkedTokenSource 这样创建并返回对象的方法的返回值，以及方法之间的所有权转移，能够追踪到何种程度会有所不同。引入时，建议在小型验证项目中针对 new CancellationTokenSource() 和 CreateLinkedTokenSource 两者确认能检测到的示例和检测不到的示例。现有规则无法充分检测的模式，可根据需要用后文介绍的自定义规则补充。

### **面向异步处理的自定义规则设计示例**

如果现有 Analyzer 无法充分追踪 CreateLinkedTokenSource 的返回值，可以用项目专用的 Roslyn Analyzer 加以补充。以下是实现自定义 Analyzer 时的规则设计示例。为便于说明，使用 UTK001 和 UTK002 作为临时的诊断 ID。

UTK001 设想检查由 new CancellationTokenSource() 或 CancellationTokenSource.CreateLinkedTokenSource() 创建的局部变量，是否受到 using 语句、using 声明或 finally 块中的 Dispose 保护。分析范围不仅包括所有正常结束路径，还包括通过 return 或异常退出方法的路径。

CODE START csharp | UTK001 检测到的示例 {{list:analyzer_detects_linked_source}}

private async UniTask LoadAsync(

	CancellationToken tokenA,

	CancellationToken tokenB)

{

	var linkedCts \=

        CancellationTokenSource.CreateLinkedTokenSource(

        	tokenA,

        	tokenB);

 

	await LoadAssetAsync(linkedCts.Token);

}

CODE END

{{list:analyzer_detects_linked_source}} 中，linkedCts 在未被 Dispose 的情况下退出了方法。该规则设想在 CreateLinkedTokenSource 的调用位置发出警告，并提示使用 using 声明或 try-finally 来保证销毁。

UTK002 设想检查 CancellationToken.Register 的返回值 CancellationTokenRegistration 是否被直接丢弃。不接收 Register 的返回值而直接丢弃的代码，其解除注册的职责不明确。如果回调只需在异步处理期间有效，建议用 using 声明管理。如果保存到字段中，则在所有者的结束处理中调用 Dispose。

CODE START csharp | 管理 CancellationTokenRegistration 的示例 {{list:registration_using}}

private async UniTask WaitAsync(CancellationToken token)

{

	using var registration \= token.Register(OnCanceled);

 

	await WaitForSignalAsync(token);

}

CODE END

{{list:registration_using}} 中，在 WaitForSignalAsync 结束时 registration 会被销毁。另一方面，也存在让注册与 Token 的生命周期保持一致的设计，例如向整个应用程序共享的 Token 进行永久注册。如果对这类示例也一律发出警告，误报会增加，因此建议 UTK002 先以信息级别试运行，并允许排除项目中常见的安全模式。

保存在字段中的 CancellationTokenSource 比局部变量更难判定所有权。作为第二阶段的规则，可以检查在向字段赋值新的 CancellationTokenSource 之前是否对旧值调用了 Dispose，以及是否在 MonoBehaviour 的 OnDestroy 等所有者的结束处理中调用了 Dispose。不过，由于可能伴随向其他对象转移所有权的情况，建议与局部变量的规则分开引入。

### **通过 Code Fix 统一修复方法**

Roslyn Analyzer 还可以提供在 IDE 中自动修复检测到的问题的 Code Fix。例如，针对 UTK001 中所有权在方法内闭合的简单情形，提供将 var 转换为 using var 的 Code Fix，就更容易在团队内统一可安全机械转换的模式的修复方法。对于向字段赋值或通过返回值转移所有权等无法唯一确定销毁时机的情形，不要提供自动修复，而应仅给出促使确认设计的诊断。

在代码审查中，除了 Analyzer 的结果之外，还建议确认能否从代码中读出创建、Cancel、Dispose 的负责方。还需注意，仅凭 Analyzer 没有发出警告并不能证明安全性。

### **仅靠 Analyzer 无法检测的问题**

静态分析有其局限。当 CancellationTokenSource 被存入集合，或被传递给其他对象并转移所有权等情况下，可能无法准确判断最终的销毁位置。如果为了避免误报而削弱分析，漏报又会增加。

这一领域可以通过 Development Build 或测试环境中的运行时验证来补充。如果经由项目通用的创建机制或管理生命周期的类型来创建 CancellationTokenSource，并记录创建数和销毁数，就可以在场景切换或测试结束时确认是否残留未销毁的作用域。如果还要记录创建时的堆栈跟踪，考虑到开销，建议仅在开发环境中启用。

可根据需要结合以下手段：通过 Profiler 确认内存增长的调查、运行时的未销毁检测，以及 Roslyn Analyzer 在编辑时的警告。不仅要有问题发生后由负责人调查的流程，还要建立在编写代码时就能提示修复方法的流程，这有助于团队层面防止问题复发。

{{pagebreak}}

### **参考资料**

Unity Manual《Roslyn 分析器和源生成器》

Microsoft Learn《CA2000：丢失范围之前释放对象》

Unity Technologies《Mono CancellationTokenSource.cs（unity-2022.3-mbe 分支）》

Microsoft Learn《CancellationTokenSource.Dispose Method》

Microsoft Learn《CancellationTokenRegistration.Dispose Method》

Unity Manual《托管内存》

脚注: {{fn:roslyn_analyzers}}

https://docs.unity3d.com/ja/2022.3/Manual/roslyn-analyzers.html（访问日期：2026 年 9 月 8 日）

脚注: {{fn:ca2000}}

https://learn.microsoft.com/ja-jp/dotnet/fundamentals/code-analysis/quality-rules/ca2000（访问日期：2026 年 9 月 8 日）

脚注: {{fn:mono_cancellation_token_source}}

https://github.com/Unity-Technologies/mono/blob/unity-2022.3-mbe/mcs/class/referencesource/mscorlib/system/threading/CancellationTokenSource.cs（访问日期：2026 年 9 月 8 日）

脚注: {{fn:cancellation_token_source_dispose}}

https://learn.microsoft.com/ja-jp/dotnet/api/system.threading.cancellationtokensource.dispose（访问日期：2026 年 9 月 8 日）

脚注: {{fn:cancellation_token_registration_dispose}}

https://learn.microsoft.com/ja-jp/dotnet/api/system.threading.cancellationtokenregistration.dispose（访问日期：2026 年 9 月 8 日）

脚注: {{fn:managed_memory}}

https://docs.unity3d.com/ja/2022.3/Manual/performance-managed-memory.html（访问日期：2026 年 9 月 8 日）