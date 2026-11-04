# **UniTask and Lifetime Management of Cancellation**

Name：Inada Taketo

Affiliation：CyberAgent, Inc. SGE Core Technology Division

X account / Website / GitHub account (optional):

In asynchronous processing with UniTask, if termination conditions and cancellation are not managed properly, operations that never complete and unnecessary references can remain.

The previous book, "Unity Performance Tuning Bible", introduced a case in which UniTask keeps waiting even after the MonoBehaviour is destroyed. This section introduces a case in which unnecessary objects keep being referenced because a CancellationTokenSource is not disposed.

Note that CancellationTokenSource and CreateLinkedTokenSource are not UniTask-specific features but APIs that make up cancellation handling in .NET.

Among the code covered in this section, the using declaration (using var) is C\# 8 syntax and is available in Unity 2020.2 and later. In addition, the internal implementation of CreateLinkedTokenSource described later is based on the Mono implementation for Unity 2022.3 published by Unity Technologies (the unity-2022.3-mbe branch) and may vary depending on the Unity version.

### **Disposing a CancellationTokenSource**

CancellationToken is a value type for conveying a cancellation request to an operation. CancellationToken itself does not implement IDisposable, so there is no need to call Dispose.

However, when a callback is registered with CancellationToken.Register, the returned CancellationTokenRegistration is IDisposable{{fn:cancellation_token_registration_dispose}}. A callback registered before cancellation is held by the originating CancellationTokenSource, so unless you Dispose the registration to unregister it when it is no longer needed, the objects referenced by the callback may not be released.

On the other hand, CancellationTokenSource implements IDisposable. When a CancellationTokenSource is no longer in use, its owner calls Dispose.

CODE START csharp | Example of disposing a CancellationTokenSource {{list:cts_dispose}}

private async UniTask LoadCancelableAsync()

{

	using var cts \= new CancellationTokenSource();

	

	// The code that issues the cancellation request is omitted

	await LoadAsync(cts.Token);

}

CODE END

In {{list:cts_dispose}}, the CancellationTokenSource is managed with a using declaration. cts.Dispose() is called automatically when the method exits, not only when the asynchronous operation completes normally but also when it ends through cancellation or an exception.

Cancel and Dispose have different roles. Cancel requests cancellation of the operation, while Dispose unregisters and releases the timers, callback registrations, and so on held by the CancellationTokenSource. Calling Dispose alone does not notify the operation of a cancellation request.

Note that even if you forget to call Dispose, the CancellationTokenSource and its related objects become eligible for garbage collection once none of them is referenced from outside{{fn:managed_memory}}. However, references can remain, for example through callback registrations on a long-lived CancellationTokenSource, so as a rule, Dispose it as soon as you finish using it.

### **CancellationTokenSource.CreateLinkedTokenSource**

CancellationTokenSource.CreateLinkedTokenSource creates a new CancellationTokenSource that monitors multiple source tokens. When any of the source tokens is canceled, the Token of the created CancellationTokenSource enters the canceled state.

The source tokens are not canceled by one another; cancellation propagates from the source tokens to the linked source.

In the following, let A and B be the CancellationTokenSources that issue tokenA and tokenB, respectively.

In the Mono implementation for Unity 2022.3 published by Unity Technologies on GitHub{{fn:mono_cancellation_token_source}}, CreateLinkedTokenSource registers a cancellation callback on the source tokens. Simplifying the parts relevant to the reference relationships gives {{list:linked_source_internal}}.

CODE START csharp | Key points of the internal implementation of CreateLinkedTokenSource {{list:linked_source_internal}}

linkedCts.m\_linkingRegistrations\[0\] \=

	tokenA.InternalRegisterWithoutEC(

    	s\_LinkedTokenCancelDelegate,

    	linkedCts);

 

linkedCts.m\_linkingRegistrations\[1\] \=

	tokenB.InternalRegisterWithoutEC(

    	s\_LinkedTokenCancelDelegate,

    	linkedCts);

CODE END

linkedCts is referenced by the callbacks registered on the source tokens, and linkedCts also holds the registration information for A and B.

Therefore, if B is a long-lived CancellationTokenSource used throughout the application and is not canceled for a long time, the following references can remain unless linkedCts is disposed.

Long-lived B → callback registration → linkedCts → registration information for A → short-lived A

This reference relationship alone does not necessarily cause a memory leak. Once none of the objects is referenced from outside, they all become eligible for garbage collection together. However, while B is alive and the linking callback has not been unregistered, linkedCts continues to be referenced from B, and the short-lived A remains reachable through it.

In addition, if the callbacks registered on linkedCts.Token reference other objects, those objects may also be retained.

When linkedCts is disposed{{fn:cancellation_token_source_dispose}}, the CancellationTokenRegistrations for A and B are disposed internally, and the callbacks registered on the source tokens are unregistered.

It is good to manage the return value of CreateLinkedTokenSource with a using declaration or similar, within the scope of the operation that uses the linked token.

CODE START csharp | Example of disposing a linked CancellationTokenSource {{list:linked_source_using}}

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

In {{list:linked_source_using}}, linkedCts.Dispose() is called after LoadAssetAsync ends through normal completion, cancellation, or an exception, and the registrations on the source tokens are removed.

Even if a linked CancellationTokenSource is already in the canceled state, registrations on other source tokens may remain. Always Dispose the return value of CreateLinkedTokenSource as soon as it is no longer needed.

### **Preventing Missed Dispose Calls on IDisposable Objects as a Team**

So far, we have looked at missed disposal of IDisposable objects using CancellationTokenSource as an example. The countermeasures assume that the owner of an object calls Dispose at the appropriate time. However, as a project grows, it becomes difficult to keep checking every creation site through code review alone. In particular, CreateLinkedTokenSource can create an object with very little code, which makes it easy to overlook that the return value is IDisposable.

To prevent recurrence as a team, in addition to individual attention, it is good to combine steering objects that require disposal toward easy-to-use APIs, detecting problems early with an Analyzer, and, where necessary, detecting new violations in CI. Rather than concentrating all responsibility on the Analyzer, it is effective to support the same policy from design, static analysis, and operations.

The disposal with using and the detection with an Analyzer introduced here are not limited to CancellationTokenSource and can also be applied to other resources that implement IDisposable.

Among such mechanisms, this section introduces a method that uses a Roslyn Analyzer, which can automatically inspect C\# code.

### **What Is a Roslyn Analyzer**

A Roslyn Analyzer is a mechanism that statically analyzes C\# code at compile time or in the IDE and displays warnings or suggestions at locations that may be problematic. Based on syntax, type information, execution paths, and so on, it can detect signs of bugs and violations of coding conventions, and if a Code Fix is provided, it can also suggest fixes.

What an Analyzer detects as a problem is defined as a diagnostic rule, and each rule is assigned an identifying diagnostic ID such as CA2000. CA2000 is an existing rule provided by .NET, while UTK001 and UTK002, described later, are proposed rules to be added by the project itself.

In a Unity project, an Analyzer is introduced by placing its DLL under Assets and assigning the RoslynAnalyzer label{{fn:roslyn_analyzers}}. You can also use an asmdef to limit the assemblies to which it applies.

### **Using an Existing Analyzer as the First Line of Defense**

The first thing to consider is CA2000, included in the .NET code quality rules{{fn:ca2000}}. CA2000 detects paths in which a locally created IDisposable object goes out of scope without being disposed. Because CancellationTokenSource is also IDisposable, it can serve as the first line of defense for detecting a missed disposal of new CancellationTokenSource().

However, depending on the Analyzer version and analysis settings, the extent to which it can track the return value of a method that creates and returns an object, such as CreateLinkedTokenSource, and the transfer of ownership between methods varies. When adopting it, it is good to check, in a small verification project, both detected and undetected examples for new CancellationTokenSource() and CreateLinkedTokenSource. Patterns that existing rules cannot detect sufficiently can be supplemented with the custom rules described later, as needed.

### **Example Custom Rule Design for Asynchronous Processing**

If an existing Analyzer cannot sufficiently track the return value of CreateLinkedTokenSource, one option is to supplement it with a project-specific Roslyn Analyzer. The following is an example of rule design when implementing a custom Analyzer. For illustration, UTK001 and UTK002 are used as provisional diagnostic IDs.

UTK001 is assumed to check whether a local variable created with new CancellationTokenSource() or CancellationTokenSource.CreateLinkedTokenSource() is protected by a using statement, a using declaration, or a Dispose in a finally clause. The analysis covers not only all normal completion paths but also paths that exit the method through return or an exception.

CODE START csharp | Example detected by UTK001 {{list:analyzer_detects_linked_source}}

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

In {{list:analyzer_detects_linked_source}}, the method exits without linkedCts being disposed. This rule is assumed to issue a warning at the CreateLinkedTokenSource call site and guide the developer to guarantee disposal with a using declaration or try-finally.

UTK002 is assumed to check whether the CancellationTokenRegistration returned by CancellationToken.Register is left undisposed. Code that discards the return value of Register leaves the responsibility for unregistering unclear. If the callback should be valid only during the asynchronous operation, it is good to manage it with a using declaration. If it is held in a field, Dispose it in the owner's termination process.

CODE START csharp | Example of managing a CancellationTokenRegistration {{list:registration_using}}

private async UniTask WaitAsync(CancellationToken token)

{

	using var registration \= token.Register(OnCanceled);

 

	await WaitForSignalAsync(token);

}

CODE END

In {{list:registration_using}}, registration is disposed when WaitForSignalAsync finishes. On the other hand, some designs match the lifetime of the registration to that of the Token, such as permanently registering on a Token shared across the whole application. Warning on such cases uniformly increases false positives, so it is good to try UTK002 at the information level first and make it possible to exclude safe patterns that appear frequently in the project.

A CancellationTokenSource held in a field makes ownership harder to determine than a local variable. As a second-stage rule, you can check whether the old value is disposed before a new CancellationTokenSource is assigned to the field, and whether it is disposed in the owner's termination process, such as OnDestroy of a MonoBehaviour. However, ownership may be transferred to another object, so it is good to introduce this separately from the rule for local variables.

### **Standardizing Fixes with Code Fix**

For a Roslyn Analyzer, you can also provide a Code Fix that automatically fixes detected problems in the IDE. For example, for the simple case of UTK001 in which ownership is entirely contained within the method, providing a Code Fix that converts var to using var makes it easier to standardize within the team how patterns that can be safely converted mechanically are fixed. For cases in which the disposal timing cannot be determined uniquely, such as assignment to a field or transfer of ownership through a return value, it is good not to offer an automatic fix and to keep to a diagnostic that prompts a design review.

In code review, it is good to check not only the Analyzer results but also whether the responsibilities for creation, Cancel, and Dispose can be read from the code. Note also that the absence of Analyzer warnings alone does not prove safety.

### **Problems an Analyzer Alone Cannot Detect**

Static analysis has limits. When a CancellationTokenSource is stored in a collection, or when ownership is transferred by passing it to another object, the final disposal location may not be determined accurately. If the analysis is weakened to avoid false positives, missed detections increase instead.

This area can be supplemented by runtime verification in a Development Build or a test environment. If CancellationTokenSources are created through a project-wide creation mechanism or a type that manages lifetime, and the numbers created and disposed are recorded, you can check at scene transitions or at the end of tests whether any undisposed scopes remain. If stack traces at creation are also recorded, it is good to enable this only in development environments, considering the load.

As needed, it is good to combine an investigation that checks memory growth with the Profiler, runtime detection of undisposed objects, and edit-time warnings from a Roslyn Analyzer. Rather than relying only on a flow in which a person investigates after a problem occurs, a flow that can suggest even the fix at the time the code is written helps prevent recurrence across the team.

{{pagebreak}}

### **References**

Unity Manual "Roslyn analyzers and source generators"

Microsoft Learn "CA2000: Dispose objects before losing scope"

Unity Technologies "Mono CancellationTokenSource.cs (unity-2022.3-mbe branch)"

Microsoft Learn『CancellationTokenSource.Dispose Method』

Microsoft Learn『CancellationTokenRegistration.Dispose Method』

Unity Manual "Managed memory"

脚注: {{fn:roslyn_analyzers}}

https://docs.unity3d.com/ja/2022.3/Manual/roslyn-analyzers.html (Accessed: September 8, 2026)

脚注: {{fn:ca2000}}

https://learn.microsoft.com/ja-jp/dotnet/fundamentals/code-analysis/quality-rules/ca2000 (Accessed: September 8, 2026)

脚注: {{fn:mono_cancellation_token_source}}

https://github.com/Unity-Technologies/mono/blob/unity-2022.3-mbe/mcs/class/referencesource/mscorlib/system/threading/CancellationTokenSource.cs (Accessed: September 8, 2026)

脚注: {{fn:cancellation_token_source_dispose}}

https://learn.microsoft.com/ja-jp/dotnet/api/system.threading.cancellationtokensource.dispose (Accessed: September 8, 2026)

脚注: {{fn:cancellation_token_registration_dispose}}

https://learn.microsoft.com/ja-jp/dotnet/api/system.threading.cancellationtokenregistration.dispose (Accessed: September 8, 2026)

脚注: {{fn:managed_memory}}

https://docs.unity3d.com/ja/2022.3/Manual/performance-managed-memory.html (Accessed: September 8, 2026)