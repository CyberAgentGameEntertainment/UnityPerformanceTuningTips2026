# **原稿**

# **在 Unity 中利用 iOS 标准性能测量功能**

* 姓名: Yamazaki Masahiro  
* 所属: 株式会社 Applibot  
* X: [https://x.com/TEST\_H\_](https://x.com/TEST_H_)  
* GitHub: [https://github.com/mao-test-h](https://github.com/mao-test-h)

# **前言**

Unity 本身提供了以 Unity Profiler 为代表的测量功能，但对于面向 iOS 的应用，还可以额外利用 OS 自带的性能测量功能。

Apple 提供的这些功能的特点是，无需依赖 Unity 的 Development Build 标志即可使用。  
例如，可以在尚未集成测量处理的阶段，或在禁用调试功能、接近发布版本的配置下粗略确认开销，也可以只从测试人员的设备接收 Trace 结果。像这样把握大致的开销、在一定程度上缩小瓶颈候选范围之后，再切换到启用 Development Build 的构建，用 Unity Profiler 或 Instruments 进行详细调查，这样分阶段使用是可行的。

本文将从 iOS 标准提供的功能中介绍以下 2 项，并进一步讲解 macOS 27 / iOS 27 新增的功能。

* Metal Performance HUD  
* Performance Trace

此外，笔者还公开了“MetalPerfKit-Unity”{{fn:metalperfkit_unity}}，这是一个让这些功能可以在 Unity 中使用的开源示例仓库，实现细节请一并参考。

※ 以下说明统一使用 iOS 这一表述，但除非特别说明，也包含 iPadOS。

## **验证环境**

本文在以下环境中进行了运行验证。

* Unity 6000.0.81f1  
* Xcode 26.6 / 27.0 beta  
* iOS 26.6 / 27.0 beta  
* macOS 27.0 beta

※ 截至撰写时，iOS 27、macOS 27、Xcode 27 以及后文介绍的 \`StateReporter\` API 仍处于 Beta 阶段。本文内容基于 2026 年 8 月时的信息，在正式发布之前，API、命令、设置界面及行为等均可能发生变化。

# **Metal Performance HUD**

Metal Performance HUD 是一款工具，可将使用 Metal 的应用程序的性能信息实时叠加显示在屏幕上，或以日志形式输出。在 Apple 的文档 {{fn:hud_monitoring_doc}} 中，它被介绍为一项用于快速把握当前状态、找出需要用 Xcode 或 Instruments 详细捕获的范围的功能。

该功能本身在 Metal 4 之前（iOS 16 时代）就已存在，但随着 2025 年 WWDC 上发布的 Metal 4（iOS 26 时代），得到了大幅增强。Metal 4 之前与 Metal 4 及之后的显示差异见 {{fig:perf_hud_legacy}} 和 {{fig:perf_hud_metal4}}。

![][image1]{scale=0.5}

図: Metal 4 之前的 Metal Performance HUD {{fig:perf_hud_legacy}}

![][image2]{scale=0.5}

図: Metal 4 及之后的 Metal Performance HUD {{fig:perf_hud_metal4}}

Metal 4 起的主要增强点包括：测量项目的扩充，显示位置、不透明度、缩放的自定义，以及可通过 API 动态控制的项目增加等。

显示主要指标时的示例见 {{fig:perf_hud_metrics_all}}。

![][image3]{scale=1.0}

図: 显示主要指标时的示例 {{fig:perf_hud_metrics_all}}

各指标的详细说明请参阅官方文档“Understanding the Metal Performance HUD metrics”{{fn:hud_metrics_doc}}。

此外，笔者此前也公开过关于 Metal Performance HUD 的讲解文章 {{fn:qiita_gpu_measurement}} 和技术同人志“UniTips Vol.12”{{fn:unitips_vol12}}。其中讲解了从基本用法到面向 Unity 的使用技巧等内容，详情也可参考。

后续章节将以 Metal 4（iOS 26 世代）的内容为基础进行讲解。

## **如何打开 HUD**

显示 HUD 的方法有多种，这里讲解“从设置应用打开”和“通过 API 控制显示”这 2 种方法。

### **从设置应用打开**

可以在 iOS 设备的设置应用中启用。步骤如下。

※ 前提是需要已显示 \`Developer\` 菜单。此外，菜单名称可能因 OS 版本而异。

1. 打开 iOS 的“设置”应用  
2. 打开 \`Developer\` 菜单，再打开其中的 \`Metal HUD\`（参见 {{fig:settings_developer_menu}}）

![][image4]{scale=0.4}

図: 从 Developer 菜单打开 Metal HUD {{fig:settings_developer_menu}}

打开 \`Metal HUD\` 后会显示 {{fig:settings_metal_hud}} 界面，启用其中的 \`Show Metal Performance HUD\` 并重新启动目标应用，即可显示。

在该界面中，还可以切换要显示的指标，以及更改不透明度、缩放和显示位置。此外，启用同一区块中的 \`Log Metal Performance\` 后，可按固定间隔将性能信息输出为日志。详情将在后文“关于日志功能”中讲解。

![][image5]{scale=0.4}

図: Metal HUD 的设置界面 {{fig:settings_metal_hud}}

### **通过 API 控制显示**

如果想在应用运行期间动态控制 HUD 的显示，可向 \`CAMetalLayer\` 类的 \`developerHUDProperties\` 属性传入设置值。{{list:hud_properties_basic}} 中启用了 HUD 和日志输出，并将显示位置指定为左上角。

CODE START swift | 通过 developerHUDProperties 控制 HUD 的显示 {{list:hud_properties_basic}}

myMetalLayer.developerHUDProperties \= \[  
   "mode": "default",      // HUD 的 ON/OFF  
   "logging": "default",   // 日志功能的 ON/OFF  
   "positionX": 0,         // 显示位置（以像素为单位的 X 坐标）  
   "positionY": 0,         // 显示位置（以像素为单位的 Y 坐标）  
\]

CODE END

若要隐藏 HUD，则为 \`mode\` 指定 \`disabled\`。以往通过该属性只能控制 HUD 与日志功能的 ON/OFF，无法调整位置等，但从 Metal 4 起，可以通过 \`positionX\` / \`positionY\` 以像素为单位指定位置。

此外，该 Dictionary 还可以传入 HUD 支持的环境变量键。例如传入 \`MTL\_HUD\_OPACITY\` 和 \`MTL\_HUD\_SCALE\`，即可在 0.0〜1.0 的范围内调整不透明度和缩放（{{list:hud_properties_env}}）。

CODE START swift | 通过环境变量键调整不透明度与缩放 {{list:hud_properties_env}}

myMetalLayer.developerHUDProperties \= \[  
   ...,  
   "MTL\_HUD\_OPACITY": opacity,     // 不透明度（0.0〜1.0）  
   "MTL\_HUD\_SCALE": scale,         // 缩放（0.0〜1.0）  
\]

CODE END

此外还有许多可设置的项目，详情请参阅官方文档“Customizing the Metal Performance HUD”{{fn:hud_customizing_doc}}。

从 Unity 调用时，需要通过 iOS 原生插件获取用于绘制的 \`CAMetalLayer\`，再从 C\# 侧通过 P/Invoke 调用。\`CAMetalLayer\` 可经由 \`UnityFramework\` 按 {{list:get_metal_layer}} 所示的方式获取。

CODE START swift | 经由 UnityFramework 获取 CAMetalLayer {{list:get_metal_layer}}

private func getMetalLayer() \-\> CAMetalLayer? {  
   guard let instance \= UnityFramework.getInstance(),  
         let rootView \= instance.appController().rootView,  
         let metalLayer \= rootView.layer as? CAMetalLayer else {  
       return nil  
   }  
   return metalLayer  
}

CODE END

MetalPerfKit-Unity 将这些实现公开为 \`PerformanceHUDSwitcher\` 类，从 C\# 可按 {{list:hud_switcher_csharp}} 所示的方式调用。另外，原生 API 以像素为单位指定位置，而 \`PerformanceHUDSwitcher\` 为了不依赖分辨率，接收 0.0〜1.0 的相对位置坐标，并在内部根据绘制区域的大小转换为像素坐标。

CODE START csharp | PerformanceHUDSwitcher 的调用示例 {{list:hud_switcher_csharp}}

// 将 HUD 显示在屏幕右上角（x=1.0, y=0.0）  
// （坐标以 0.0〜1.0 的相对位置指定）  
PerformanceHUDSwitcher.SetPerformanceHUDVisible(true, 1.0f, 0.0f);

CODE END

完整的实现示例请参阅 MetalPerfKit-Unity 的源代码。

## **关于日志功能**

启用前述的 \`Log Metal Performance\` 或 API 的 \`logging\` 后，应用运行期间会以 1 秒间隔将性能信息输出为日志。日志以 \`metal-HUD:\` 开头、按 {{list:hud_log_format}} 的格式输出，可在 Xcode 的控制台或 macOS 的 Console.app 中查看。

CODE START text | metal-HUD 日志的输出格式 {{list:hud_log_format}}

metal-HUD: \<first-frame-number-integer\>,\<graphics-memory-usage-float\>,\<process-memory-usage-float\>,\<first-frame-present-interval-float\>,\<first-frame-gpu-time-float\>,...\<last-frame-present-interval-float\>,\<last-frame-gpu-time-float\>

CODE END

从开头起依次记录“开始采样的帧编号”“图形内存使用量”“整个进程的内存使用量”，之后则重复记录所采样各帧的 Frame Present Interval 与 GPU Time 的配对。实际的输出示例见 {{fig:perf_hud_log_sample}}。

![][image6]{scale=1.0}

図: Metal Performance HUD 的日志输出示例 {{fig:perf_hud_log_sample}}

将其作为逗号分隔的数据进行汇总，即可在事后分析帧间隔的紊乱、GPU Time 以及内存使用量的变化。此外，只要这些日志仍保留在 Unified Logging 中，就可以利用 \`OSLogStore\` API 实现“获取过去 N 秒的日志并保存到文件”等功能。MetalPerfKit-Unity 以 \`PerformanceLogger\` 类的形式提供了该功能（{{list:fetch_performance_logs}}）。

CODE START csharp | 通过 PerformanceLogger 获取并保存日志 {{list:fetch_performance_logs}}

// 获取过去 60 秒的日志并保存到文件  
PerformanceLogger.FetchPerformanceLogs(60, FileUtility.GenerateFetchLoggingFilePath());

CODE END

# **Performance Trace**

Performance Trace 是 iOS 26 新增的功能，无需连接 Mac，仅凭 iOS 设备即可收集与性能相关的数据。收集到的 Trace 文件可传输到 Mac，用 Instruments 或后文介绍的 \`metalperftrace\` 命令进行分析。

由于在设备上即可完成，除了可以记录长时间游玩等在连接 Mac 的状态下难以复现的情况之外，还可以让测试人员等他人在其设备上收集 Trace 文件并共享，然后在本地进行分析。

本章将重点介绍其中在 WWDC 2026 上随功能增强一同讲解的 Lookback Collection。

## **使用方法**

Performance Trace 需要从 \`Developer\` 菜单启用。步骤如下。

1. 打开 iOS 的“设置”应用  
2. 打开 \`Developer\` 菜单，再打开其中的 \`Performance Trace\`  
3. 启用 \`Enable Performance Trace\`（切换启用/禁用需要重新启动设备）  
4. 选择任意 \`Tracing Mode\`（详见后文）

\`Developer\` 菜单中的 \`Performance Trace\` 及其设置界面分别见 {{fig:trace_developer_menu}} 和 {{fig:trace_enable}}。

![][image7]{scale=0.4}

図: Developer 菜单中的 Performance Trace {{fig:trace_developer_menu}}

![][image8]{scale=0.4}

図: Enable Performance Trace 的设置界面 {{fig:trace_enable}}

启用完成后，为了进行 Trace 的开始/停止以及 Trace 数据的收集等操作，接下来需要将 \`Performance Trace\` 添加到控制中心。打开控制中心，即可从 \`Developer\` 区块中添加。添加步骤与添加后的状态见 {{fig:trace_add_control}} 和 {{fig:trace_control}}。

![][image9]{scale=0.4}

図: 将 Performance Trace 添加到控制中心 {{fig:trace_add_control}}

![][image10]{scale=0.4}

図: 添加 Performance Trace 控件后的状态 {{fig:trace_control}}

## **关于 Tracing Mode**

\`Tracing Mode\` 有 {{table:tracing_modes}} 所示的 4 种（System Activity 分为 Summary / Detailed 2 种），也可以通过长按控制中心的 \`Performance Trace\` 控件来切换。

表: Tracing Mode 的种类 {{table:tracing_modes}}

| 模式 | 概要 |
| :---- | :---- |
| Power Profiler | 记录对电量消耗的影响 |
| System Activity (Summary / Detailed) | 记录系统的运行状况 |
| Lookback Collection | 回溯收集系统持续记录的数据 |

Power Profiler 和 System Activity 通过点击控制中心的 \`Performance Trace\` 控件开始记录，再次点击则停止。而 Lookback Collection 无需记录操作，会从点击的时间点向前回溯收集数据。详情将在后文说明。

收集到的 Trace 文件会在 Performance Trace 界面的 \`Available Trace Files\` 中列出，可通过共享按钮经 AirDrop 等方式传输到 Mac 或其他开发者。解压共享的归档后，即可得到可用 Instruments 打开的 \`.atrc\` 文件。另外，当设备存储空间不足时 Trace 文件会被自动删除，也可以通过滑动操作手动删除。

关于各模式的补充说明如下。

### **Power Profiler**

Power Profiler 是测量应用对 CPU / GPU / 显示 / 网络等各子系统的使用给设备电量消耗带来的影响（Power Impact）的模式。本文不展开详述，官方文档“Measuring your app’s power use with Power Profiler”{{fn:power_profiler_doc}} 和 WWDC 2025 的 Session“Profile and optimize power usage in your app”{{fn:wwdc2025_power}} 中有详细讲解。

### **System Activity**

System Activity 是提供“Summary”和“Detailed”2 种选项的 Trace 模式。  
就笔者确认的范围而言，截至撰写时（2026 年 8 月），Apple 的文档和 WWDC Session 中尚未见到关于 System Activity 的详细规范，以及 Summary 与 Detailed 之间差异的说明。因此，本文不深入各模式的细节，仅介绍在实际 Trace 中能够确认的信息。

作为参考，在搭载 iOS 27 的设备上记录 System Activity (Summary) 的 Trace，并用后文介绍的 \`metalperftrace\` 命令分析的结果见 {{list:overview_system_activity}}。

CODE START text | System Activity (Summary) Trace 的分析结果 {{list:overview_system_activity}}

$ metalperftrace overview system\_activity\_summary\_ios27.atrc  
\[MetalPerfKit-Unity pid:1974\]  
   Mem: 799.0 MiB (799.0 Peak, 616.8 Metal, 2577.0 Headroom)  
   Total CPU Time: 12.679s (20.47% Sys, 79.53% User)  
   Instructions: 43.33B (60.88% P, 39.12% E)  
   Cycles: 21.04B (38.97% P, 61.03% E)  
   Disk Reads / Writes: 9.13 / 0.08 MiB (Logical Write 0.07)  
   \[States\]  
     com.DefaultCompany.MetalPerfKit-Unity.graphics: 3 transitions, last known state: URP-HighFidelity  
     com.DefaultCompany.MetalPerfKit-Unity.screen: 11 transitions, last known state: Title  
 UnityView (UnityDefaultViewController) (1290x2796)  Interval 30.037s  Active 30.037s  
   44.5 FPS  1202 Frames  1202 Detached  11 Skipped  
   Frame Time           avg:  22.47ms  min:  16.67  max: 420.93  stddev:  30.68  p95:  50.01  p99: 133.36  
   CPU Begin-to-Present avg:  17.24ms  min:   1.14  max: 794.63  stddev:  31.76  p95:  31.28  p99: 111.73  
   On-GPU Time          avg:  15.50ms  min:   1.81  max: 424.87  stddev:  37.24  p95:  51.17  p99: 134.48  
   Next Drawable Wait   avg:  14.53ms  min:   0.00  max: 401.43  stddev:  32.11

CODE END

在该 Trace 中，除了各进程的内存使用量、CPU 时间、磁盘 I/O 之外，还可以确认以 \`CAMetalLayer\` 为单位的 FPS、Frame Time、CPU Begin-to-Present、On-GPU Time 等统计信息。另外，\`\[States\]\` 项中记录了应用的状态转换，这是由下一章讲解的 \`StateReporter\` API 产生的，详情将在后文说明。

### **Lookback Collection**

Lookback Collection 是事后回溯收集系统持续记录的性能数据的模式。  
就笔者确认的范围而言，Lookback Collection 在 iOS 26 时就已作为 \`Tracing Mode\` 的选项存在，但 WWDC 2025 和官方文档中并未对其进行讲解。之后，该功能在 WWDC 2026 上得到增强，并公开了讲解其内容的 Session 视频“Find and fix performance issues in your Metal games”{{fn:wwdc2026_metal_games}}。

据演讲介绍，游戏运行期间，系统会持续记录性能指标（CPU、GPU、FPS、内存等），并将数据保存数天。Lookback Collection 正是利用了这一机制，因此其特点是即使是测试人员的长时间游玩会话，也能回溯获取数据。该机制的概要见 {{fig:wwdc_lookback_metrics}}。

![][image11]{scale=1.0}

図: 系统对性能指标的持续记录（摘自 WWDC 2026 Session） {{fig:wwdc_lookback_metrics}}

使用方法只需在照常进行游玩测试之后，点击控制中心的 \`Performance Trace\` 控件即可。从点击的时间点起，回溯所设定的时长开始收集和处理 Trace，完成后会收到通知，并添加到 \`Available Trace Files\` 中。

在搭载 iOS 26 的设备上实际收集约 3 小时的 Lookback Collection Trace，并用 Instruments 打开的示例见 {{fig:lookback_process_list}}。回溯期间内曾启动的多个应用（进程）会以列表显示，可以总览各自的 FPS 等变化。

![][image12]{scale=1.0}

図: 用 Instruments 打开 Lookback Collection 的 Trace 的示例 {{fig:lookback_process_list}}

进一步展开进程后，可如 {{fig:lookback_frame_metrics}} 所示，以 \`CAMetalLayer\` 为单位确认 FPS、GPU Usage %、Frame Interval、CPU / GPU 时间等帧指标的变化。

![][image13]{scale=1.0}

図: 以 CAMetalLayer 为单位的帧指标变化 {{fig:lookback_frame_metrics}}

※ 在笔者的 iOS 27 环境中，汇总过程中会显示 \`Error processing Performance Trace\`，无法正常运行。关于该问题，即使在 iOS / iPadOS 27 正式发布时，发布说明的 \`Control Center -> Known Issues\` 中仍记载了“收集 Power Profiler 或 Lookback 的 Trace 时，可能不会生成 Trace”这一已知问题（184729556）。{{fn:ios_ipados_27_release_notes}}

# **macOS 27 / iOS 27 新增的功能**

如前一章所述，游戏运行期间系统会持续记录并保存 Metal 性能指标。在 WWDC 2026 上，以这一机制为核心发布了新功能：为 Trace 附加应用特有信息的 \`StateReporter\` API，以及可从 CLI 分析 Trace 的 \`metalperftrace\` 命令。

## **StateReporter API**

即使能从 Trace 数据中定位帧率下降的时刻，仅凭 Metal 性能指标也无法得知当时游戏在做什么。这是因为“游戏正打开哪个场景”“对象数量有多少”“图形设置是什么”之类的应用特有信息不会自动包含在 Metal 性能指标中。

使用 iOS 27 新增的 \`StateReporting\` 框架 {{fn:statereporting_doc}} 的 \`StateReporter\` API {{fn:statereporter_doc}}，可以向系统报告应用的状态，为 Trace 数据附加应用特有的上下文。

报告的状态在 \`metalperftrace\` 中用于按状态汇总，在 Instruments 中用于显示 Points of Interest。用 Instruments 打开前一章展示的 System Activity Trace 的示例见 {{fig:instruments_state_reporter}}。报告的状态以 Points of Interest 轨道的形式显示。

![][image14]{scale=1.0}

図: 报告的状态显示为 Points of Interest 的示例 {{fig:instruments_state_reporter}}

\`StateReporter\` 由 {{table:state_reporter_elements}} 所示的 4 个要素组成。

表: 组成 StateReporter 的 4 个要素 {{table:state_reporter_elements}}

| 要素 | 概要 | 示例 |
| :---- | :---- | :---- |
| Domain | 报告所针对的功能领域。采用反向 DNS 格式的字符串，每个 Domain 同时只能有 1 个活动状态 | \`com.mygame.level\` |
| Label | 表示当前状态的短字符串 | \`Level 1\` |
| Stable metadata | 在状态存续期间保持不变的元数据。与 Label 组合用于唯一标识状态 | \`{id: 1001}\` |
| Volatile metadata | 在状态内可能变化的易变元数据。更新时不会发生状态转换 | \`{health: 20}\` |

可以定义多个 Domain，例如将“关卡进度”“图形设置”“网络状态”分别作为独立的 Domain 报告，并组合起来进行分析。

在 Swift 中按 {{list:state_reporter_swift}} 所示的方式使用。

CODE START swift | 在 Swift 中使用 StateReporter 的示例 {{list:state_reporter_swift}}

import StateReporting  
// 获取与 Domain 对应的 Reporter  
let reporter \= StateReporter.reporter(for: "com.mygame.gameplay")  
// 报告状态转换  
reporter.reportTransition(to: "Playing")  
// 若要结束状态，则传入 nil  
reporter.reportTransition(to: nil)

CODE END

### **在 Unity 中使用**

\`StateReporting\` 是面向 Swift / Objective-C 的 API，因此要在 Unity 中使用需要原生插件。MetalPerfKit-Unity 将其封装为 \`StateReporter\` 类，作为预览功能公开，从 C\# 可按 {{list:state_reporter_csharp}} 所示的方式使用。

CODE START csharp | 在 C\# 中使用 StateReporter 的示例 {{list:state_reporter_csharp}}

using MetalPerfKit;  
// 获取与 Domain 对应的 Reporter  
var reporter \= StateReporter.ReporterForDomain($"{Application.identifier}.screen");  
// 报告状态转换（附带易变元数据）  
reporter.ReportTransitionToStateLabel(  
   "Main",  
   volatileMetadata: new Dictionary\<string, StateReporterMetadataValue\>  
   {  
       \["CubeCount"\] \= cubeCount,  
   });  
// 保持状态不变，仅更新易变元数据  
reporter.ReportVolatileMetadataUpdate(  
   new Dictionary\<string, StateReporterMetadataValue\>  
   {  
       \["CubeCount"\] \= cubeCount,  
   });

CODE END

此外，仓库的 \`Assets/\_Example/2\_MetalPerformanceMetrics\` 中提供了一个示例，将界面跳转（Title / Main / Settings）和图形质量（URP 的 3 个档位）分别作为独立的 Domain 报告。前一章展示的 System Activity Trace 中的 \`\[States\]\` 就是用该示例记录的。

## **metalperftrace**

\`metalperftrace\` 是 macOS 27 新增的 CLI 工具，除了分析 Trace 文件之外，还可以收集面向 macOS 的应用的性能指标。由于收集功能仅针对在 macOS 上运行的应用，本文不作介绍，但分析在 iOS 设备上收集的 Trace 文件时可以利用该工具。

### **关于 Trace 文件的分析**

分析 Trace 文件时使用 \`overview\` 子命令（{{list:metalperftrace_overview}}）。

CODE START bash | 通过 overview 子命令分析 Trace 文件 {{list:metalperftrace_overview}}

\# 分析已收集的 Trace 文件  
metalperftrace overview MyGameTrace.atrc

CODE END

输出内容如前一章所介绍。进一步传入 \`--include-state-transitions\`，还可以确认用 \`StateReporter\` 记录的状态转换详情。{{list:overview_state_transitions}} 是分析前述示例应用的 Trace 的例子，输出了切换到各图形质量的变更历史，以及在该状态下的停留时间和占比。

CODE START text | 包含状态转换详情的分析结果 {{list:overview_state_transitions}}

$ metalperftrace overview \--include-state-transitions system\_activity\_summary\_ios27.atrc  
\[MetalPerfKit-Unity pid:1974\]  
   ...  
   \[States\]  
     com.DefaultCompany.MetalPerfKit-Unity.graphics  
       URP-Balanced  (32.75%, 9.904s)  
         (2026-08-16T14:22:33.025Z → 2026-08-16T14:22:42.930Z)  
       URP-Performant  (29.43%, 8.900s)  
         ...  
       URP-HighFidelity  (37.82%, 11.437s)  
         ...  
     com.DefaultCompany.MetalPerfKit-Unity.screen  
       Title  (5.07%, 1.535s)  
         ...  
       Main  (21.09%, 6.379s)  
         ...  
       Title  (2.22%, 0.671s)  
         ...  
       Settings  (6.28%, 1.899s)  
         ...

CODE END

※ 输出内容经过部分摘录和整理后刊载。实际上相同状态也会按时间顺序重复输出，并各自附有开始和结束的时间戳。

此外，传入 \`--aggregate\` 可按状态汇总指标。指定 \`--domain com.mygame.graphics\` 可筛选特定的 Domain，再配合 \`--state-label "High"\` 则可进一步筛选到特定状态。像“只有图形设置为 High 时平均 FPS 才下降”这样的应用状态与性能之间的相关性，用 1 条命令即可确认。

此外，传入 \`--json\` 即可以 JSON 格式获取这些输出。演讲中也提到“结构化的输出可用于通过脚本进行回归测试，以及由 AI 智能体自动分诊性能问题”。Apple 的“Metal Developer Tools”页面 {{fn:metal_developer_tools}} 也以“Hello, agentic debugging”为题，介绍了与智能体结合的工作流。

脚注: {{fn:metalperfkit_unity}} MetalPerfKit-Unity [https://github.com/mao-test-h/MetalPerfKit-Unity](https://github.com/mao-test-h/MetalPerfKit-Unity)

脚注: {{fn:hud_monitoring_doc}} Monitoring your Metal app’s graphics performance [https://developer.apple.com/documentation/xcode/monitoring-your-metal-apps-graphics-performance](https://developer.apple.com/documentation/xcode/monitoring-your-metal-apps-graphics-performance)

脚注: {{fn:hud_metrics_doc}} Understanding the Metal Performance HUD metrics [https://developer.apple.com/documentation/xcode/understanding-metal-performance-hud-metrics](https://developer.apple.com/documentation/xcode/understanding-metal-performance-hud-metrics)

脚注: {{fn:qiita_gpu_measurement}} 【Unity】在移动端应用上测量 GPU 开销 [https://qiita.com/mao\_/items/3b531d17cacb86036b7f](https://qiita.com/mao_/items/3b531d17cacb86036b7f)

脚注: {{fn:unitips_vol12}} UniTips Vol.12（在“第2章: Metal 4 起增强的 Metal Performance HUD 的 Unity 使用技巧”中讲解） [https://techbookfest.org/product/8mHpXz8LJCKAsLxENJKwaz?productVariantID=ewK5GtNTjkz4k8heM4BrxU](https://techbookfest.org/product/8mHpXz8LJCKAsLxENJKwaz?productVariantID=ewK5GtNTjkz4k8heM4BrxU)

脚注: {{fn:hud_customizing_doc}} Customizing the Metal Performance HUD [https://developer.apple.com/documentation/xcode/customizing-metal-performance-hud](https://developer.apple.com/documentation/xcode/customizing-metal-performance-hud)

脚注: {{fn:power_profiler_doc}} Measuring your app’s power use with Power Profiler [https://developer.apple.com/documentation/xcode/measuring-your-app-s-power-use-with-power-profiler](https://developer.apple.com/documentation/xcode/measuring-your-app-s-power-use-with-power-profiler)

脚注: {{fn:wwdc2025_power}} WWDC 2025 \- Profile and optimize power usage in your app [https://developer.apple.com/videos/play/wwdc2025/226/](https://developer.apple.com/videos/play/wwdc2025/226/)

脚注: {{fn:wwdc2026_metal_games}} WWDC 2026 \- Find and fix performance issues in your Metal games [https://developer.apple.com/videos/play/wwdc2026/388/](https://developer.apple.com/videos/play/wwdc2026/388/)

脚注: {{fn:ios_ipados_27_release_notes}} iOS & iPadOS 27 Release Notes [https://developer.apple.com/documentation/ios-ipados-release-notes/ios-ipados-27-release-notes](https://developer.apple.com/documentation/ios-ipados-release-notes/ios-ipados-27-release-notes)

脚注: {{fn:statereporting_doc}} Getting started with StateReporting [https://developer.apple.com/documentation/StateReporting/getting-started-with-statereporting](https://developer.apple.com/documentation/StateReporting/getting-started-with-statereporting)

脚注: {{fn:statereporter_doc}} StateReporter [https://developer.apple.com/documentation/statereporting/statereporter](https://developer.apple.com/documentation/statereporting/statereporter)

脚注: {{fn:metal_developer_tools}} Metal Developer Tools [https://developer.apple.com/metal/tools/](https://developer.apple.com/metal/tools/)

[image1]: assets/yamazaki-masahiro-1/media/image1.jpg

[image2]: assets/yamazaki-masahiro-1/media/image2.jpg

[image3]: assets/yamazaki-masahiro-1/media/image3.jpg

[image4]: assets/yamazaki-masahiro-1/media/image4.jpg

[image5]: assets/yamazaki-masahiro-1/media/image5.jpg

[image6]: assets/yamazaki-masahiro-1/media/image6.png

[image7]: assets/yamazaki-masahiro-1/media/image7.jpg

[image8]: assets/yamazaki-masahiro-1/media/image8.jpg

[image9]: assets/yamazaki-masahiro-1/media/image9.jpg

[image10]: assets/yamazaki-masahiro-1/media/image10.jpg

[image11]: assets/yamazaki-masahiro-1/media/image11.png

[image12]: assets/yamazaki-masahiro-1/media/image12.png

[image13]: assets/yamazaki-masahiro-1/media/image13.png

[image14]: assets/yamazaki-masahiro-1/media/image14.png