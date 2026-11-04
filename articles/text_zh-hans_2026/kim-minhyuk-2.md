# **iOS 的内存上限与虚拟地址空间**

姓名 / 罗马字：Minhyuk Kim  
所属：株式会社 CyberAgent SGE 核心技术本部

游戏可使用的设备内存自然存在上限。在场景切换过程中，如果新旧场景的资源同时驻留在内存中，或临时的渲染缓冲区增多，这一上限就会成为问题。

除了降低游戏内的内存峰值之外，内存上限本身也有调整的余地。Xcode 的“Increased Memory Limit”是在受支持的设备上提高该上限的设置。另一项设置“Extended Virtual Addressing”则是扩大进程可使用的虚拟地址空间。两者名称相似，但能够应对的领域并不相同。

本章介绍这两项设置的区分使用方法、在真机上的效果，以及在 App Store 上的运营经验。文中的数值来自笔者的真机测量，并非 Apple 保证的各设备上限。

# **内存上限与虚拟地址空间是不同的限制**

当应用发生 OOM（Out of Memory）时，至少需要将以下两点分开考虑。

表: iOS 应用中需要区分的两种内存限制 {{table:memory_constraints}}

| 限制 | 主要查看的值 | 典型表现 |
| :---- | :---- | :---- |
| 物理内存侧 | `phys_footprint` | 因内存使用过高被系统终止（jetsam） |
| 虚拟地址空间侧 | VM Region Summary、Virtual Size | 内存分配失败，随后发生崩溃 |

物理内存涉及实际需要保留在 RAM 中的页面。在 iOS 上，应用可使用的物理内存量并不等于设备搭载的 RAM 本身，还会受到当时的系统状态以及应用所适用的上限影响。

Apple 的 `os_proc_available_memory()` 返回当前应用在达到内存上限之前还可以分配的量。按照 Apple 的说明，该返回值对应于“当前内存上限减去应用的 memory footprint”，不一定等于设备的空闲物理内存。此外，上限在应用的生命周期中也可能变化，因此不应缓存该值，而应将其作为参考信息处理 {{fn:os_proc_available_memory}}。

虚拟地址空间是用于管理内存和文件映射到哪些地址的范围。例如，即使对大文件执行 `mmap`，其全部页面也不一定会立即读入 RAM，但放置该范围所需的虚拟地址仍然是必需的。反过来，也会出现物理内存的 footprint 仍然较低、`os_proc_available_memory()` 也仍有余量，却无法获取新的地址范围，导致 `malloc` 等失败的情况。

因此，为了区分 {{table:memory_constraints}} 中的两种限制，需要确认是留下了 jetsam report，还是在普通的崩溃报告中以分配失败为起点。
即使 `os_proc_available_memory()` 返回较大的值，也不代表虚拟地址空间有同等的余量。

# **两个 Entitlement 的作用**

## **Increased Memory Limit**

Increased Memory Limit 的键是 `com.apple.developer.kernel.increased-memory-limit`。Apple 将其说明为一种 Entitlement，用于告知系统：在受支持的设备上超过默认的应用内存上限，可以改善应用的核心功能 {{fn:increased_memory_limit}}。在 Xcode 的 Capability 列表中，也会如 {{fig:xcode_increased_memory_limit}} 所示显示带有“在受支持的设备上”这一条件的说明。

![][image1]{scale=0.8}
図: Xcode 的 Capability 列表中显示的 Increased Memory Limit {{fig:xcode_increased_memory_limit}}

也就是说，并非只要指定了就会在所有设备上增加固定的量。Apple 的文档中也写明，只有部分设备能够使用提高后的上限，并要求应用在无法使用额外内存的情况下也能正常运行。

### **Increased Memory Limit 的真机测量**

表: 启用与未启用 Increased Memory Limit 时 OOM 发生时的内存测量结果（iPhone） {{table:increased_memory_observations}}

| 搭载 RAM | 未启用 | 启用 |
| :---- | ----: | ----: |
| 3GB | 约 1,850MB | 约 1,850MB |
| 4GB | 约 2,050MB | 约 2,290MB |
| 6GB | 约 3GB | 约 4GB |

在 4GB 和 6GB 的 iPhone 上内存分别有所增加，而在 3GB 的 iPhone 上，启用与未启用没有差别。
此外，iPad 在 OOM 发生时的内存量本来就与 iPhone 不同，因此启用后的结果很可能也不一样。
如果目标用户的设备占比中 3GB 的 iPhone 较多，就不应指望仅靠添加 Increased Memory Limit 便能解决问题。

## **Extended Virtual Addressing**

Extended Virtual Addressing 的键是 `com.apple.developer.kernel.extended-virtual-addressing`。Apple 将其说明为表示可访问更大地址空间的 Boolean 值，并以游戏作为示例 {{fn:extended_virtual_addressing}}。

![][image2]{scale=0.8}
図: Xcode 的 Capability 列表中显示的 Extended Virtual Addressing {{fig:xcode_extended_virtual_addressing}}

该设置不会增加 RAM。对于接近物理内存上限而被 jetsam 的应用，仅添加 Extended Virtual Addressing 并不能保证可以继续使用同样多的 RAM。能够期待效果的情况是：由于大型内存映射、碎片化的地址区域、大型固定预留等原因，在用尽物理内存之前虚拟地址空间已先行吃紧。

启用本身几乎没有成本。页表只会为实际映射的范围创建，因此扩大上限并不会使其增加 {{fn:xnu_pmap}}。需要考虑的是，是否存在能产生效果的设备。添加 Increased Memory Limit 时地址空间也会随之扩大，因此在某些设备上，即使两者都添加，`max_address` 也不会变化。

另一方面，在像 {{table:increased_memory_observations}} 中的 3GB 设备那样 Increased Memory Limit 不起作用的设备上，Extended Virtual Addressing 是扩大地址空间的唯一手段。Increased Memory Limit 在设备不支持时，即使签名正确也不会生效，而 Extended Virtual Addressing 没有这一条件。

### **虚拟地址空间的上限值**

Extended Virtual Addressing 扩大的是与 footprint 上限不同的地址空间上限。该上限由 `pmap_max_64bit_offset()` 根据搭载内存返回的 {{table:va_ceilings}} 中的某一个值决定 {{fn:xnu_pmap}}。

表: arm64 用户进程的地址空间上限 {{table:va_ceilings}}

| 条件 | 常量 | 上限 |
| :---- | :---- | ----: |
| `max_mem` > 3GB | `ARM64_MAX_OFFSET_DEVICE_LARGE` | 17.375GB |
| `max_mem` 大于 1GB 且不超过 3GB | `ARM64_MAX_OFFSET_DEVICE_SMALL` | 13.375GB |
| `max_mem` 1GB 以下 | `ARM64_MIN_MAX_ADDRESS` | 12.5GB |
| 通过包括 entitlements 在内的任一手段扩展 | `MACH_VM_JUMBO_ADDRESS` | 63GB |

`max_mem` 是内核识别到的物理内存，小于产品规格上标注的 RAM 容量。
这些都是内核的编译期常量，因此可能随 OS 版本而变化。

## **在 App Store 上的上架先例**

采用这些 Entitlement 时，令人担心的一点是会不会影响 App Store 的审核。在笔者了解的多款运营中的游戏里，Increased Memory Limit 和 Extended Virtual Addressing 都有在启用设置的状态下通过 App Store 审核并上线发行的先例。

Apple 自身也在面向游戏开发者的“Bring your high-end game to iPhone 15 Pro”{{fn:apple_game_memory}} 中建议启用 Increased Memory Limit，以增加可用内存。这两项都是可以在产品中考虑采用的公开功能。虽然这并不保证单个应用整体的审核结果，但没有必要因为“这是涉及内存上限的设置，审核时可能被要求说明或被拒绝”这样的担忧而将其排除在候选之外。

# **确认 App ID 与开发者账号**

## **无法使用通配符 Profile**

这两个 Entitlement 无法在通配符形式的 App ID 上启用。
如果采用如 `com.example.*` 那样对多个应用统一签名的配置，
则需要切换为与游戏的 Bundle ID 一致的 Explicit App ID。

如果直接这样签名，Xcode 的 Signing & Capabilities 中会出现 {{fig:xcode_wildcard_profile_error}} 的错误。请注意，Capability 名称和 Entitlement 的键是分别报告的。错误同时指出了 Profile 不包含该 Capability，以及没有与该键对应的许可这两点。

![][image3]
図: 尝试使用通配符 Provisioning Profile 签名时的错误 {{fig:xcode_wildcard_profile_error}}

在 Apple Developer 的 Certificates, Identifiers & Profiles 中选择目标 App ID，并如 {{fig:appid_increased_memory_limit}} 所示启用所需的 Capability。Apple 也说明，在 App ID 上设置的 Capability 就是应用可使用功能的许可列表，而需要 Explicit App ID 的功能无法在通配符下选择 {{fn:explicit_app_id}}。

![][image4]
図: 在 App ID 的设置中启用 Increased Memory Limit {{fig:appid_increased_memory_limit}}

手动签名时，在更改 Capability 之后需要重新生成 Provisioning Profile，并更新构建所使用的 Profile。自动签名时，需要确认选中的是以正确的 Team 和 Bundle ID 更新后的 Profile。如果保持旧的 Profile 不变，只向 `.entitlements` 添加键，那么所请求的权限与签名侧的许可就不会一致。在排查 Unity 侧的设置之前，先把这里对齐。

## **Extended Virtual Addressing 需要付费会员资格**

Extended Virtual Addressing 无法在将免费的 Apple Account 作为 Xcode 的 Personal Team 使用的配置中启用。
Apple 的 Capability 支持表中列出的
是 Apple Developer Program 和 Apple Developer Enterprise Program 的会员资格，
不包括免费的 Apple Account {{fn:capability_membership}}。

另一方面，Increased Memory Limit 在 Personal Team 下也可以启用。

# **在 Unity 的构建后处理中自动设置**

Entitlements 文件中需要写入的最小内容如 {{list:memory_entitlements}} 所示。这是同时启用两者的示例。

CODE START xml | 内存相关 Entitlement 的最小配置 {{list:memory_entitlements}}
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.developer.kernel.increased-memory-limit</key>
    <true/>
    <key>com.apple.developer.kernel.extended-virtual-addressing</key>
    <true/>
</dict>
</plist>
CODE END

虽然也可以在 Xcode 的 Signing & Capabilities 中手动添加，但每次导出都要重新添加，容易混入遗漏了设置的构建。在 Unity 中，可以通过构建后处理一并更新 Entitlements 文件以及引用它的 `CODE_SIGN_ENTITLEMENTS`。该处理在签名前的构建阶段运行，并不是在应用启动后由 C# 获取权限。

## **Unity 6000.0.11f1 及之后的版本使用专用 API**

在 Unity 6000.0.11f1 中，`ProjectCapabilityManager` 新增了以下方法 {{fn:unity_memory_api}}。

* `AddIncreasedMemoryLimit()`
* `AddExtendedVirtualAddressing()`

使用这些方法，可以一并完成向 Entitlements 添加 Boolean 值以及将其应用到 Xcode 项目中。

也提供了对应的 `PBXCapabilityType` 定义。不过，这并不是 C# 的 enum，而是表示 Capability 要求的类。将该值传给 `PBXProject.AddCapability()`，与向 Entitlements 添加所需的键并不是同一回事。因此，基本上应使用同时处理两者的 `ProjectCapabilityManager`。

下面是使用 `ProjectCapabilityManager` 添加 Capability 的示例。

CODE START csharp | 在 Unity 6000.0.11f1 及之后的版本中添加内存 Capability 的构建后处理 {{list:unity_memory_postprocess}}
#if UNITY_IOS
using UnityEditor;
using UnityEditor.Callbacks;
using UnityEditor.iOS.Xcode;

static class IosMemoryCapabilities
{
    [PostProcessBuild]
    static void OnPostprocessBuild(BuildTarget target, string buildPath)
    {
        if (target != BuildTarget.iOS) return;

        string pbxPath = PBXProject.GetPBXProjectPath(buildPath);
        var pbx = new PBXProject();
        pbx.ReadFromFile(pbxPath);
        string mainTarget = pbx.GetUnityMainTargetGuid();

        string entitlements = pbx.GetBuildPropertyForAnyConfig(
            mainTarget, "CODE_SIGN_ENTITLEMENTS");
        if (string.IsNullOrEmpty(entitlements))
            entitlements = "Unity-iPhone.entitlements";

        var capabilities = new ProjectCapabilityManager(
            pbxPath, entitlements, targetGuid: mainTarget);
        capabilities.AddIncreasedMemoryLimit();
        capabilities.AddExtendedVirtualAddressing();
        capabilities.WriteToFile();
    }
}
#endif
CODE END

`UnityEditor.iOS.Xcode` 包含在 iOS Build Support 中，因此需要用 `#if UNITY_IOS` 包裹。否则，在未安装该模块的环境中，整个编辑器的编译都无法通过，请注意。

此示例面向的配置是：所有构建配置共用一个 Entitlements 文件，并以相对于 Xcode 项目的相对路径指定其路径。如果 Debug 和 Release 使用不同的文件，或路径中包含构建变量，请相应调整处理，使其写入各配置实际使用的文件。

最后的保存由 `capabilities.WriteToFile()` 完成。如果在其他构建后处理中也编辑 Xcode 项目，
重新保存 `pbx` 时可能会覆盖 Capability Manager 已保存的更改，
因此请注意，要先重新读取更新后的文件再进行修改。

## **没有专用 API 的版本则直接更新 plist**

要在没有专用 API 的编辑器中执行相同处理，可将 {{list:unity_memory_postprocess}} 中从生成 `ProjectCapabilityManager` 到保存为止的 5 行替换为 {{list:unity_memory_legacy}}。

CODE START csharp | 在没有专用 Capability API 的 Unity 中的替代处理 {{list:unity_memory_legacy}}
string fullPath = System.IO.Path.Combine(buildPath, entitlements);
System.IO.Directory.CreateDirectory(
    System.IO.Path.GetDirectoryName(fullPath));
var plist = new PlistDocument();
if (System.IO.File.Exists(fullPath))
    plist.ReadFromFile(fullPath);

plist.root.SetBoolean(
    "com.apple.developer.kernel.increased-memory-limit", true);
plist.root.SetBoolean(
    "com.apple.developer.kernel.extended-virtual-addressing", true);
plist.WriteToFile(fullPath);

if (string.IsNullOrEmpty(pbx.FindFileGuidByProjectPath(entitlements)))
    pbx.AddFile(entitlements, entitlements);
pbx.SetBuildProperty(mainTarget, "CODE_SIGN_ENTITLEMENTS", entitlements);
pbx.WriteToFile(pbxPath);
CODE END

先读取现有文件再添加两个键，这样可以在不删除 Push 通知等 Entitlement 的情况下进行更新。修改的是 `.entitlements`，而不是 `Info.plist`。此外，该文件只有在被 `CODE_SIGN_ENTITLEMENTS` 引用之后才会用于签名。

仅添加 C# 代码并不会连账号侧的权限也一并启用。
无论采用哪种方法，Apple Developer 侧的 App ID 和 Provisioning Profile 都需要另行设置。

# **分两个阶段确认是否生效**

设置是否已生效，分为两个阶段确认：签名中是否包含了键，以及真机上内核是否已实际采用。

## **确认是否包含在签名中**

无论是手动操作还是构建后处理，仅在项目内生成了 {{list:memory_entitlements}} 文件，并不能证明设置已应用到安装在真机上的应用中。Entitlement 最终会嵌入代码签名。导出的 `.ipa` 是 zip 格式，因此将其解压后，对 `Payload` 内的应用本体执行 {{list:codesign_entitlements}}。

CODE START bash | 确认已签名应用中嵌入的 Entitlement {{list:codesign_entitlements}}
unzip -q YourApp.ipa -d ipa
codesign --display --entitlements :- ipa/Payload/YourApp.app
CODE END

在输出的 plist 中，确认是否包含 {{list:memory_entitlements}} 的两个键。Apple 也建议在签名后使用此命令进行确认 {{fn:diagnosing_entitlements}}。

## **在真机上确认上限是否已变化**

即使在 {{list:codesign_entitlements}} 中能看到键，也不代表内核已实际采用。Increased Memory Limit 在设备侧不支持时，即使签名正确也会被视为无效。

在真机上是否生效，通过直接读取内核为本进程持有的值来判断。获取该值的是 `task_info()`，这是 iOS SDK 中标准提供的 Mach API（`<mach/mach.h>`）。由于无法从 C# 直接调用，因此由放在 `Assets/Plugins/iOS/` 中的 `.mm` 读取后再返回给 C#。

CODE START objc | 读取生效状态的原生插件 {{list:task_vm_info_probe}}
#import <mach/mach.h>

extern "C" void UPT_ReadLimits(uint64_t *memlimit, uint64_t *ceiling)
{
    task_vm_info_data_t i;
    mach_msg_type_number_t c = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&i, &c) != KERN_SUCCESS) {
        return;
    }

    // footprint 上限（Increased Memory Limit）
    *memlimit = i.limit_bytes_remaining + i.phys_footprint;
    // 地址空间上限：若已扩大到 0xF_C000_0000 则表示已扩展
    *ceiling  = i.max_address;
}
CODE END

C# 侧通过 `DllImport("__Internal")` 声明 `UPT_ReadLimits` 并调用。

应查看的值是上限。`os_proc_available_memory()` 返回的剩余量会随着 footprint 的增加而相应减小，因此仅看该值无法区分是上限没有提高，还是只是用得较多。

`limit_bytes_remaining` 与该 `os_proc_available_memory()` 是在内核中由同一函数得到的值 {{fn:xnu_memorystatus}}，因此加上当前的 footprint 即可还原出当前生效的上限。地址空间侧的 `max_address` 也可以从同一结构体中获取，因此两个 Entitlement 可以通过一次调用确认。`max_address` 只有在传入 `TASK_VM_INFO_COUNT` 时才会被填充。

不过，无法从 `max_address` 中单独分离出 Extended Virtual Addressing 的效果。添加了 Increased Memory Limit 的构建，或启用了 Xcode 的 Enhanced Security 的构建，也会得到相同的上限。能够确认的只是“上限是否已扩大到 63GB”。

# **在 iOS 26.4 中显现的虚拟地址预留**

## **`JS VM Gigacage (reserved)` 并不是 1GB 的 RAM 使用量**

在搭载 iOS 26.4 的 3GB iPhone 上进行验证时，崩溃报告的 VM Region Summary 中出现了约 1GB 的 `JS VM Gigacage (reserved)` 区域，而启用 Extended Virtual Addressing 后可以避免 OOM 发生时内存量的下降。
该区域并不是真正的 Gigacage，而是复用了现有 VM tag 名称来显示的 JavaScriptCore 的 Structure Heap 预留 {{fn:apple_forum_826325}}。

## **WebKit 的修复根据设备 RAM 缩小预留量**

JavaScriptCore 的公开 bug `315240` 中记录了这样的方针：由于在低内存设备上 Structure Heap 的预留占据了虚拟地址空间的很大比例，因此根据物理内存量缩小预留 {{fn:webkit_bug_315240}}。

在应对该问题的 WebKit 提交中，对于 iOS 系列平台，取基础预留大小与物理内存 20% 中的较小者，
再向下取整到 2 的幂，以此决定 Structure Heap 的期望预留大小。
公开提交的说明中给出了在 3GB 设备上为 512MB 的例子 {{fn:webkit_commit_structure_heap}}。

在包含该修复的 iOS 26.6.1 的 iPhone SE（第 2 代，RAM 3GB）上测量时，显示的预留从约 1GB 缩小到 512MB，
在 26.4 中下降的 OOM 发生时内存量，即使不启用 Extended Virtual Addressing 也已恢复。

# **总结**

Increased Memory Limit 和 Extended Virtual Addressing 是启用后几乎没有损失的设置。页表只会为实际映射的范围创建，因此扩大地址空间并不会增加成本。它们也有通过 App Store 审核并上线发行的先例。只要准备好 Explicit App ID 和 Profile，笔者认为基本上两者都应启用。

不过，效果取决于设备。在笔者的 iPhone 测量中，Increased Memory Limit 带来的余量在 4GB 设备上约为 240MB，在 6GB 设备上约为 1GB，而在 3GB 设备上没有差别。iPad 需要另行测量。Extended Virtual Addressing 不会增加物理内存，只在虚拟地址空间先行不足的情况下才有效果。iOS 26.4 中出现的 Structure Heap 预留问题在 iOS 26.6 系列版本中已得到缓解，但在需要支持旧版 OS，或应用自身的内存映射占用大量地址空间的情况下，仍然可以期待效果。

两者都不会减少内存使用量本身。为了在没有额外上限的设备上也能持续运行，仍然需要另行开展降低纹理和临时 RenderTexture 的峰值、缩短资源的常驻时间、减少原生插件的大型 allocation 以及缩短文件映射寿命等工作。这两个 Entitlement 是在此基础上用于扩大剩余余量的设置。

脚注: {{fn:increased_memory_limit}} Apple Developer Documentation, `com.apple.developer.kernel.increased-memory-limit` [https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.increased-memory-limit](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.increased-memory-limit)

脚注: {{fn:extended_virtual_addressing}} Apple Developer Documentation, Extended Virtual Addressing Entitlement [https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.extended-virtual-addressing](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.extended-virtual-addressing)

脚注: {{fn:os_proc_available_memory}} Apple Developer Documentation, `os_proc_available_memory` [https://developer.apple.com/documentation/os/os_proc_available_memory](https://developer.apple.com/documentation/os/os_proc_available_memory)

脚注: {{fn:diagnosing_entitlements}} Apple Developer Documentation, Diagnosing Issues with Entitlements [https://developer.apple.com/documentation/bundleresources/diagnosing-issues-with-entitlements](https://developer.apple.com/documentation/bundleresources/diagnosing-issues-with-entitlements)

脚注: {{fn:apple_forum_826325}} Apple Developer Forums, iPadOS 26.4+ significantly reduced per-app memory limit from 6GB to 3GB on 8GB iPad, breaking memory-intensive apps [https://developer.apple.com/forums/thread/826325](https://developer.apple.com/forums/thread/826325)

脚注: {{fn:webkit_bug_315240}} WebKit Bugzilla 315240, Scale down StructureAlignedMemoryAllocator reservation in proportion to phys-mem size [https://bugs.webkit.org/show_bug.cgi?id=315240](https://bugs.webkit.org/show_bug.cgi?id=315240)

脚注: {{fn:webkit_commit_structure_heap}} WebKit commit `cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b`, Scale down structure-heap reservation size with physical memory [https://github.com/WebKit/WebKit/commit/cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b](https://github.com/WebKit/WebKit/commit/cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b)

脚注: {{fn:apple_game_memory}} Apple Developer Tech Talks, Bring your high-end game to iPhone 15 Pro [https://developer.apple.com/videos/play/tech-talks/111372/](https://developer.apple.com/videos/play/tech-talks/111372/)

脚注: {{fn:explicit_app_id}} Apple Developer, Register an App ID [https://developer.apple.com/help/account/identifiers/register-an-app-id/](https://developer.apple.com/help/account/identifiers/register-an-app-id/)

脚注: {{fn:capability_membership}} Apple Developer, Supported capabilities (iOS) [https://developer.apple.com/help/account/reference/supported-capabilities-ios/](https://developer.apple.com/help/account/reference/supported-capabilities-ios/)

脚注: {{fn:unity_memory_api}} Unity 6.0 Scripting API, ProjectCapabilityManager [https://docs.unity3d.com/6000.0/Documentation/ScriptReference/iOS.Xcode.ProjectCapabilityManager.html](https://docs.unity3d.com/6000.0/Documentation/ScriptReference/iOS.Xcode.ProjectCapabilityManager.html)

脚注: {{fn:unity_capability_release}} Unity 6000.0.11f1 Release Notes, iOS: Updated capability list（UUM-65394）。API 的新增内容通过比较官方发布的 6000.0.10f1 与 6000.0.11f1 的 iOS Build Support 中的 UnityEditor.iOS.Extensions.Xcode.dll 确认。[https://unity.com/releases/editor/whats-new/6000.0.11f1](https://unity.com/releases/editor/whats-new/6000.0.11f1)

脚注: {{fn:xnu_memorystatus}} Apple Open Source, xnu-12377.121.6, `bsd/kern/kern_memorystatus.c`（`memorystatus_available_memory_internal`） [https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/bsd/kern/kern_memorystatus.c](https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/bsd/kern/kern_memorystatus.c)

脚注: {{fn:xnu_pmap}} Apple Open Source, xnu-12377.121.6, `osfmk/arm/pmap/pmap.c`（`pmap_max_64bit_offset`） [https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/osfmk/arm/pmap/pmap.c](https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/osfmk/arm/pmap/pmap.c)

[image1]: assets/kim-minhyuk-2/media/image1.png
[image2]: assets/kim-minhyuk-2/media/image2.png
[image3]: assets/kim-minhyuk-2/media/image3.png
[image4]: assets/kim-minhyuk-2/media/image4.png
