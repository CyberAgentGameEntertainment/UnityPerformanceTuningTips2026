# **依裝置切換繪製設定**

* NAOKI SAKUMA / 株式會社 CyberAgent SGE 核心技術本部

智慧型手機遊戲預期會在各式各樣的裝置上遊玩。  
若一律套用相同的繪製設定，在低規格裝置上有時會造成當機、掉影格、發熱等問題。  
反之，在高規格裝置上則有機會善用裝置效能，提供更好的品質。  
因此，本文將介紹依裝置切換為合適繪製設定的機制。

# **切換的方法**

遊戲中常見的做法有兩種：低、中、高之類的預設集形式，以及讓使用者直接操作數值的形式。

## **預設集形式**

這是以預設集為單位進行切換時所採用的方法。  
無論是讓使用者選擇預設集，還是由裝置自動判定，都採用此方法。

在 ProjectSettings 的 Quality 中按下 Add Quality Level，依預設集的數量新增。  
接著，為各預設集附加對應的 RenderPipelineAsset。  
必要時，也可以設定 Quality 的其他項目（Rendering、Textures 等）。  
![][image1]  
在程式碼中呼叫 QualitySettings.SetQualityLevel，即可切換到任意的 QualityLevel。

## **直接操作數值的形式**

若要讓使用者操作參數，則採用此方法。  
透過以下這類程式碼，可以直接操作參數。

CODE START csharp  
var urpAsset \= (UniversalRenderPipelineAsset)GraphicsSettings.currentRenderPipeline;  
// 變更解析度  
urpAsset.renderScale \= value;  
// 變更陰影級聯（Cascade）的數量  
urpAsset.shadowCascadeCount \= value;  
// 其他還有陰影解析度、額外光源的數量等  
CODE END

# **依裝置切換**

有時也會希望依裝置推薦或自動設定預設集。  
這種情況下，SystemInfo 型別相當方便。

特別是查看 deviceModel、graphicsDeviceName、systemMemorySize，可以取得容易識別的值。  
以下是回傳值的範例。  
表: 各裝置 SystemInfo 回傳值的範例{{table:device_system_info}}

| 機型 | deviceModel | graphicsDeviceName | systemMemorySize |
| :---- | :---- | :---- | :---- |
| iPhone 13 | iPhone14,5 | Apple A15 GPU | 約 3800 |
| iPad Pro 11-inch（第 3 代） | iPad13,4 | Apple M1 GPU | 約 7800 |
| Galaxy S23 | samsung SM-S911B | Adreno (TM) 740 | 約 7500 |
| Pixel 8 | Google Pixel 8 | Mali-G715 | 約 7400 |

iOS 的 deviceModel 與 graphicsDeviceName 命名規則各自具有一貫性，且基本上在同一系列內，A 或 M 之後的數字越大，規格越高。  
因此，將兩者組合起來即可在一定程度上加以區分。

Android 各機型回傳的值參差不齊，難以像 iOS 那樣一概而論地比較。  
因此，  
\* 以 RAM 大小粗略劃分  
\* 交由主資料（master data）進行比較  
\* 在啟動時執行基準測試工具

建議以上述等方式加以區分。

# **自製 RendererFeature**

自行製作 RendererFeature 等時，事先準備好可切換繪製設定的 API 也會很方便。  
例如，

*  降低暫存紋理的格式、降低解析度  
*  減少 Blit 的次數  
*  替換為簡易的著色器

等因應方式。

[image1]: assets/sakuma-naoki-1/media/image1.png