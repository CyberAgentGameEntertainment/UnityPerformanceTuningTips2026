# **Lifetime Management of AssetBundles**

Name：Inada Taketo  
Affiliation：CyberAgent, Inc. SGE Core Technology Division  
X account / Website / GitHub account (optional):

Games load many assets at runtime, such as textures, meshes, and audio. If each implementer manages them individually, missed releases and unnecessary assets staying resident are likely to occur.

Therefore, as a rule, consolidate the entry points for loading and releasing assets that are loaded dynamically from AssetBundles, Addressables, and similar sources into the system foundation. Determine the asset's lifetime at load time, and build a mechanism in which the foundation releases the assets together when that lifetime ends.

In this section, "release" means releasing the references and release handles managed by the loading foundation. Whether an asset is actually unloaded from memory depends on the implementation of the foundation, the reference counts, and the state of dependencies.

Note that the classifications and implementation methods introduced below are examples. Choose the approach that fits the game structure of each project.

### **Managing Asset Lifetimes Separately**

First, organize the periods during which assets are needed in the game. For example, they can be classified as follows.

表: Example classification of asset lifetimes {{table:asset_lifetime_types}}

Lifetime	Example retention period  
Common	Until common data is reset, such as at logout, account switching, or return to the title screen  
Scene	Until the target scene ends  
View	Until the target screen or dialog is closed  
Custom	Until a period defined independently, such as a battle or an event, ends

What matters is not names such as Common, Scene, and View, but making clear when each lifetime starts and when it ends.

If the classification is too coarse, unnecessary assets remain for a long time; if it is too fine, the number of loads and releases and the management cost increase. Also, when assets with different lifetimes are included in the same AssetBundle, unloading per AssetBundle cannot select and unload only some of the assets. Design the lifetime classification together with the AssetBundle granularity (Chapter 5, "Tuning Practice - AssetBundle", of the previous book, "Unity Performance Tuning Bible").

### **Determining the Lifetime at Load Time**

Once the lifetimes are classified, decide the registration destination when loading an asset.

One implementation example is to specify the lifetime as an argument of the load function.

CODE START csharp | Example of loading with a specified lifetime {{list:asset_load_with_lifetime}}  
var background \=  
    await AssetLoader.LoadAsync\<GameObject\>(  
        address,  
        AssetLifetime.Scene);  
CODE END

In {{list:asset_load_with_lifetime}}, the loaded asset is registered to the Scene lifetime. When the scene ends, the foundation releases the registered assets together.

As another implementation example, there is also a method of separating the load functions by lifetime.

CODE START csharp | Example of separating load functions by lifetime {{list:asset_load_by_lifetime}}  
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

Separating the functions, as in LoadCommonAsync, LoadSceneAsync, and LoadViewAsync, lets you tell the lifetime from the call site and makes it easier to prevent mistakes in specifying the registration destination. On the other hand, specifying it by argument makes it easier to add custom lifetimes.

With either method, what matters is that the lifetime is always determined at load time and that the foundation can release the asset when that lifetime ends.

To handle the case in which the lifetime ends before an asynchronous load completes, track the operation from the start of the load, and either cancel it if the lifetime ends midway or make it possible to release the result after completion. Also, when the same asset is used from multiple lifetimes, manage a reference count and release the asset when the last use ends.

### **Tying Lifetimes to a GameObject**

As another management method, a GameObject can be made the owner of a lifetime.

Add a dedicated Component to the root GameObject of a screen or dialog and register the release handles of the loaded assets. When the GameObject is destroyed, release the registered handles from OnDestroy.

CODE START csharp | Example of tying a lifetime to a GameObject {{list:asset_scope_component}}  
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

In {{list:asset_scope_component}}, even if an asynchronous load completes and Add is called after the GameObject has been destroyed, the added handle is released immediately.

However, merely destroying the GameObject does not automatically release dynamically loaded assets. The dedicated Component must call the release process corresponding to the loading foundation.

Do not create a management GameObject for each asset; consolidate them into one per lifetime unit, such as one screen or one dialog. This method is effective when the lifetimes of the GameObject and the assets coincide, but it may not suit assets used across multiple scenes. Use it together with per-lifetime management tables as needed.

### **Checking for Missed Releases**

Take Memory Profiler snapshots before and after a screen or scene transition and compare them to see whether assets of the ended lifetime remain. It is also effective to repeat the same transition several times and check whether assets or load handles keep increasing.

For measurement, compare states against the same baseline after the necessary release processing has completed, and ultimately verify in a player build on a physical device. However, some assets are referenced by other features or intentionally held as a cache, so do not judge a missed release merely because an asset remains; check the actual referrers.

Changing asset management late in development affects many load processes. Organize the necessary lifetimes early in development, and design a mechanism in which a lifetime suited to the project is applied without implementers having to think about individual release timing.
