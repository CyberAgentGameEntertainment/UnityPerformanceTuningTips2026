# **根据设备切换渲染设置**

* NAOKI SAKUMA / 株式会社 CyberAgent SGE 核心技术本部

面向智能手机的游戏预计会在各种各样的设备上运行。  
如果一律采用相同的渲染设置，在某些情况下会导致低端设备崩溃、掉帧、发热。  
反过来，在高端设备上则有可能充分利用设备性能，获得更好的画质。  
因此，本文介绍一种根据设备切换到合适渲染设置的机制。

# **切换的方法**

游戏中常见的方法有两种：低、中、高这样的预设形式，以及让用户直接调整数值的形式。

## **预设形式**

这是以预设为单位进行切换时的方法。  
无论是让用户选择预设，还是根据设备自动判定，都使用这种方法。

在 ProjectSettings 的 Quality 中点击 Add Quality Level，按预设数量添加。  
然后，为各个预设附加对应的 RenderPipelineAsset。  
根据需要，也可以设置 Quality 中的其他项目（Rendering、Textures 等）。  
![][image1]  
在代码中调用 QualitySettings.SetQualityLevel，即可切换到任意 QualityLevel。

## **直接调整数值的形式**

需要让用户调整参数时，采用这种方法。  
通过如下代码，可以直接调整参数。

CODE START csharp  
var urpAsset \= (UniversalRenderPipelineAsset)GraphicsSettings.currentRenderPipeline;  
// 修改分辨率  
urpAsset.renderScale \= value;  
// 修改阴影级联的数量  
urpAsset.shadowCascadeCount \= value;  
// 其他还有阴影分辨率、附加光源的数量等  
CODE END

# **根据设备切换**

有时也希望根据设备推荐预设或自动进行设置。  
这种情况下，SystemInfo 类型很方便。

尤其是查看 deviceModel、graphicsDeviceName、systemMemorySize，可以获取便于识别的值。  
以下是返回值的示例。  
表: 各设备 SystemInfo 返回值的示例{{table:device_system_info}}

| 机型 | deviceModel | graphicsDeviceName | systemMemorySize |
| :---- | :---- | :---- | :---- |
| iPhone 13 | iPhone14,5 | Apple A15 GPU | 约 3800 |
| iPad Pro 11-inch（第 3 代） | iPad13,4 | Apple M1 GPU | 约 7800 |
| Galaxy S23 | samsung SM-S911B | Adreno (TM) 740 | 约 7500 |
| Pixel 8 | Google Pixel 8 | Mali-G715 | 约 7400 |

iOS 的 deviceModel 和 graphicsDeviceName 各自的命名规则都很统一，而且基本上在同一系列内，A 或 M 后面的数字越大，性能越高。  
因此，将两者结合起来就能在一定程度上进行区分。

Android 各机型返回的值参差不齐，难以像 iOS 那样一概而论地进行比较。  
因此，  
\* 直接按 RAM 划分  
\* 交由主数据（master data）进行比较  
\* 启动时运行基准测试工具

等方式来区分比较好。

# **自定义 RendererFeature**

在创建自定义 RendererFeature 等时，也最好预先准备可切换渲染设置的 API。  
例如，

*  降低临时纹理的格式、降低分辨率  
*  减少 Blit 的次数  
*  替换为简化的着色器

等应对措施。

[image1]: assets/sakuma-naoki-1/media/image1.png