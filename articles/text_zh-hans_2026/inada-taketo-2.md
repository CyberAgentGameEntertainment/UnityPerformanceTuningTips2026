# **AssetBundle 的生命周期管理**

姓名 / 罗马字：Inada Taketo  
所属：株式会社 CyberAgent SGE 核心技术本部  
X 账号 / 网站 / GitHub 账号（可选）：

游戏在运行时会加载纹理、网格、音频等大量资源。如果由各个实现者分别管理这些资源，就容易出现释放遗漏或不需要的资源常驻内存的问题。

因此，对于从 AssetBundle 或 Addressables 等动态加载的资源，原则上应将加载和释放的入口集中到系统基础层。在加载时确定资源的生命周期，并建立在生命周期结束时由基础层统一释放的机制。

本节所说的“释放”，是指释放由加载基础层管理的引用或释放用句柄。资源是否真正从内存中卸载，取决于基础层的实现、引用计数以及依赖关系的状态。

另外，下文介绍的分类和实现方法仅为参考示例。请根据各项目的游戏结构选择合适的方法。

### **按生命周期分开管理资源**

首先，整理游戏中各资源需要存在的期间。例如，可以进行如下分类。

表: 资源生命周期的分类示例 {{table:asset_lifetime_types}}

生命周期	保持期间的示例  
Common	直到登出、切换账号、返回标题界面等重置共用数据为止  
Scene	直到结束目标场景为止  
View	直到关闭目标界面或对话框为止  
Custom	直到战斗、活动等自行定义的期间结束为止

重要的不是 Common、Scene、View 这些名称，而是明确每个生命周期何时开始、何时结束。

分类过粗，不需要的资源会长期残留；分类过细，加载和释放的次数以及管理成本会增加。此外，如果生命周期不同的资源包含在同一个 AssetBundle 中，以 AssetBundle 为单位卸载时无法只选择其中一部分资源卸载。生命周期的分类应与 AssetBundle 的粒度（前作《Unity 性能调优宝典》第 5 章“Tuning Practice \- AssetBundle”）一并设计。

### **在加载时确定生命周期**

完成生命周期的分类后，在加载资源时确定其注册目标。

其中一种实现方式是通过加载函数的参数指定生命周期。

CODE START csharp | 指定生命周期进行加载的示例 {{list:asset_load_with_lifetime}}  
var background \=  
    await AssetLoader.LoadAsync\<GameObject\>(  
        address,  
        AssetLifetime.Scene);  
CODE END

在 {{list:asset_load_with_lifetime}} 中，将加载的资源注册到了 Scene 生命周期。场景结束时，基础层会统一释放已注册的资源。

另一种实现方式是按生命周期拆分加载函数。

CODE START csharp | 按生命周期拆分加载函数的示例 {{list:asset_load_by_lifetime}}  
var commonIcon \=  
    await AssetLoader.LoadCommonAsync\<Sprite\>(  
        commonAddress);

var stagePrefab \=  
    await AssetLoader.LoadSceneAsync\<GameObject\>(  
        stageAddress);

var dialogPrefab \=  
    await AssetLoader.LoadViewAsync\<GameObject\>(  
        dialogAddress,  
        viewId);  
CODE END

像 LoadCommonAsync、LoadSceneAsync、LoadViewAsync 这样拆分函数后，可以从调用处判断生命周期，也更容易防止注册目标指定错误。另一方面，通过参数指定的方式则具有易于添加自定义生命周期的特点。

无论采用哪种方式，重要的是在加载时必定确定生命周期，并且在其结束时能够由基础层释放。

为应对异步加载完成前生命周期就已结束的情况，应从开始加载时起跟踪处理，在中途结束时取消加载，或能够释放完成后的结果。此外，当同一资源被多个生命周期使用时，应管理引用计数，在最后一次使用结束时释放。

### **将生命周期绑定到 GameObject**

另一种管理方式是让 GameObject 成为生命周期的所有者。

在界面或对话框的根 GameObject 上添加专用的 Component，并注册已加载资源的释放用句柄。当 GameObject 被销毁时，在 OnDestroy 中释放已注册的句柄。

CODE START csharp | 将生命周期绑定到 GameObject 的示例 {{list:asset_scope_component}}  
public sealed class AssetLifetimeScope : MonoBehaviour  
{  
    private readonly List\<IDisposable\> \_handles \=  
        new List\<IDisposable\>();

    private bool \_released;

    public void Add(IDisposable handle)  
    {  
        if (\_released)  
        {  
            handle.Dispose();  
            return;  
        }

        \_handles.Add(handle);  
    }

    private void OnDestroy()  
    {  
        \_released \= true;

        for (var i \= \_handles.Count \- 1; i \>= 0; i--)  
        {  
            \_handles\[i\].Dispose();  
        }

        \_handles.Clear();  
    }  
}  
CODE END

在 {{list:asset_scope_component}} 中，即使 GameObject 销毁后异步加载才完成并调用了 Add，也会立即释放所添加的句柄。

不过，仅销毁 GameObject 并不会自动释放动态加载的资源。需要从专用的 Component 调用与加载基础层对应的释放处理。

管理用的 GameObject 不要按资源逐个创建，而是按一个界面或对话框等一个生命周期单位合并为一个。这种方式在 GameObject 与资源的寿命一致时有效，但对于跨多个场景使用的资源等情况可能并不适用。请根据需要与按生命周期划分的管理表配合使用。

### **检查释放遗漏**

使用 Memory Profiler 获取界面或场景跳转前后的快照，比较已结束生命周期的资源是否仍然残留。多次重复同一跳转，确认资源或加载句柄是否持续增加，也是有效的方法。

测量时，应在必要的释放处理完成后，在相同的基准状态之间进行比较，并最终在真机的 Player 构建上确认。不过，也存在被其他功能引用的资源，以及作为缓存有意保留的资源，因此不要仅凭资源残留就判断为释放遗漏，而要确认到实际的引用方。

资源管理若在开发后期变更，会影响大量加载处理。应在开发早期整理所需的生命周期，并设计出即使实现者不关注各自的释放时机，也能应用适合项目的生命周期的机制。
