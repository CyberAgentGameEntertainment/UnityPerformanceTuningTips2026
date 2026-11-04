# **Unity Managed Code Stripping Practical Guide**

Mukai Yuichiro / Affiliation: Applibot, Inc.  
yucchiy\_  / blog.yucchiy.com / yucchiy  
\---

As a means of reducing app size, this chapter introduces Managed Code Stripping, a Unity feature that removes unused code at build time.  
It can be enabled with a single setting, but increasing the level of reduction can cause problems in which a specific feature works in the Editor yet breaks only in the built app.  
This chapter covers how Managed Code Stripping works and how to use it, the typical patterns in which more code is removed than necessary, and how to deal with them.

The verification environment is as follows.

*  Unity 6000.3.19f1  
* Addressables 4.0.1  
* macOS Sequoia 15.7.3

To state the conclusion first, Managed Code Stripping is not an optimization that you can finish by setting the level to High.

It is safe to use only when combined with an operation that makes dependencies invisible to static analysis explicit and keeps verifying the size and behavior of the built app.

# **What Is Managed Code Stripping**

## **Overview**

Managed Code Stripping is a standard Unity feature that detects unused managed code (C\# code) at build time and removes it from the built app.

By removing classes and methods that are not used from scenes or scripts, it reduces the code size of the app.

The target is not limited to user code. It also covers UnityEngine modules, the .NET class library, and the assemblies of packages and plugins in the project, so it can reduce unused code inside libraries that are difficult to modify yourself.

In IL2CPP builds in particular, reducing managed code also reduces the C++ code that IL2CPP subsequently generates, which shortens app size and build time.

Note that the similarly named Strip Engine Code setting targets the native code of the Unity engine, and is a separate feature from Managed Code Stripping, which targets managed code.

### **\#\#\# Managed Stripping Level**

How aggressively code is removed is controlled by the Managed Stripping Level setting.

You can set it from "Managed Stripping Level" in Edit \> Project Settings \> Player \> Other Settings \> Optimization, and the following levels are available.

表: Managed Stripping Level options {{table:stripping_levels}}

| Level | Overview |
| :---- | :---- |
| Disabled | Performs no stripping. Selectable only in Mono builds |
| Minimal | Reduces only UnityEngine and the .NET class library, and does not remove user code. The default for IL2CPP builds |
| Low | Includes user code in the analysis, but reduces conservatively |
| Medium | Reduces all assemblies, balancing size and safety |
| High | Reduces all assemblies most aggressively |

In Mono builds, the default is Disabled, so no stripping is performed unless you configure it.

In IL2CPP builds, on the other hand, the default is Minimal and Disabled cannot be selected, so stripping is always performed to some degree.

Note that the Unity 6.3 documentation announces the future deprecation of Low, and its use is not recommended.

References: \[Managed code stripping\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping.html) / \[Configure managed code stripping\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-configure.html)

## **Basic Principles**

Managed Code Stripping is implemented by \*\*UnityLinker\*\*, a tool developed in-house by Unity.

UnityLinker runs in the app build pipeline after the C\# code has been compiled, and removes unused code from the compiled assemblies (DLLs).

In IL2CPP builds, the assemblies that have undergone this stripping become the input for the subsequent C++ code conversion.

UnityLinker determines unused code in the following three steps.

1. Determine the code that serves as the starting point of the analysis (\*\*root\*\*)  
2. Recursively follow the types and members statically referenced from the root, and mark them as in use (\*\*mark\*\*)  
3. Regard code that remains unmarked at the end as unreachable and remove it from the assemblies

Typical items treated as roots are as follows.

* Types referenced from scenes and assets included in the build  
* Types that inherit from MonoBehaviour or ScriptableObject  
* Methods with attributes such as \[RuntimeInitializeOnLoadMethod\]  
* Code explicitly preserved with link.xml or the \[Preserve\] attribute (covered in the countermeasures below)

What matters here is that whether code is kept is decided not by whether it is "actually used at runtime" but by whether it is "reachable from a root in build-time static analysis."

Static analysis is analysis that mechanically follows only the references appearing in compiled code, without executing the code.

For example, creating a type with new or calling a method directly leaves the information on the referenced type and member in the compiled code as it is, so UnityLinker can follow it.

On the other hand, code that is invoked in a way that leaves no reference in the code is regarded as unreachable, regardless of whether it is used at runtime.

Therefore, code that has only references invisible to static analysis is judged unused and removed, even if it is needed at runtime.

Also, marking is performed per member rather than per type, so even in a type that remains, methods that are not used are removed individually.

What code is actually removed is examined in the next section with concrete code and measurement results.

### **Checking What Was Stripped with Mono.Cecil**

In the sections that follow, this chapter repeatedly checks which types and methods remain in the build.

For that check, it uses \*\*Mono.Cecil\*\*, a library for analyzing .NET assemblies.

It reads an assembly included in the build output (for a Mono build for macOS, "AppName.app/Contents/Resources/Data/Managed/Assembly-CSharp.dll") and writes the list of types and methods to a file.

CODE START csharp | Outputting the list of types and methods {{list:cecil_dump}}  
// Read the assembly and write out the list of types and methods (including .ctor)  
using var assembly \= AssemblyDefinition.ReadAssembly(dllPath);  
var lines \= assembly.MainModule.Types  
    .Where(type \=\> type.Name \!= "\<Module\>")  
    .SelectMany(type \=\> new\[\] { type.FullName }  
        .Concat(type.Methods.Select(method \=\> $"  {method.Name}")));  
File.WriteAllLines(outputPath, lines);  
CODE END

The method list also includes constructors (.ctor), so it can also be used to check the symptom, described later, in which "the type remains but only the constructor disappears."

Mono.Cecil can be installed and used through the Unity Package Manager. If you enable overrideReferences in an Editor Assembly Definition and add Mono.Cecil.dll to precompiledReferences, it can be referenced from Editor code.

By taking this list from two builds with different Levels and comparing the difference, you can identify which types disappeared and which methods were removed per member.

After a build, the stripped assemblies may remain under Library as intermediate products.

In this environment, DLLs with the same content as the build output remain in Library/Bee/artifacts/MacStandalonePlayerBuildProgram/ManagedStripped/, so you can analyze them without extracting the build artifacts.

The official documentation does not state that stripped assemblies are saved at this path, so if you cannot find them, analyze the build output instead.

Wherever the following sections say "the types and methods were checked," the check was basically done with this method. Comparing the DLLs before and after stripping also reveals the breakdown of the reduction.

Reference: \[Unity linker marking rules\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-marking-rules.html)

## **Why Reduce Code**

The main purpose of using Managed Code Stripping is to reduce app size.

As a secondary effect, unused code is not loaded into memory, so a reduction in runtime memory usage can also be expected.

To see how effective it is, we build a simple scene containing only a single script for quitting, and compare Minimal and High for Managed Stripping Level.

The measurement results are shown below ({{table:size_comparison}}).

表: Comparison of Minimal and High in a simple scene {{table:size_comparison}}

| Metric | Minimal | High | Reduction |
| :---- | :---- | :---- | :---- |
| Total size of managed assemblies | 7,702,016 bytes | 7,188,480 bytes | Approx. 6.7% |
| Number of assemblies (DLLs) | 52 | 51 | 1 |
| Total app size | 89,069,307 bytes | 88,548,299 bytes | Approx. 0.6% |

Even in a small sample project, managed code decreased by 6.7%.

Looking at the breakdown, System.Runtime.Serialization.dll, which is not used anywhere, was removed entirely, mscorlib.dll shrank by about 272 KB, and Assembly-CSharp.dll, which contains much code not referenced from the scene, also shrank by 22.5%.

The amount of managed code reduced is determined by the amount of libraries the project references and the proportion of code within them that is not actually used.

Large projects tend to introduce many libraries, and often many features within those libraries are not actually used, so such projects are likely to benefit more from code reduction.

In IL2CPP builds, the amount of code converted to C++ and compiled decreases by the amount of managed code reduced, so the effect extends to the size of the native binary and to build time.

# **Cautions When Applying Managed Code Stripping**

As seen in the previous section, the decisions of Managed Code Stripping depend on build-time static analysis.

Therefore, code that has only references invisible to static analysis is removed even if it is needed at runtime.

What makes this problem troublesome is that the symptoms appear only in the built app.

It works normally in the Editor and in the default Mono build, where stripping is disabled, so it is hard to notice during development, and the defect surfaces in the form of only a specific feature breaking as soon as the Stripping Level is raised.

This section covers two typical patterns in which more code is stripped than necessary.

One is code invoked through reflection, and the other is code not referenced from Unity entry points.

## **Code Invoked Through Reflection Disappears**

We verify the decision "whether it is reachable from a root in static analysis," explained in the previous section, with concrete code.

Prepare three classes that are referenced in different ways ({{list:probe_targets}}).

CODE START csharp | Three classes referenced in different ways {{list:probe_targets}}  
// Created directly with new  
public class DirectlyUsedService  
{  
    public string Describe() \=\> "DirectlyUsedService is alive";  
}

// Only the type is referenced with typeof  
public class TypeofOnlyService  
{  
    public string Describe() \=\> "TypeofOnlyService is alive";  
}

// Created only through reflection  
public class ReflectionOnlyService  
{  
    public string Describe() \=\> "ReflectionOnlyService is alive";  
}  
CODE END

From a MonoBehaviour placed in the scene, these three are used as follows ({{list:stripping_probe}}).

CODE START csharp | MonoBehaviour that uses the three classes {{list:stripping_probe}}  
public class StrippingProbe : MonoBehaviour  
{  
    private static readonly string ReflectionTypeName \= "ReflectionOnlyService";  
    private static readonly string TypeofTypeName \= "TypeofOnlyService";  
    private static readonly string TypeofMethodName \= "Describe";

    private void Start()  
    {  
        // Static reference: the information on the referenced type and method remains in the compiled code  
        var direct \= new DirectlyUsedService();  
        Debug.Log($"direct: {direct.Describe()}");

        // Static reference to the type only: typeof references the type, but there is no static reference to its members  
        Debug.Log($"typeof: type is referenced: {typeof(TypeofOnlyService).Name}");

        // To check whether the members remain, retrieve the type again by a path invisible to static analysis  
        var typeofType \= Type.GetType(TypeofTypeName);  
        var describeMethod \= typeofType?.GetMethod(TypeofMethodName);  
        if (describeMethod \== null)  
        {  
            Debug.Log($"typeof: type {typeofType?.Name} exists but Describe is stripped");  
        }  
        else  
        {  
            Debug.Log($"typeof: {describeMethod.Invoke(Activator.CreateInstance(typeofType), null)}");  
        }

        // Reference invisible to static analysis: create through reflection  
        var type \= Type.GetType(ReflectionTypeName);  
        if (type \== null)  
        {  
            Debug.Log($"reflection: Type.GetType(\\"{ReflectionTypeName}\\") returned null");  
        }  
        else  
        {  
            var instance \= Activator.CreateInstance(type);  
            Debug.Log($"reflection: {type.GetMethod("Describe")?.Invoke(instance, null)}");  
        }  
    }  
}  
CODE END

When this sample is built and run with Managed Stripping Level changed to Minimal and High, the results are as follows.

CODE START text  
\# Minimal  
direct: DirectlyUsedService is alive  
typeof: type is referenced: TypeofOnlyService  
typeof: TypeofOnlyService is alive  
reflection: ReflectionOnlyService is alive

\# High  
direct: DirectlyUsedService is alive  
typeof: type is referenced: TypeofOnlyService  
typeof: type TypeofOnlyService exists but Describe is stripped  
reflection: Type.GetType("ReflectionOnlyService") returned null  
CODE END

With High, in the typeof case the type can be obtained but GetMethod returns null, and in the reflection case Type.GetType itself returns null.

In summary, the behavior is as follows ({{table:probe_results}}).

表: What remains with Minimal and High {{table:probe_results}}

| Target | Minimal | High |
| :---- | :---- | :---- |
| StrippingProbe | Remains | Remains |
| DirectlyUsedService (referenced with new) | Remains | Both type and method remain |
| TypeofOnlyService (only the type referenced with typeof) | Remains | Type remains, but Describe is removed |
| ReflectionOnlyService (reflection only) | Remains | Removed along with the type |

Minimal does not remove user code, so all three remain as they are.

With High, the difference in how each is referenced appears directly in the result.

DirectlyUsedService references the type with new and also calls Describe directly, so both the type and the method remain.

TypeofOnlyService, which is referenced with typeof, remains as a type, but Describe, which is not called from anywhere, is removed per member.

And ReflectionOnlyService, which has no static reference at all, is removed along with its type.

Note that in this experiment, reading all type-name and method-name strings through static readonly fields is intentional.

UnityLinker understands some typical uses of reflection, and we confirmed that when a constant string is passed directly, as in Type.GetType("ReflectionOnlyService"), the type is preserved even with High.

Also, when GetMethod was called directly on a Type obtained with typeof, all public methods of that type were preserved even if the method name was not a constant.

On the other hand, UnityLinker does not track strings passed through a field.

Reading from a field reproduces the real-world situation in which type names and member names come from paths not determined at build time, such as configuration files, serialized data, and server responses.

If you make the field const, it is embedded as a constant string at the use site, so it is treated the same as passing a constant directly.

### Type Resolution in DI Containers Breaks

The examples so far are simplified for explanation, but the same structure of problem occurs in real libraries. A typical case is a DI container.

Many DI containers register the mapping between interfaces and implementation types, and at resolution time create an instance by invoking the constructor through reflection.

We verify this with a minimal container that imitates this behavior. As in real DI containers, it uses an interface that is registered with Register and resolved with Resolve ({{list:mini_container}}).

CODE START csharp | Minimal DI container {{list:mini_container}}  
public sealed class MiniContainer  
{  
    private readonly Dictionary\<Type, Type\> \_registrations \= new Dictionary\<Type, Type\>();

    // Register the implementation type with a type argument. TImplementation becomes a static reference equivalent to typeof  
    public void Register\<TInterface, TImplementation\>() where TImplementation : TInterface  
    {  
        \_registrations\[typeof(TInterface)\] \= typeof(TImplementation);  
    }

    // Instantiate the registered implementation type through reflection and return it  
    public TInterface Resolve\<TInterface\>()  
    {  
        var implementationType \= \_registrations\[typeof(TInterface)\];  
        return (TInterface)Activator.CreateInstance(implementationType);  
    }  
}  
CODE END

The registered implementation type is assumed to be used only through the container and is not statically referenced anywhere ({{list:notification_service}}).

CODE START csharp | Registered interface and implementation {{list:notification_service}}  
public interface INotificationService  
{  
    string Notify();  
}

// An implementation used only through the container and not statically referenced anywhere  
public sealed class NotificationService : INotificationService  
{  
    public string Notify() \=\> "NotificationService.Notify succeeded";  
}  
CODE END

A MonoBehaviour placed in the scene performs registration and resolution ({{list:di_probe}}).

CODE START csharp | MonoBehaviour that registers and resolves {{list:di_probe}}  
public sealed class DiContainerProbe : MonoBehaviour  
{  
    private void Start()  
    {  
        try  
        {  
            var container \= new MiniContainer();  
            container.Register\<INotificationService, NotificationService\>();  
            Debug.Log($"\[DiContainerProbe\] explicit: {container.Resolve\<INotificationService\>().Notify()}");  
        }  
        catch (Exception exception)  
        {  
            Debug.Log($"\[DiContainerProbe\] explicit failed: {exception.GetType().Name}: {exception.Message}");  
        }  
    }  
}  
CODE END

When this is built under the same conditions as before, the results are as follows.

CODE START text  
\# Minimal  
\[DiContainerProbe\] explicit: NotificationService.Notify succeeded

\# High  
\[DiContainerProbe\] explicit failed: MissingMethodException: Default constructor not found for type NotificationService  
CODE END

With High, the type argument of Register\<INotificationService, NotificationService\>() becomes a static reference, so the NotificationService type itself remains.

Notify, which is called statically through the interface, also remains.

However, the default constructor, which is called only through reflection, has no static reference and is removed per member.

As a result, Activator.CreateInstance cannot find the constructor, and resolution fails with MissingMethodException.

This is the cause of the defect in which resolution fails only in the built app even though the type is registered.

The structure is the same in real DI containers.

As long as constructor resolution relies on reflection, the implicit default constructor is a candidate for removal even for a type that is explicitly registered.

With a method that scans assemblies for implementation types and registers them automatically, implementation types that have no static reference are removed along with their types, and the registration itself fails.

## **Code Not Referenced from Unity Entry Points Disappears**

Another pattern is code that is referenced from neither scenes nor scripts and is needed only through external content such as AssetBundles.

As seen in the previous section, the sources of roots are the scenes and assets included in the build.

Content built separately as an AssetBundle is not included in the build of the app itself, so the types that content references are not treated as roots.

Therefore, types needed only by the content are judged unused unless the app code has a static reference to them, regardless of whether they are loaded at runtime.

We reproduce this pattern with a Timeline asset.

The Timeline asset is included only in the AssetBundle, and the app does not statically reference Unity.Timeline; it loads the asset by name and plays it with PlayableDirector ({{list:timeline_probe}}).

CODE START csharp | MonoBehaviour that plays a Timeline asset from an AssetBundle {{list:timeline_probe}}  
public class TimelineAssetBundleProbe : MonoBehaviour  
{  
    private const string BundleDirectoryName \= "TimelineBundles";  
    private const string BundleName \= "timeline.bundle";  
    private const string TimelineAssetName \= "TimelineOnly";

    private void Start()  
    {  
        var bundlePath \= Path.Combine(Application.streamingAssetsPath, BundleDirectoryName, BundleName);  
        var bundle \= AssetBundle.LoadFromFile(bundlePath);  
        if (bundle \== null)  
        {  
            Debug.Log("timeline bundle: failed to load asset bundle");  
            return;  
        }

        var asset \= bundle.LoadAsset(TimelineAssetName);  
        if (asset \== null)  
        {  
            Debug.Log($"timeline bundle: asset {TimelineAssetName} failed to load");  
            bundle.Unload(true);  
            return;  
        }

        Debug.Log($"timeline bundle: loaded {asset.GetType().AssemblyQualifiedName}");  
        var playableAsset \= asset as PlayableAsset;  
        if (playableAsset \== null)  
        {  
            Debug.Log($"timeline bundle: asset {TimelineAssetName} is not playable");  
            bundle.Unload(true);  
            return;  
        }

        var director \= gameObject.AddComponent\<PlayableDirector\>();  
        director.playableAsset \= playableAsset;  
        director.Play();  
        Debug.Log($"timeline bundle: play state={director.state}, graph valid={director.playableGraph.IsValid()}");  
        bundle.Unload(false);  
    }  
}  
CODE END

By deleting the original Timeline asset from the project after creating the AssetBundle and then building the app, this imitates a setup in which content is updated separately from the app.

Build with Managed Stripping Level changed to Minimal and High. Both levels give the same result.

CODE START text  
\# Both Minimal and High  
The referenced script (UnityEngine.Timeline.TimelineAsset) on this Behaviour is missing\!  
timeline bundle: asset TimelineOnly failed to load  
CODE END

Also, when we checked the Managed folder of the built app, Unity.Timeline.dll itself did not exist in either build.

Note that the result does not differ between Minimal and High.

The countermeasure for this pattern is not to adjust the Stripping Level but to explicitly specify the types and assemblies to preserve.

The means for this are covered in the next section, and the mechanism when using Addressables is covered in the advanced section.

References: \[How code stripping affects content\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-content.html) / \[A case where a Timeline asset disappears (Qiita, 2021)\](https://qiita.com/KyoheiOkawa/items/0cf9cc09679eababec75)

# **Countermeasures for Over-Stripping**

## **Basic Countermeasures**

The most basic way to prevent necessary code from being removed is to explicitly tell UnityLinker that "this code is used."

Unity provides the link.xml file and the \[Preserve\] attribute for this purpose.

Code specified by either is treated as a root of the analysis and is not removed even without a static reference.

### **link.xml**

link.xml is an XML file that lists the assemblies, types, and members to preserve.

If you place it under the project's Assets, UnityLinker reads it at build time.

ReflectionOnlyService, which was removed along with its type in the previous sample, can be preserved with the following link.xml ({{list:linkxml_reflection}}).

CODE START xml | link.xml that preserves ReflectionOnlyService {{list:linkxml_reflection}}  
\<linker\>  
  \<assembly fullname="Assembly-CSharp"\>  
    \<type fullname="ReflectionOnlyService" preserve="all" /\>  
  \</assembly\>  
\</linker\>  
CODE END

When this link.xml was placed directly under Assets and the app was rebuilt with High, the execution result changed as follows.

CODE START text  
\# High (without link.xml)  
reflection: Type.GetType("ReflectionOnlyService") returned null

\# High (with link.xml)  
reflection: ReflectionOnlyService is alive  
CODE END

Checking the assemblies included in the build also confirms that ReflectionOnlyService is preserved along with Describe.

link.xml specifies what to preserve outside the code, so it can also be used for types in DLLs and packages that you cannot edit yourself.

The preserved scope can be specified per assembly, per type, or per member such as a method or field.

Note that preserving an entire assembly with preserve="all" is convenient, but it cancels out the effect of stripping on that assembly.

It can be used to isolate a problem, but for a permanent fix it is better to narrow down to the necessary types and members.

### **\[Preserve\] Attribute**

For code you can edit yourself, you can attach the \[Preserve\] attribute of the UnityEngine.Scripting namespace directly to the target.

Note that the scope preserved by \[Preserve\] changes depending on where it is attached.

We verify this difference with two classes: one with the attribute on the type, and one with it only on the method ({{list:type_preserved}}, {{list:method_preserved}}).

CODE START csharp | When Preserve is attached to the type {{list:type_preserved}}  
using UnityEngine.Scripting;

// Attach \[Preserve\] to the type. Do not attach it to the method  
\[Preserve\]  
public sealed class TypePreservedService  
{  
    public string Describe() \=\> "TypePreservedService.Describe is alive";  
}  
CODE END

CODE START csharp | When Preserve is attached only to the method {{list:method_preserved}}  
using UnityEngine.Scripting;

// Do not attach it to the type; attach \[Preserve\] only to the method  
public sealed class MethodPreservedService  
{  
    \[Preserve\]  
    public string Describe() \=\> "MethodPreservedService.Describe is alive";  
}  
CODE END

When these two were created and invoked through reflection without any static reference, the results with High were as follows ({{table:preserve_results}}).

表: Position of Preserve and the scope preserved with High {{table:preserve_results}}

| Target | Type | Default constructor | Describe |
| :---- | :---- | :---- | :---- |
| TypePreservedService | Remains | Remains | Removed |
| MethodPreservedService | Remains | Removed | Remains |

\[Preserve\] attached to a type preserves the type and its default constructor, but does not preserve Describe, which has no attribute.

\[Preserve\] attached to a method preserves that type and method, but this time the default constructor is not preserved, and creation with Activator.CreateInstance fails with MissingMethodException.

To make it work all the way through creating and invoking via reflection, you must attach \[Preserve\] to both the type and the method.

If "it does not work even though I attached \[Preserve\]," first suspect the correspondence between the position where the attribute is attached and the preserved scope.

Reference: \[Preserving code using annotations\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-preserving.html)

## **Advanced Topics**

###  **Using Attributes Other Than Preserve**

Whereas \[Preserve\] specifies a single point, "keep this element," the UnityEngine.Scripting namespace also has attributes that declare preservation by pattern, such as "keep all implementations of this interface."

In a structure common in frameworks, such as "find types implemented by the user through reflection and invoke them," you can handle this by attaching an attribute to the interface or base class, instead of specifying each implementation type with \[Preserve\].

* \[RequireImplementorsAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireImplementorsAttribute.html): when attached to an interface, preserves the types that implement that interface](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireImplementorsAttribute.html\): when attached to an interface, preserves the types that implement that interface)  
* \[RequireDerivedAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireDerivedAttribute.html): when attached to a class, preserves the types that inherit from that type](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireDerivedAttribute.html\): when attached to a class, preserves the types that inherit from that type)  
* \[RequiredMemberAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredMemberAttribute.html): when attached to a member, also preserves that member if its type remains](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredMemberAttribute.html\): when attached to a member, also preserves that member if its type remains)  
* \[RequiredInterfaceAttribute\](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredInterfaceAttribute.html): when attached to a type, preserves the implementation information of the specified interface

As a representative, we verify the behavior of \[RequireImplementors\].

The attribute is attached to the interface, and the implementation is not statically referenced anywhere ({{list:require_implementors}}).

CODE START csharp | Interface with RequireImplementors attached, and its implementation {{list:require_implementors}}  
// When attached to the interface, the implementation types become targets of preservation  
\[RequireImplementors\]  
public interface IRequiredContract  
{  
}

// An implementation not statically referenced anywhere  
public sealed class RequiredContractImpl : IRequiredContract  
{  
}  
CODE END

When built with High, RequiredContractImpl was preserved along with its default constructor.

An implementation type with no static reference remains solely through the declaration on the interface side.

Preservation was also confirmed in the same way for derived types of a base class with \[RequireDerived\] attached.

### **Generating link.xml at Build Time with IUnityLinkerProcessor**

When the code to preserve is known in advance, simply specifying the target classes and members in link.xml is sufficient.

However, in situations where the types to preserve differ by build target, or the necessary types change depending on the content included in the build, continually updating link.xml by hand is not realistic.

With \*\*IUnityLinkerProcessor\*\*, you can dynamically generate link.xml at build time and pass it to UnityLinker.

IUnityLinkerProcessor is an Editor interface, and if you place the implementation class in an Editor assembly, it is called automatically at build time.

The link.xml at the path returned by GenerateAdditionalLinkXmlFile is added to the input of UnityLinker.

The minimal implementation is as follows ({{list:linker_processor}}).

CODE START csharp | Minimal IUnityLinkerProcessor implementation that generates link.xml {{list:linker_processor}}  
using System.IO;  
using UnityEditor.Build;  
using UnityEditor.Build.Reporting;  
using UnityEditor.UnityLinker;  
using UnityEngine;

public sealed class PreserveListLinkerProcessor : IUnityLinkerProcessor  
{  
    private const string LinkXmlContent \= @"\<linker\>  
  \<assembly fullname=""Assembly-CSharp""\>  
    \<type fullname=""GeneratedPreserveService"" preserve=""all"" /\>  
  \</assembly\>  
\</linker\>  
";

    public int callbackOrder \=\> 0;

    // The link.xml at the returned path is added to the input of UnityLinker  
    public string GenerateAdditionalLinkXmlFile(BuildReport report, UnityLinkerBuildPipelineData data)  
    {  
        var path \= Path.Combine(Path.GetTempPath(), "managed-stripping-sample-generated-link.xml");  
        File.WriteAllText(path, LinkXmlContent);  
        Debug.Log("\[PreserveListLinkerProcessor\] generated 1 assembly / 1 type.");  
        return path;  
    }  
}  
CODE END

In this example, the generated content is fixed for explanation, but in practice you would replace this part with processing that builds it from build-time input, such as reading a settings asset or analyzing content dependencies.

When we prepared GeneratedPreserveService, which has no static reference, and built with High, we confirmed that the type, constructor, and method were preserved even though no link.xml was placed under Assets.

CODE START text  
\[GeneratedLinkXmlProbe\] type=present  
\[GeneratedLinkXmlProbe\] ctor=present, method=GeneratedPreserveService.Describe is alive  
CODE END

Reference: \[IUnityLinkerProcessor\](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Build.IUnityLinkerProcessor.html)

###  **link.xml Generated by Addressables**

Addressables addresses the problem seen in the previous section, in which "types needed only by the content disappear," by automatically generating link.xml as an artifact of the content build.

We check what is actually generated. The content is kept minimal: we create one prefab that only displays a Cube mesh (a GameObject with MeshFilter and MeshRenderer attached) and register it in an Addressables group.

Running the content build generates link.xml.

In Addressables 4.0.1, the output location is Library/com.unity.addressables/aa/\<platform\>/AddressablesLink/link.xml.

The content generated this time was as follows ({{list:addressables_linkxml}}).

CODE START xml | link.xml generated by Addressables {{list:addressables_linkxml}}  
\<linker\>  
  \<assembly fullname="Unity.Addressables, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null" preserve="all"\>  
    \<type fullname="UnityEngine.AddressableAssets.Addressables" preserve="all" /\>  
  \</assembly\>  
  \<assembly fullname="Unity.ResourceManager, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null" preserve="all"\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.AssetBundleProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.BundledAssetProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.IAssetBundleResource" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.InstanceProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.SceneProvider" preserve="all" /\>  
  \</assembly\>  
  \<assembly fullname="UnityEngine.CoreModule, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null"\>  
    \<type fullname="UnityEngine.GameObject" preserve="all" /\>  
    \<type fullname="UnityEngine.MeshFilter" preserve="all" /\>  
    \<type fullname="UnityEngine.MeshRenderer" preserve="all" /\>  
    \<type fullname="UnityEngine.Transform" preserve="all" /\>  
  \</assembly\>  
\</linker\>  
CODE END

The content falls into two kinds.

One is the provider types of the Addressables runtime (Unity.Addressables and Unity.ResourceManager), which become necessary depending on the group configuration.

The other is the types referenced by the registered content. This time, the GameObject, Transform, MeshFilter, and MeshRenderer that make up the prefab are listed as preservation targets as they are.

This shows that Addressables analyzes the dependencies on the content side and includes types in the preservation targets even when the app code does not reference them.

Reference: \[Addressables Build Artifacts\](https://docs.unity3d.com/ja/Packages/com.unity.addressables@1.20/manual/BuildArtifacts.html) (the description of link.xml is in the 1.20 manual)

### **Replacing Code Invocation via Reflection with Static Calls by a Source Generator**

All the countermeasures so far specify preservation so that necessary code is not removed.

In contrast, there is also a method of replacing the reflection itself, which creates dependencies invisible to static analysis, with code generation by a \*\*Source Generator\*\*.

A Source Generator is a mechanism in which the C\# compiler analyzes source code at compile time and generates additional source code.

If type discovery and creation are completed at compile time, the dependencies are expressed as static references from the start, and the preservation specification itself becomes unnecessary.

As an example, consider dispatch processing that receives a command name string and executes the corresponding handler.

Handlers declare their command names with an attribute. Creating them with reflection at runtime would mean searching assemblies for attributed types and creating them ({{list:command_handlers}}).

CODE START csharp | Handlers that declare command names with an attribute {{list:command_handlers}}  
\[AttributeUsage(AttributeTargets.Class)\]  
public sealed class CommandHandlerAttribute : Attribute  
{  
    public CommandHandlerAttribute(string command)  
    {  
        Command \= command;  
    }

    public string Command { get; }  
}

public interface ICommandHandler  
{  
    string Execute();  
}

// The handlers are not statically referenced anywhere  
\[CommandHandler("greet")\]  
public sealed class GreetCommandHandler : ICommandHandler  
{  
    public string Execute() \=\> "GreetCommandHandler.Execute succeeded";  
}

\[CommandHandler("farewell")\]  
public sealed class FarewellCommandHandler : ICommandHandler  
{  
    public string Execute() \=\> "FarewellCommandHandler.Execute succeeded";  
}  
CODE END

The two handlers are not statically referenced anywhere, so if the app is built with High while keeping the runtime-scan design, they are removed along with their types, as seen in the previous section.

We implement the resolution of handlers for these command names with a Source Generator.

We prepare a generator that collects classes with \[CommandHandler\] at compile time and generates a dispatcher that looks up a concrete type from a command string.

The generator works in two stages: the first (ForAttributeWithMetadataName) collects command names and type names from attributed classes, and the second (GenerateDispatcher) pours the collected pairs into a template.

The full code of the generator is shown below ({{list:dispatcher_generator}}).

CODE START csharp | Source Generator that generates the dispatcher {{list:dispatcher_generator}}  
\#nullable enable  
using System.Collections.Immutable;  
using System.Linq;  
using System.Text;  
using Microsoft.CodeAnalysis;  
using Microsoft.CodeAnalysis.CSharp;  
using Microsoft.CodeAnalysis.Text;  
namespace StrippingSample.SourceGenerators  
{  
    \[Generator\]  
    public sealed class CommandDispatcherGenerator : IIncrementalGenerator  
    {  
        public void Initialize(IncrementalGeneratorInitializationContext context)  
        {  
            // Collect pairs of command names and type names from classes with \[CommandHandler\]  
            var handlers \= context.SyntaxProvider  
                .ForAttributeWithMetadataName(  
                    "CommandHandlerAttribute",  
                    static (\_, \_) \=\> true,  
                    static (syntaxContext, \_) \=\> (  
                        Command: (string)syntaxContext.Attributes\[0\].ConstructorArguments\[0\].Value\!,  
                        TypeName: ((INamedTypeSymbol)syntaxContext.TargetSymbol)  
                            .ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat)))  
                .Collect();  
            // Generate the dispatcher from the collected pairs  
            context.RegisterSourceOutput(handlers, static (sourceContext, commandHandlers) \=\>  
            {  
                if (\!commandHandlers.IsDefaultOrEmpty)  
                {  
                    sourceContext.AddSource(  
                        "CommandDispatcher.g.cs",  
                        SourceText.From(GenerateDispatcher(commandHandlers), Encoding.UTF8));  
                }  
            });  
        }  
        private static string GenerateDispatcher(  
            ImmutableArray\<(string Command, string TypeName)\> handlers)  
        {  
            // To make the output deterministic, sort by command name before pouring them in  
            var cases \= string.Join("\\n", handlers  
                .Distinct()  
                .OrderBy(static handler \=\> handler.Command, System.StringComparer.Ordinal)  
                .Select(static handler \=\> $$"""  
                            case {{SymbolDisplay.FormatLiteral(handler.Command, true)}}:  
                                return new {{handler.TypeName}}().Execute();  
                """));  
            return $$"""  
                // \<auto-generated/\>  
                public static class CommandDispatcher  
                {  
                    public static string Execute(string command)  
                    {  
                        switch (command)  
                        {  
                {{cases}}  
                            default:  
                                throw new global::System.ArgumentException($"Unknown command: {command}");  
                        }  
                    }  
                }  
                """;  
        }  
    }  
}  
CODE END

The sorting by command name ensures that the same input generates the same code, so that the output does not fluctuate from build to build.

With this generator, code like the following is generated at compile time ({{list:generated_dispatcher}}).

CODE START csharp | Generated CommandDispatcher {{list:generated_dispatcher}}  
// \<auto-generated/\>  
public static class CommandDispatcher  
{  
    public static string Execute(string command)  
    {  
        switch (command)  
        {  
            case "farewell":  
                return new global::FarewellCommandHandler().Execute();  
            case "greet":  
                return new global::GreetCommandHandler().Execute();  
            default:  
                throw new global::System.ArgumentException($"Unknown command: {command}");  
        }  
    }  
}  
CODE END

The processing that looked up types from strings at runtime has become a table mapping strings to concrete types.

Both construction (new) and the Execute call appear in the generated code, so static references to the type, constructor, and method are self-contained within the generated code.

The calling side only passes a command string and never references the concrete types of the handlers.

As in the reflection experiment, the command string is passed through a static readonly field, keeping it an input that UnityLinker cannot track ({{list:dispatch_probe}}).

CODE START csharp | Calling side of the generated dispatcher {{list:dispatch_probe}}  
// Pass the command string through a field to make it an input that UnityLinker cannot track  
private static readonly string\[\] Commands \= { "farewell", "greet" };

private static void RunGeneratedDispatcher()  
{  
    foreach (var command in Commands)  
    {  
        var result \= CommandDispatcher.Execute(command);  
        Debug.Log($"\[SourceGeneratorProbe\] dispatch command={command}, result={result}");  
    }  
}  
CODE END

When this was built with High and run, we confirmed that both commands executed successfully.

CODE START text  
\[SourceGeneratorProbe\] dispatch command=farewell, result=FarewellCommandHandler.Execute succeeded  
\[SourceGeneratorProbe\] dispatch command=greet, result=GreetCommandHandler.Execute succeeded  
CODE END

The string itself remains an input that cannot be tracked at build time, but the mapping from string to type is now a switch in generated code rather than a runtime search, so to UnityLinker it looks like an ordinary static reference, no different from handwritten code.

A check with Mono.Cecil also showed that the types, constructors, and Execute of the two handlers, as well as CommandDispatcher.Execute, were preserved.

The benefit of this replacement is not limited to resistance to stripping.

\- Because the dependencies are visible to UnityLinker, maintaining preservation specifications with link.xml or attributes becomes unnecessary  
\- Runtime reflection searches are eliminated, which reduces startup cost and the possibility of runtime failures  
\- Because targets are collected at compile time, missing registrations and duplicates can be diagnosed at compile time

Some real libraries also adopt such an implementation.

Among serializers, MessagePack for C\# has long provided code generation for IL2CPP environments and has now moved to Source Generator-based generation.

Among DI containers, VContainer offers an option to generate injection code with a Source Generator and reduce runtime reflection, and it resolves the symptom seen in the previous section, "the constructor disappears and resolution fails," with a Source Generator.

For how to introduce a Source Generator into a Unity project (a setup that places a netstandard2.0 DLL with the RoslynAnalyzer label), see the Unity manual.

References: \[Create and use a source generator\](https://docs.unity3d.com/6000.3/Documentation/Manual/create-source-generator.html) / \[MessagePack for C\#\](https://github.com/MessagePack-CSharp/MessagePack-CSharp) / \[MemoryPack\](https://github.com/Cysharp/MemoryPack) / \[VContainer\](https://github.com/hadashiA/VContainer)

# **Summary**

Managed Code Stripping is not an optimization that you can finish by raising the Stripping Level.

As this chapter has shown, dependencies through reflection or external content are invisible to static analysis, and code that is needed at runtime disappears from the build.

Moreover, because the symptoms appear only in the built app, you need to operate two practices together: making invisible dependencies explicit, and continuing to verify with the built app.

The means of specifying preservation are used selectively according to the target and situation ({{table:approach_selection}}).

表: Choosing among the countermeasures {{table:approach_selection}}

| Method | Suitable situation |
| :---- | :---- |
| \[Preserve\] | Preserve a small number of elements in code you can edit yourself |
| link.xml | Preserve fixed targets, including DLLs you cannot edit |
| Require-family attributes | Declare a contract on the framework side to "keep implementations or derived types" |
| IUnityLinkerProcessor | The preservation targets vary by build target or content |
| Source Generator | Replace the reflection-based search itself with static references |

In addition, the following are policies for making stripping more effective.

* Except in cases where it is a problem, do not overuse \[Preserve\] and link.xml  
* Reduce type discovery and creation through reflection, and consider switching to static references through explicit registration or a Source Generator  
* Narrow code preservation specifications down to the type or member level wherever possible. Limit preserve="all" for an entire assembly to isolating problems  
* For preserving types used only by content built separately from the app, use mechanisms such as Addressables' link.xml or generation by IUnityLinkerProcessor rather than doing it by hand  
* Run and verify builds with a raised Stripping Level from the middle of development
