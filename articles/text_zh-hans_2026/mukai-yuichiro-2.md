# **Unity Managed Code Stripping 实践指南**

Mukai Yuichiro / 所属: 株式会社 Applibot  
yucchiy\_  / blog.yucchiy.com / yucchiy  
\---

作为削减包体大小的手段，本文介绍 Unity 中在构建时移除未使用代码的 Managed Code Stripping 功能。  
它只需一项设置即可启用，十分便捷，但如果加大削减力度，可能会引发“在编辑器中能正常运行，却只在构建后的应用中特定功能损坏”的问题。  
本章介绍 Managed Code Stripping 的机制与用法、代码被过度削减的典型模式及其应对方法等。

验证环境如下。

*  Unity 6000.3.19f1  
* Addressables 4.0.1  
* macOS Sequoia 15.7.3

先说结论：Managed Code Stripping 并不是把设置调到 High 就万事大吉的优化。

只有配合显式声明静态分析看不到的依赖，并持续验证构建后应用的大小与行为的工作流程，才能安全地使用这一功能。

# **什么是 Managed Code Stripping**

## **功能概述**

Managed Code Stripping 是 Unity 的标准功能，它在构建时检测未使用的托管代码（C\# 代码），并将其从构建后的应用中移除。

通过移除场景和脚本未使用的类与方法，可以削减应用的代码大小。

削减范围不仅限于用户代码，还包括 UnityEngine 的模块、.NET 的类库，以及项目中包含的包和插件的程序集，因此连自己难以修改的库内部的未使用代码也能一并削减。

尤其在 IL2CPP 构建中，托管代码被削减后，之后由 IL2CPP 生成的 C++ 代码也随之减少，因此还有助于缩小包体大小和缩短构建时间。

另外，有一个名称相近的设置 Strip Engine Code，但它是针对 Unity 引擎原生代码的削减设置，与针对托管代码的 Managed Code Stripping 是不同的功能。

### **\#\#\# Managed Stripping Level**

削减代码的积极程度由名为 Managed Stripping Level 的设置控制。

可在 Edit \> Project Settings \> Player \> Other Settings \> Optimization 的“Managed Stripping Level”中设置，可选择下列级别。

表: Managed Stripping Level 的选项 {{table:stripping_levels}}

| Level | 概述 |
| :---- | :---- |
| Disabled | 不进行 stripping。仅在 Mono 构建中可选 |
| Minimal | 仅对 UnityEngine 和 .NET 的类库进行削减，不删除用户代码。IL2CPP 构建的默认值 |
| Low | 用户代码也纳入分析范围，但保守地削减 |
| Medium | 以全部程序集为范围，在大小与安全性之间取得平衡进行削减 |
| High | 对全部程序集进行最积极的削减 |

Mono 构建的默认值是 Disabled，因此不做任何设置就不会进行 stripping。

另一方面，IL2CPP 构建的默认值是 Minimal，且无法选择 Disabled，因此无论程度如何，必定会进行 stripping。

另外，Unity 6.3 的文档已预告 Low 将来会被废弃，不推荐使用。

参考：\[Managed code stripping\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping.html) / \[Configure managed code stripping\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-configure.html)

## **基本工作原理**

Managed Code Stripping 由 Unity 自研的工具 \*\*UnityLinker\*\* 实现。

UnityLinker 在应用的构建管线中、C\# 代码编译完成后执行，从编译好的程序集（DLL）中移除未使用代码。

在 IL2CPP 构建中，经过这一 stripping 的程序集会成为后续 C++ 代码转换的输入。

UnityLinker 通过以下三个阶段判定未使用代码。

1. 确定作为分析起点的代码（\*\*root\*\*）  
2. 从 root 开始递归追踪被静态引用的类型和成员，并标记为使用中（\*\*mark\*\*）  
3. 将最终未被标记的代码视为不可达，从程序集中移除

被视为 root 的典型代码如下。

* 构建中包含的场景和资源所引用的类型  
* 继承自 MonoBehaviour 或 ScriptableObject 的类型  
* 带有 \[RuntimeInitializeOnLoadMethod\] 等特性的方法  
* 通过 link.xml 或 \[Preserve\] 特性显式指定保留的代码（在后文的应对方法中说明）

这里的关键在于，代码是否保留并不取决于“运行时是否实际被使用”，而是取决于“构建时的静态分析能否从 root 到达”。

所谓静态分析，是指不执行代码，仅机械地追踪编译后代码中出现的引用的分析方式。

例如，通过 new 创建类型或直接调用方法时，被引用类型和成员的信息会原样保留在编译后的代码中，因此 UnityLinker 能够追踪。

另一方面，以代码中不出现引用的形式被调用的代码，无论运行时是否被使用，都会被视为不可达。

因此，只拥有静态分析看不到的引用的代码，即使运行时需要，也会被判定为未使用而删除。

此外，mark 是以成员为单位而非以类型为单位进行的，因此即使类型被保留，其中未使用的方法也会被逐个删除。

实际上哪些代码会被删除，将在下一节结合具体代码和测量结果进行确认。

### **用 Mono.Cecil 确认哪些代码被删除**

本章在后续各节中会反复确认“哪些类型和方法留在了构建中”。

这一确认使用的是用于分析 .NET 程序集的库 \*\*Mono.Cecil\*\*。

读取构建输出中包含的程序集（面向 macOS 的 Mono 构建为“应用名.app/Contents/Resources/Data/Managed/Assembly-CSharp.dll”），并将类型和方法的列表写入文件。

CODE START csharp | 输出类型和方法列表 {{list:cecil_dump}}  
// 读取程序集，写出类型和方法（含 .ctor）的列表  
using var assembly \= AssemblyDefinition.ReadAssembly(dllPath);  
var lines \= assembly.MainModule.Types  
    .Where(type \=\> type.Name \!= "\<Module\>")  
    .SelectMany(type \=\> new\[\] { type.FullName }  
        .Concat(type.Methods.Select(method \=\> $"  {method.Name}")));  
File.WriteAllLines(outputPath, lines);  
CODE END

方法列表中也包含构造函数（.ctor），因此也可用于确认后文所述“类型还在，只有构造函数消失”的症状。

Mono.Cecil 可通过 Unity Package Manager 安装使用。在编辑器用的 Assembly Definition 中启用 overrideReferences，并在 precompiledReferences 中添加 Mono.Cecil.dll，即可从编辑器代码中引用。

对改变 Level 的两个构建分别获取这一列表并比较差异，就能确定哪些类型消失了、哪些方法以成员为单位被删除了。

构建后，stripping 之后的程序集有时会作为中间产物留在 Library 目录下。

在本次环境中，Library/Bee/artifacts/MacStandalonePlayerBuildProgram/ManagedStripped/ 下留有与构建输出内容相同的 DLL，无需解开构建产物即可将其作为分析目标。

官方文档并未明确说明 stripping 后的程序集会保存到该路径，因此如果找不到，就分析构建输出一侧。

后续各节中写到“确认了类型和方法”的地方，基本都是通过这一方法确认的。比较 stripping 前后的 DLL，还能确认削减的具体构成。

参考：\[Unity linker marking rules\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-marking-rules.html)

## **为什么要削减代码**

使用 Managed Code Stripping 的主要目的是削减包体大小。

附带的好处是，未使用代码不会加载到内存中，因此也可期待运行时内存占用的削减。

为了解效果有多大，我们构建一个仅放置了一个退出处理脚本的简单场景，比较 Managed Stripping Level 为 Minimal 和 High 时的结果。

测量结果如下（{{table:size_comparison}}）。

表: 简单场景下 Minimal 与 High 的比较 {{table:size_comparison}}

| 指标 | Minimal | High | 削减 |
| :---- | :---- | :---- | :---- |
| 托管程序集总大小 | 7,702,016 bytes | 7,188,480 bytes | 约 6.7% |
| 程序集（DLL）数量 | 52 | 51 | 1 个 |
| 应用整体大小 | 89,069,307 bytes | 88,548,299 bytes | 约 0.6% |

即使是小型示例项目，托管代码也减少了 6.7%。

从具体构成来看，没有任何地方使用的 System.Runtime.Serialization.dll 被整个删除，mscorlib.dll 减少了约 272KB，包含大量未被场景引用的代码的 Assembly-CSharp.dll 也减少了 22.5%。

托管代码的削减量取决于项目引用的库的数量，以及其中实际未使用代码所占的比例。

大型项目往往会引入许多库，而库中实际用不到的功能常常也很多，因此更容易从代码削减中获益。

在 IL2CPP 构建中，托管代码削减多少，需要转换为 C++ 并编译的代码就减少多少，因此效果还会波及原生二进制文件的大小和构建时间。

# **应用 Managed Code Stripping 时的注意事项**

如前一节所述，Managed Code Stripping 的判断依赖于构建时的静态分析。

因此，只拥有静态分析看不到的引用的代码，即使运行时需要，也会被删除。

这个问题的麻烦之处在于，症状只会在构建后的应用中出现。

在编辑器中运行，或在未启用 stripping 的默认 Mono 构建中，都能正常工作，因此开发过程中很难察觉；问题会以“一提高 Stripping Level，特定功能就突然损坏”的形式暴露出来。

本节讨论代码被过度削减的两种典型模式。

一种是通过反射调用的代码，另一种是未被 Unity 入口点引用的代码。

## **通过反射调用的代码消失**

用具体代码来确认前一节所说的“静态分析能否从 root 到达”这一判断。

准备三个被引用方式各不相同的类（{{list:probe_targets}}）。

CODE START csharp | 被引用方式各不相同的三个类 {{list:probe_targets}}  
// 通过 new 直接创建  
public class DirectlyUsedService  
{  
    public string Describe() \=\> "DirectlyUsedService is alive";  
}

// 仅通过 typeof 引用类型  
public class TypeofOnlyService  
{  
    public string Describe() \=\> "TypeofOnlyService is alive";  
}

// 仅通过反射创建  
public class ReflectionOnlyService  
{  
    public string Describe() \=\> "ReflectionOnlyService is alive";  
}  
CODE END

从放置在场景中的 MonoBehaviour 按如下方式使用这三个类（{{list:stripping_probe}}）。

CODE START csharp | 使用三个类的 MonoBehaviour {{list:stripping_probe}}  
public class StrippingProbe : MonoBehaviour  
{  
    private static readonly string ReflectionTypeName \= "ReflectionOnlyService";  
    private static readonly string TypeofTypeName \= "TypeofOnlyService";  
    private static readonly string TypeofMethodName \= "Describe";

    private void Start()  
    {  
        // 静态引用：被引用类型和方法的信息保留在编译后的代码中  
        var direct \= new DirectlyUsedService();  
        Debug.Log($"direct: {direct.Describe()}");

        // 静态引用仅限类型：typeof 引用了类型，但没有对成员的静态引用  
        Debug.Log($"typeof: type is referenced: {typeof(TypeofOnlyService).Name}");

        // 通过静态分析看不到的途径重新获取类型，确认成员是否保留  
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

        // 静态分析看不到的引用：通过反射创建  
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

将 Managed Stripping Level 分别设为 Minimal 和 High 构建并运行此示例，结果如下。

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

在 High 下，typeof 的情况能获取类型，但 GetMethod 返回 null；反射的情况则是 Type.GetType 本身返回 null。

总结起来，行为如下（{{table:probe_results}}）。

表: Minimal 与 High 下的保留情况 {{table:probe_results}}

| 类 | Minimal | High |
| :---- | :---- | :---- |
| StrippingProbe | 保留 | 保留 |
| DirectlyUsedService（通过 new 引用） | 保留 | 类型和方法都保留 |
| TypeofOnlyService（仅通过 typeof 引用类型） | 保留 | 类型保留，但 Describe 被删除 |
| ReflectionOnlyService（仅通过反射） | 保留 | 整个类型被删除 |

Minimal 不删除用户代码，因此三个类都原样保留。

在 High 下，被引用方式的差异直接体现在结果中。

DirectlyUsedService 通过 new 引用了类型，并直接调用了 Describe，因此类型和方法都保留。

通过 typeof 引用的 TypeofOnlyService 作为类型得以保留，但没有任何地方调用的 Describe 以成员为单位被删除了。

而完全没有静态引用的 ReflectionOnlyService 则被整个删除。

另外，本实验中类型名和方法名的字符串全部通过 static readonly 字段读取，这是有意为之。

UnityLinker 能理解部分典型的反射用法，我们已确认，像 Type.GetType("ReflectionOnlyService") 这样直接传入常量字符串时，即使在 High 下类型也会被保留。

此外，对 typeof 获得的 Type 直接调用 GetMethod 时，即使方法名不是常量，该类型的 public 方法也会被整体保留。

但另一方面，通过字段传入的字符串 UnityLinker 就不会追踪了。

类型名和成员名来自配置文件、序列化数据、服务器响应等构建时无法确定的途径，这种实际情况正是通过字段读取来再现的。

如果把字段改为 const，字符串会作为常量内联到使用处，因此与直接传入常量的情况相同。

### DI 容器中的类型解析失效

至此的示例为便于说明做了简化，但相同结构的问题在实际的库中也会发生。典型的就是 DI 容器。

许多 DI 容器会先注册接口与实现类型的对应关系，在解析时通过反射调用构造函数来创建实例。

我们用一个模拟这一行为的最小容器来确认。与实际的 DI 容器一样，接口设计为用 Register 注册、用 Resolve 解析（{{list:mini_container}}）。

CODE START csharp | 最小的 DI 容器 {{list:mini_container}}  
public sealed class MiniContainer  
{  
    private readonly Dictionary\<Type, Type\> \_registrations \= new Dictionary\<Type, Type\>();

    // 通过类型参数注册实现类型。TImplementation 成为相当于 typeof 的静态引用  
    public void Register\<TInterface, TImplementation\>() where TImplementation : TInterface  
    {  
        \_registrations\[typeof(TInterface)\] \= typeof(TImplementation);  
    }

    // 通过反射实例化已注册的实现类型并返回  
    public TInterface Resolve\<TInterface\>()  
    {  
        var implementationType \= \_registrations\[typeof(TInterface)\];  
        return (TInterface)Activator.CreateInstance(implementationType);  
    }  
}  
CODE END

要注册的实现类型假定只通过容器使用，没有任何地方静态引用它（{{list:notification_service}}）。

CODE START csharp | 要注册的接口与实现 {{list:notification_service}}  
public interface INotificationService  
{  
    string Notify();  
}

// 只通过容器使用、没有任何地方静态引用的实现  
public sealed class NotificationService : INotificationService  
{  
    public string Notify() \=\> "NotificationService.Notify succeeded";  
}  
CODE END

从放置在场景中的 MonoBehaviour 进行注册和解析（{{list:di_probe}}）。

CODE START csharp | 进行注册和解析的 MonoBehaviour {{list:di_probe}}  
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

在与之前相同的条件下构建，结果如下。

CODE START text  
\# Minimal  
\[DiContainerProbe\] explicit: NotificationService.Notify succeeded

\# High  
\[DiContainerProbe\] explicit failed: MissingMethodException: Default constructor not found for type NotificationService  
CODE END

在 High 下，Register\<INotificationService, NotificationService\>() 的类型参数构成静态引用，因此 NotificationService 的类型本身得以保留。

此外，通过接口静态调用的 Notify 也得以保留。

然而，只通过反射调用的默认构造函数没有静态引用，因此以成员为单位被删除了。

结果，Activator.CreateInstance 找不到构造函数，解析因 MissingMethodException 而失败。

这就是“类型明明已经注册，却只在构建后的应用中解析失败”这一问题的原因。

实际的 DI 容器在结构上也是一样的。

只要构造函数的解析依赖反射，即使是显式注册的类型，其隐式默认构造函数也会成为删除候选。

如果采用从程序集扫描实现类型并自动注册的方式，没有静态引用的实现类型会被整个删除，注册本身就会失败。

## **未被 Unity 入口点引用的代码消失**

另一种模式是，既未被场景引用也未被脚本引用，仅通过 AssetBundle 等外部内容才需要的代码。

如前一节所述，root 的来源是构建中包含的场景和资源。

作为 AssetBundle 单独构建的内容不包含在应用本身的构建中，因此该内容引用的类型不会被视为 root。

因此，仅内容一侧需要的类型，只要应用代码中没有静态引用，无论运行时是否会被加载，都会被判定为未使用。

我们用 Timeline 资源再现这一模式。

Timeline 资源只包含在 AssetBundle 中，应用一侧不静态引用 Unity.Timeline，而是按资源名加载后通过 PlayableDirector 播放（{{list:timeline_probe}}）。

CODE START csharp | 从 AssetBundle 播放 Timeline 资源的 MonoBehaviour {{list:timeline_probe}}  
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

创建 AssetBundle 后，先从项目中删除原始的 Timeline 资源再构建应用，以此模拟与应用分开更新内容的方式。

将 Managed Stripping Level 分别设为 Minimal 和 High 进行构建。两个级别的结果相同。

CODE START text  
\# Minimal / High 均为  
The referenced script (UnityEngine.Timeline.TimelineAsset) on this Behaviour is missing\!  
timeline bundle: asset TimelineOnly failed to load  
CODE END

此外，查看构建后应用的 Managed 文件夹，两个构建中都不存在 Unity.Timeline.dll 本身。

需要注意的是，Minimal 和 High 的结果没有差别。

对这一模式的应对不是调整 Stripping Level，而是显式指定保留所需的类型或程序集。

具体手段在下一节讨论，使用 Addressables 时的机制在进阶篇讨论。

参考：\[How code stripping affects content\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-content.html) / \[Timeline 资源消失的案例（Qiita, 2021）\](https://qiita.com/KyoheiOkawa/items/0cf9cc09679eababec75)

# **代码过度削减问题的应对方法**

## **基本应对方法**

防止所需代码被删除的最基本手段，是向 UnityLinker 显式声明“这段代码会被使用”。

Unity 为此提供了 link.xml 文件和 \[Preserve\] 特性两种手段。

通过任一方式指定的代码都会被视为分析的 root，即使没有静态引用也不会被删除。

### **link.xml**

link.xml 是列举要保留的程序集、类型和成员的 XML 文件。

放在项目的 Assets 目录下，UnityLinker 会在构建时读取它。

前一节示例中被整个删除的 ReflectionOnlyService，可以用下面的 link.xml 保留（{{list:linkxml_reflection}}）。

CODE START xml | 保留 ReflectionOnlyService 的 link.xml {{list:linkxml_reflection}}  
\<linker\>  
  \<assembly fullname="Assembly-CSharp"\>  
    \<type fullname="ReflectionOnlyService" preserve="all" /\>  
  \</assembly\>  
\</linker\>  
CODE END

将这个 link.xml 放在 Assets 根目录下，用 High 重新构建后，运行结果变为如下。

CODE START text  
\# High（无 link.xml）  
reflection: Type.GetType("ReflectionOnlyService") returned null

\# High（有 link.xml）  
reflection: ReflectionOnlyService is alive  
CODE END

查看构建中包含的程序集，也可以确认 ReflectionOnlyService 连同 Describe 一起被保留了。

link.xml 可以在代码之外指定保留目标，因此也适用于自己无法编辑的 DLL 和包中的类型。

保留范围可以按程序集、按类型，或按方法、字段等成员为单位指定。

另外，用 preserve="all" 保留整个程序集的写法虽然简便，但会抵消对该程序集的 stripping 效果。

它可用于问题排查，但作为永久性措施，最好能缩小到所需的类型和成员。

### **\[Preserve\] 特性**

如果是自己可以编辑的代码，可以直接给目标加上 UnityEngine.Scripting 命名空间的 \[Preserve\] 特性。

需要注意的是，\[Preserve\] 的保留范围会随标注位置而变化。

我们用两个类来确认这一差异：一个在类型上标注，另一个只在方法上标注（{{list:type_preserved}}、{{list:method_preserved}}）。

CODE START csharp | 在类型上标注 Preserve 的情况 {{list:type_preserved}}  
using UnityEngine.Scripting;

// 在类型上加 \[Preserve\]，方法上不加  
\[Preserve\]  
public sealed class TypePreservedService  
{  
    public string Describe() \=\> "TypePreservedService.Describe is alive";  
}  
CODE END

CODE START csharp | 只在方法上标注 Preserve 的情况 {{list:method_preserved}}  
using UnityEngine.Scripting;

// 类型上不加，只在方法上加 \[Preserve\]  
public sealed class MethodPreservedService  
{  
    \[Preserve\]  
    public string Describe() \=\> "MethodPreservedService.Describe is alive";  
}  
CODE END

以不含静态引用的方式通过反射创建并调用这两个类，在 High 下的结果如下（{{table:preserve_results}}）。

表: Preserve 的标注位置与 High 下的保留范围 {{table:preserve_results}}

| 类 | 类型 | 默认构造函数 | Describe |
| :---- | :---- | :---- | :---- |
| TypePreservedService | 保留 | 保留 | 被删除 |
| MethodPreservedService | 保留 | 被删除 | 保留 |

标注在类型上的 \[Preserve\] 会保留类型和默认构造函数，但不会保留未标注特性的 Describe。

标注在方法上的 \[Preserve\] 会保留该类型和方法，但这次默认构造函数不被保留，通过 Activator.CreateInstance 的创建因 MissingMethodException 而失败。

要让通过反射创建并调用的流程完整运行，需要在类型和方法上都加上 \[Preserve\]。

遇到“加了 \[Preserve\] 却不起作用”的情况时，首先应怀疑特性的标注位置与保留范围的对应关系。

参考：\[Preserving code using annotations\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-preserving.html)

## **进阶篇**

###  **活用 Preserve 以外的特性**

相对于 \[Preserve\] 是“保留这个元素”的逐点指定，UnityEngine.Scripting 命名空间中还有按模式声明保留的特性，例如“保留这个接口的全部实现”。

在“通过反射查找并调用用户实现的类型”这类框架中常见的结构里，无需逐个用 \[Preserve\] 指定实现类型，只要在接口或基类上标注特性即可应对。

* \[RequireImplementorsAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireImplementorsAttribute.html)：标注在接口上时，保留实现该接口的类型](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireImplementorsAttribute.html\)：标注在接口上时，保留实现该接口的类型)  
* \[RequireDerivedAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireDerivedAttribute.html)：标注在类上时，保留继承该类型的类型](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireDerivedAttribute.html\)：标注在类上时，保留继承该类型的类型)  
* \[RequiredMemberAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredMemberAttribute.html)：标注在成员上时，在该类型被保留的情况下也保留该成员](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredMemberAttribute.html\)：标注在成员上时，在该类型被保留的情况下也保留该成员)  
* \[RequiredInterfaceAttribute\](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredInterfaceAttribute.html)：标注在类型上时，保留指定接口的实现信息

以 \[RequireImplementors\] 为代表确认其行为。

在接口上标注特性，实现则没有任何地方静态引用（{{list:require_implementors}}）。

CODE START csharp | 标注了 RequireImplementors 的接口与实现 {{list:require_implementors}}  
// 标注在接口上后，实现类型成为保留目标  
\[RequireImplementors\]  
public interface IRequiredContract  
{  
}

// 没有任何地方静态引用的实现  
public sealed class RequiredContractImpl : IRequiredContract  
{  
}  
CODE END

用 High 构建后，RequiredContractImpl 连同默认构造函数一起被保留了。

没有静态引用的实现类型，仅凭接口一侧的声明就得以保留。

对于标注了 \[RequireDerived\] 的基类的派生类型，也同样确认了保留。

### **用 IUnityLinkerProcessor 在构建时生成 link.xml**

如果要保留的代码已经确定，只需在 link.xml 中指定目标类和成员即可。

但另一方面，在不同构建目标需要保留的类型不同、所需类型随构建中包含的内容而变化等情况下，持续手动更新 link.xml 并不现实。

使用 \*\*IUnityLinkerProcessor\*\*，可以在构建时动态生成 link.xml 并传给 UnityLinker。

IUnityLinkerProcessor 是编辑器用的接口，将实现类放在编辑器程序集中，构建时就会被自动调用。

GenerateAdditionalLinkXmlFile 返回的路径所指向的 link.xml 会被添加到 UnityLinker 的输入中。

最小实现如下（{{list:linker_processor}}）。

CODE START csharp | 生成 link.xml 的最小 IUnityLinkerProcessor 实现 {{list:linker_processor}}  
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

    // 返回路径所指向的 link.xml 会被添加到 UnityLinker 的输入中  
    public string GenerateAdditionalLinkXmlFile(BuildReport report, UnityLinkerBuildPipelineData data)  
    {  
        var path \= Path.Combine(Path.GetTempPath(), "managed-stripping-sample-generated-link.xml");  
        File.WriteAllText(path, LinkXmlContent);  
        Debug.Log("\[PreserveListLinkerProcessor\] generated 1 assembly / 1 type.");  
        return path;  
    }  
}  
CODE END

此例为便于说明固定了生成内容，实际使用时应将这部分替换为根据构建时的输入进行组装的处理，例如读取配置资源或分析内容的依赖。

准备一个没有静态引用的 GeneratedPreserveService 并用 High 构建，可以确认即使 Assets 下没有放置 link.xml，类型、构造函数和方法也都被保留了。

CODE START text  
\[GeneratedLinkXmlProbe\] type=present  
\[GeneratedLinkXmlProbe\] ctor=present, method=GeneratedPreserveService.Describe is alive  
CODE END

参考：\[IUnityLinkerProcessor\](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Build.IUnityLinkerProcessor.html)

###  **Addressables 生成的 link.xml**

Addressables 针对前一节所述“仅内容一侧需要的类型消失”的问题，通过将 link.xml 作为内容构建的产物自动生成来应对。

我们来确认实际会生成什么。内容采用最小配置，只创建一个显示 Cube 网格的 prefab（在 GameObject 上添加 MeshFilter 和 MeshRenderer），并注册到 Addressables 的分组中。

执行内容构建后，会生成 link.xml。

在 Addressables 4.0.1 中，生成位置为 Library/com.unity.addressables/aa/\<platform\>/AddressablesLink/link.xml。

本次生成的内容如下（{{list:addressables_linkxml}}）。

CODE START xml | Addressables 生成的 link.xml {{list:addressables_linkxml}}  
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

内容分为两类。

一类是 Addressables 运行时（Unity.Addressables 和 Unity.ResourceManager）的 provider 类型，根据分组配置而需要。

另一类是已注册内容所引用的类型。本次构成 prefab 的 GameObject、Transform、MeshFilter、MeshRenderer 都直接列为了保留目标。

由此可见，Addressables 会分析内容一侧的依赖，即使是应用代码未引用的类型也会纳入保留范围。

参考：\[Addressables Build Artifacts\](https://docs.unity3d.com/ja/Packages/com.unity.addressables@1.20/manual/BuildArtifacts.html)（关于 link.xml 的说明见 1.20 版手册）

### **将反射调用的代码替换为 Source Generator 生成的静态调用**

到目前为止的应对方法，都是通过指定保留来防止所需代码被删除。

与之相对，还有一种方法是用 \*\*Source Generator\*\* 的代码生成来替换反射本身，因为正是反射产生了静态分析看不到的依赖。

Source Generator 是 C\# 编译器在编译时分析源代码并生成额外源代码的机制。

只要把类型的查找和创建在编译时完成，依赖从一开始就以静态引用的形式表达，保留指定本身就不再需要。

作为示例，考虑一个接收命令名字符串并执行对应处理器的分发处理。

处理器通过特性声明命令名。如果在运行时用反射实现，就是从程序集中查找带该特性的类型并创建的结构（{{list:command_handlers}}）。

CODE START csharp | 通过特性声明命令名的处理器 {{list:command_handlers}}  
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

// 处理器没有任何地方静态引用  
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

这两个处理器没有任何地方静态引用，因此如果保持运行时扫描的结构用 High 构建，就会如前一节所述被整个删除。

我们用 Source Generator 来实现从命令名到处理器的解析。

准备一个生成器，它在编译时收集标注了 \[CommandHandler\] 的类，并生成根据命令字符串查找具体类型的分发器。

生成器的工作分两个阶段：前半部分（ForAttributeWithMetadataName）从带特性的类中收集命令名和类型名，后半部分（GenerateDispatcher）把收集到的配对填入模板。

生成器的完整代码如下（{{list:dispatcher_generator}}）。

CODE START csharp | 生成分发器的 Source Generator {{list:dispatcher_generator}}  
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
            // 从标注了 \[CommandHandler\] 的类中收集命令名和类型名的配对  
            var handlers \= context.SyntaxProvider  
                .ForAttributeWithMetadataName(  
                    "CommandHandlerAttribute",  
                    static (\_, \_) \=\> true,  
                    static (syntaxContext, \_) \=\> (  
                        Command: (string)syntaxContext.Attributes\[0\].ConstructorArguments\[0\].Value\!,  
                        TypeName: ((INamedTypeSymbol)syntaxContext.TargetSymbol)  
                            .ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat)))  
                .Collect();  
            // 根据收集到的配对生成分发器  
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
            // 为使输出具有确定性，先按命令名排序再填入  
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

按命令名排序是为了让相同输入生成相同代码，避免每次构建输出发生变动。

通过这个生成器，编译时会生成如下代码（{{list:generated_dispatcher}}）。

CODE START csharp | 生成的 CommandDispatcher {{list:generated_dispatcher}}  
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

原本在运行时根据字符串查找类型的处理，变成了字符串与具体类型的对应表。

构造（new）和 Execute 的调用都出现在生成代码中，因此对类型、构造函数、方法的静态引用在生成代码内部就已完整。

使用方只需传入命令字符串，完全不引用处理器的具体类型。

与反射实验相同，命令字符串通过 static readonly 字段传入，保持为 UnityLinker 无法追踪的输入（{{list:dispatch_probe}}）。

CODE START csharp | 生成的分发器的使用方 {{list:dispatch_probe}}  
// 命令字符串通过字段传入，使其成为 UnityLinker 无法追踪的输入  
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

用 High 构建并运行后，确认两个命令都执行成功。

CODE START text  
\[SourceGeneratorProbe\] dispatch command=farewell, result=FarewellCommandHandler.Execute succeeded  
\[SourceGeneratorProbe\] dispatch command=greet, result=GreetCommandHandler.Execute succeeded  
CODE END

字符串本身仍然是构建时无法追踪的输入，但由于字符串到类型的对应不再是运行时查找，而是生成代码中的 switch，在 UnityLinker 看来它与手写代码无异，是普通的静态引用。

用 Mono.Cecil 确认，两个处理器的类型、构造函数、Execute 以及 CommandDispatcher.Execute 也都被保留了。

这种替换的好处不只是对 stripping 的耐受性。

\- 依赖对 UnityLinker 可见，因此无需再维护 link.xml 或特性的保留指定  
\- 不再有运行时的反射查找，启动开销和运行时失败的可能性都减少  
\- 目标的收集在编译时进行，因此漏注册和重复可以在编译时诊断

实际的库中也有采用这类实现的案例。

序列化器方面，MessagePack for C\# 长期为 IL2CPP 环境提供代码生成，目前已迁移到基于 Source Generator 的生成。

DI 容器方面，VContainer 提供了用 SourceGenerator 生成注入代码以减少运行时反射的选项，通过 Source Generator 解决了前一节所述“构造函数消失导致解析失败”的症状。

在 Unity 项目中引入 Source Generator 的步骤（将 netstandard2.0 的 DLL 带 RoslynAnalyzer 标签放置的配置），请参阅 Unity 手册。

参考：\[Create and use a source generator\](https://docs.unity3d.com/6000.3/Documentation/Manual/create-source-generator.html) / \[MessagePack for C\#\](https://github.com/MessagePack-CSharp/MessagePack-CSharp) / \[MemoryPack\](https://github.com/Cysharp/MemoryPack) / \[VContainer\](https://github.com/hadashiA/VContainer)

# **总结**

Managed Code Stripping 并不是提高 Stripping Level 就万事大吉的优化。

如本章所见，通过反射或外部内容产生的依赖对静态分析不可见，即使是运行时需要的代码也会从构建中消失。

而且症状只会在构建后的应用中出现，因此必须把显式声明不可见依赖与持续在构建后的应用中验证作为一套流程来执行。

显式指定保留的手段应根据目标和情况区分使用（{{table:approach_selection}}）。

表: 应对方法的选用 {{table:approach_selection}}

| 方法 | 适用情况 |
| :---- | :---- |
| \[Preserve\] | 保留自己可编辑代码中的少数元素 |
| link.xml | 保留包括不可编辑 DLL 在内的固定目标 |
| Require 系列特性 | 在框架一侧声明“保留实现或派生”的契约 |
| IUnityLinkerProcessor | 保留目标随构建目标或内容而变化 |
| Source Generator | 将反射查找本身替换为静态引用 |

此外，列出几条让 stripping 更容易发挥效果的方针。

* 除会出问题的情况外，不要过度使用 \[Preserve\] 和 link.xml  
* 减少通过反射查找和创建类型，考虑改为显式注册或借助 Source Generator 的静态引用  
* 代码保留的指定尽量缩小到类型、成员级别。整个程序集的 preserve="all" 仅限于问题排查  
* 对于只有与应用分开构建的内容才使用的类型，其保留不要靠手工，而要利用 Addressables 的 link.xml 或 IUnityLinkerProcessor 生成等机制  
* 从开发阶段就运行并验证提高了 Stripping Level 的构建
