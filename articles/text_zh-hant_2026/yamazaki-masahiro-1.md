# **原稿**

# **在 Unity 中活用 iOS 內建的效能測量功能**

* 姓名: Yamazaki Masahiro  
* 所屬: 株式會社 Applibot  
* X: [https://x.com/TEST\_H\_](https://x.com/TEST_H_)  
* GitHub: [https://github.com/mao-test-h](https://github.com/mao-test-h)

# **前言**

Unity 內建了以 Unity Profiler 為首的測量功能，但若是 iOS 應用程式，還可以額外運用 OS 內建的效能測量功能。

Apple 提供的這些功能，其特點是不依賴 Unity 的 Development Build 旗標即可使用。  
例如，可在尚未整合測量處理的階段，或是以停用除錯功能、接近發佈建置的配置粗略確認負載；也可以只從測試人員的裝置接收追蹤結果。像這樣掌握大致的負載並在一定程度上縮小瓶頸候選之後，再切換到啟用 Development Build 的建置，以 Unity Profiler 或 Instruments 進行詳細調查，如此分工使用。

本文將從 iOS 內建提供的功能中介紹以下 2 項，並一併說明 macOS 27 / iOS 27 新增的新功能。

* Metal Performance HUD  
* Performance Trace

此外，筆者已公開「MetalPerfKit-Unity」{{fn:metalperfkit_unity}}，這是一個讓 Unity 能使用這些功能的 OSS 兼範例儲存庫，關於實作細節也請一併參考。

※ 以下說明中統一以 iOS 表記，但除非另有說明，亦包含 iPadOS。

## **驗證環境**

本文在以下環境進行了動作驗證。

* Unity 6000.0.81f1  
* Xcode 26.6 / 27.0 beta  
* iOS 26.6 / 27.0 beta  
* macOS 27.0 beta

※ 撰寫本文時，iOS 27、macOS 27、Xcode 27 以及後述的 \`StateReporter\` API 仍處於 Beta 階段。本文為 2026 年 8 月時的資訊，在正式發佈前，API、指令、設定畫面、行為等皆可能變更。

# **Metal Performance HUD**

Metal Performance HUD 是一款工具，可將使用 Metal 的應用程式效能資訊即時疊加顯示在畫面上，或輸出為日誌。在 Apple 的文件 {{fn:hud_monitoring_doc}} 中，它被介紹為用於快速掌握目前狀態、找出要以 Xcode 或 Instruments 詳細擷取之範圍的功能。

此功能本身自 Metal 4 之前（iOS 16 世代）就已存在，但配合 2025 年 WWDC 發表的 Metal 4（iOS 26 世代）進行了大幅強化。Metal 4 之前與 Metal 4 之後的顯示差異如 {{fig:perf_hud_legacy}} 與 {{fig:perf_hud_metal4}} 所示。

![][image1]{scale=0.5}

図: Metal 4 之前的 Metal Performance HUD {{fig:perf_hud_legacy}}

![][image2]{scale=0.5}

図: Metal 4 之後的 Metal Performance HUD {{fig:perf_hud_metal4}}

Metal 4 起的主要強化點，除了擴充測量項目之外，還包括顯示位置、不透明度與縮放比例的自訂，以及可透過 API 動態控制的項目增加等。

顯示主要指標的狀態範例如 {{fig:perf_hud_metrics_all}} 所示。

![][image3]{scale=1.0}

図: 顯示主要指標的狀態範例 {{fig:perf_hud_metrics_all}}

關於各指標的詳細說明，請參閱官方文件「Understanding the Metal Performance HUD metrics」{{fn:hud_metrics_doc}}。

此外，筆者過去也公開了關於 Metal Performance HUD 的解說文章 {{fn:qiita_gpu_measurement}} 以及技術同人誌「UniTips Vol.12」{{fn:unitips_vol12}}。其中說明了從基本用法到針對 Unity 的活用技巧等內容，詳情也請參考。

以下各節將以 Metal 4（iOS 26 世代）的內容為基礎進行說明。

## **如何開啟 HUD**

顯示 HUD 的方法有數種，此處說明「從設定 App 開啟」與「以 API 控制顯示」這 2 種方法。

### **從設定 App 開啟**

可從 iOS 裝置的設定 App 啟用。步驟如下。

※ 前提是必須已顯示 \`Developer\` 選單。此外，依 OS 版本不同，選單名稱可能有所差異。

1. 開啟 iOS 的「設定」App  
2. 開啟 \`Developer\` 選單，再開啟其中的 \`Metal HUD\`（參照 {{fig:settings_developer_menu}}）

![][image4]{scale=0.4}

図: 從 Developer 選單開啟 Metal HUD {{fig:settings_developer_menu}}

開啟 \`Metal HUD\` 後會顯示 {{fig:settings_metal_hud}} 畫面，啟用其中的 \`Show Metal Performance HUD\` 並重新啟動目標 App 即可顯示。

在此畫面中，也可以切換顯示的指標、變更不透明度、縮放比例與顯示位置。此外，啟用同一區段中的 \`Log Metal Performance\` 後，可以固定間隔將效能資訊輸出為日誌。詳情將在後述的「關於日誌功能」中說明。

![][image5]{scale=0.4}

図: Metal HUD 的設定畫面 {{fig:settings_metal_hud}}

### **以 API 控制顯示**

若想在 App 執行期間動態控制 HUD 的顯示，可將設定值傳給 \`CAMetalLayer\` 類別的 \`developerHUDProperties\` 屬性。{{list:hud_properties_basic}} 中啟用了 HUD 與日誌輸出，並將顯示位置指定為左上。

CODE START swift | 以 developerHUDProperties 控制 HUD 的顯示 {{list:hud_properties_basic}}

myMetalLayer.developerHUDProperties \= \[  
   "mode": "default",      // HUD 的 ON/OFF  
   "logging": "default",   // 日誌功能的 ON/OFF  
   "positionX": 0,         // 顯示位置（以像素為單位的 X 座標）  
   "positionY": 0,         // 顯示位置（以像素為單位的 Y 座標）  
\]

CODE END

若要隱藏 HUD，則在 \`mode\` 指定 \`disabled\`。以往此屬性只能控制 HUD 與日誌功能的 ON/OFF，無法調整位置等，但自 Metal 4 起，可透過 \`positionX\` / \`positionY\` 以像素為單位指定位置。

此外，這個 Dictionary 也可以傳入 HUD 支援的環境變數鍵。例如傳入 \`MTL\_HUD\_OPACITY\` 與 \`MTL\_HUD\_SCALE\`，即可在 0.0〜1.0 的範圍內調整不透明度與縮放比例（{{list:hud_properties_env}}）。

CODE START swift | 以環境變數鍵調整不透明度與縮放比例 {{list:hud_properties_env}}

myMetalLayer.developerHUDProperties \= \[  
   ...,  
   "MTL\_HUD\_OPACITY": opacity,     // 不透明度（0.0〜1.0）  
   "MTL\_HUD\_SCALE": scale,         // 縮放比例（0.0〜1.0）  
\]

CODE END

其他還有許多可設定的項目，詳情請參閱官方文件「Customizing the Metal Performance HUD」{{fn:hud_customizing_doc}}。

從 Unity 呼叫時，需以 iOS 原生外掛程式取得用於繪製的 \`CAMetalLayer\`，再從 C\# 端透過 P/Invoke 呼叫。\`CAMetalLayer\` 可經由 \`UnityFramework\` 如 {{list:get_metal_layer}} 所示取得。

CODE START swift | 經由 UnityFramework 取得 CAMetalLayer {{list:get_metal_layer}}

private func getMetalLayer() \-\> CAMetalLayer? {  
   guard let instance \= UnityFramework.getInstance(),  
         let rootView \= instance.appController().rootView,  
         let metalLayer \= rootView.layer as? CAMetalLayer else {  
       return nil  
   }  
   return metalLayer  
}

CODE END

MetalPerfKit-Unity 將這些實作公開為 \`PerformanceHUDSwitcher\` 類別，可從 C\# 如 {{list:hud_switcher_csharp}} 所示呼叫。另外，相對於原生 API 以像素為單位指定位置，\`PerformanceHUDSwitcher\` 為了不依賴解析度，以 0.0〜1.0 的相對位置接收座標，並在內部依繪製區域的大小轉換為像素座標。

CODE START csharp | PerformanceHUDSwitcher 的呼叫範例 {{list:hud_switcher_csharp}}

// 將 HUD 顯示在畫面右上（x=1.0, y=0.0）  
// （座標以 0.0〜1.0 的相對位置指定）  
PerformanceHUDSwitcher.SetPerformanceHUDVisible(true, 1.0f, 0.0f);

CODE END

完整的實作範例請參閱 MetalPerfKit-Unity 的原始碼。

## **關於日誌功能**

啟用前述的 \`Log Metal Performance\` 或 API 的 \`logging\` 後，App 執行期間會每隔 1 秒將效能資訊輸出為日誌。日誌以 \`metal-HUD:\` 開頭、{{list:hud_log_format}} 的格式輸出，可從 Xcode 的主控台或 macOS 的 Console.app 確認。

CODE START text | metal-HUD 日誌的輸出格式 {{list:hud_log_format}}

metal-HUD: \<first-frame-number-integer\>,\<graphics-memory-usage-float\>,\<process-memory-usage-float\>,\<first-frame-present-interval-float\>,\<first-frame-gpu-time-float\>,...\<last-frame-present-interval-float\>,\<last-frame-gpu-time-float\>

CODE END

從開頭依序記錄「開始取樣的影格編號」「圖形記憶體使用量」「整個處理程序的記憶體使用量」，之後則重複記錄每個取樣影格的 Frame Present Interval 與 GPU Time 配對。實際的輸出範例如 {{fig:perf_hud_log_sample}} 所示。

![][image6]{scale=1.0}

図: Metal Performance HUD 的日誌輸出範例 {{fig:perf_hud_log_sample}}

若以逗號分隔的資料進行彙整，即可事後分析影格間隔的紊亂、GPU Time 以及記憶體使用量的變化。此外，只要這些日誌仍保留在 Unified Logging 中，也可以利用 \`OSLogStore\` API 實作「取得過去 N 秒的日誌並儲存為檔案」等功能。MetalPerfKit-Unity 以 \`PerformanceLogger\` 類別提供此功能（{{list:fetch_performance_logs}}）。

CODE START csharp | 以 PerformanceLogger 取得並儲存日誌 {{list:fetch_performance_logs}}

// 取得過去 60 秒的日誌並儲存為檔案  
PerformanceLogger.FetchPerformanceLogs(60, FileUtility.GenerateFetchLoggingFilePath());

CODE END

# **Performance Trace**

Performance Trace 是 iOS 26 新增的功能，無需連接 Mac，即可僅以 iOS 裝置收集效能相關資料。收集到的追蹤檔案可傳送至 Mac，再以 Instruments 或後述的 \`metalperftrace\` 指令進行分析。

由於可在裝置上獨立完成，除了能記錄長時間遊玩等在連接 Mac 的狀態下難以重現的情況，也可以請測試人員等其他人分享在其裝置上收集的追蹤檔案，在自己手邊進行分析。

本章將以其中在 WWDC 2026 隨功能強化一併說明的 Lookback Collection 為主進行介紹。

## **使用方法**

Performance Trace 必須從 \`Developer\` 選單啟用。步驟如下。

1. 開啟 iOS 的「設定」App  
2. 開啟 \`Developer\` 選單，再開啟其中的 \`Performance Trace\`  
3. 啟用 \`Enable Performance Trace\`（切換啟用/停用需要重新啟動裝置）  
4. 選擇任意的 \`Tracing Mode\`（詳情後述）

\`Developer\` 選單內的 \`Performance Trace\` 及其設定畫面分別如 {{fig:trace_developer_menu}} 與 {{fig:trace_enable}} 所示。

![][image7]{scale=0.4}

図: Developer 選單內的 Performance Trace {{fig:trace_developer_menu}}

![][image8]{scale=0.4}

図: Enable Performance Trace 的設定畫面 {{fig:trace_enable}}

完成啟用後，接著為了進行追蹤的開始與停止、追蹤資料的收集等操作，需將 \`Performance Trace\` 加入控制中心。這可以開啟控制中心，從 \`Developer\` 區段加入。加入步驟與加入後的狀態如 {{fig:trace_add_control}} 與 {{fig:trace_control}} 所示。

![][image9]{scale=0.4}

図: 將 Performance Trace 加入控制中心 {{fig:trace_add_control}}

![][image10]{scale=0.4}

図: 已加入 Performance Trace 控制項的狀態 {{fig:trace_control}}

## **關於 Tracing Mode**

\`Tracing Mode\` 有 {{table:tracing_modes}} 所示的 4 種（System Activity 分為 Summary / Detailed 2 種），也可以透過長按控制中心的 \`Performance Trace\` 控制項來切換。

表: Tracing Mode 的種類 {{table:tracing_modes}}

| 模式 | 概要 |
| :---- | :---- |
| Power Profiler | 記錄對電力消耗的影響 |
| System Activity (Summary / Detailed) | 記錄系統的運作狀況 |
| Lookback Collection | 回溯收集系統持續記錄的資料 |

Power Profiler 與 System Activity 需點按控制中心的 \`Performance Trace\` 控制項開始記錄，再次點按則停止。另一方面，Lookback Collection 不需要記錄操作，會從點按的時間點回溯收集資料。詳情後述。

收集到的追蹤檔案會列在 Performance Trace 畫面的 \`Available Trace Files\` 中，可透過分享按鈕以 AirDrop 等方式傳送至 Mac 或其他開發者。解開分享的封存檔後，即可取得能以 Instruments 開啟的 \`.atrc\` 檔案。另外，追蹤檔案在裝置儲存空間不足時會自動刪除，也可以透過滑動操作手動刪除。

各模式的補充說明如下。

### **Power Profiler**

Power Profiler 是測量 App 對 CPU / GPU / 顯示器 / 網路等各子系統的使用，對裝置電力消耗造成之影響（Power Impact）的模式。本文省略其詳細說明，官方文件「Measuring your app’s power use with Power Profiler」{{fn:power_profiler_doc}} 與 WWDC 2025 的場次「Profile and optimize power usage in your app」{{fn:wwdc2025_power}} 中有詳細解說。

### **System Activity**

System Activity 是提供「Summary」與「Detailed」2 種的追蹤模式。  
就筆者確認的範圍，截至撰寫本文時（2026 年 8 月），Apple 的文件或 WWDC 場次中未見 System Activity 的詳細規範，以及 Summary 與 Detailed 差異的相關說明。因此，本文不深入各模式的細節，僅介紹在實際追蹤中確認到的資訊。

作為參考，在搭載 iOS 27 的裝置上記錄 System Activity (Summary) 的追蹤，並以後述的 \`metalperftrace\` 指令分析的結果如 {{list:overview_system_activity}} 所示。

CODE START text | System Activity (Summary) 追蹤的分析結果 {{list:overview_system_activity}}

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

在此追蹤中，除了各處理程序的記憶體使用量、CPU 時間、磁碟 I/O 之外，還能確認以 \`CAMetalLayer\` 為單位的 FPS、Frame Time、CPU Begin-to-Present、On-GPU Time 等統計資料。另外，\`\[States\]\` 項目中記錄了 App 的狀態轉換，這是由下一章說明的 \`StateReporter\` API 所產生，詳情後述。

### **Lookback Collection**

Lookback Collection 是事後回溯收集系統持續記錄之效能資料的模式。  
就筆者確認所及，Lookback Collection 在 iOS 26 時就已存在於 \`Tracing Mode\` 的選項中，但 WWDC 2025 與官方文件並未說明。之後在 WWDC 2026 進行了功能強化，並公開了解說其內容的場次影片「Find and fix performance issues in your Metal games」{{fn:wwdc2026_metal_games}}。

根據該演講，遊戲執行期間，系統會持續記錄效能指標（CPU、GPU、FPS、記憶體等），並將該資料保存數天。Lookback Collection 利用了這個機制，因此其特點是即使是測試人員的長時間遊玩工作階段，也能回溯取得資料。此機制的概要如 {{fig:wwdc_lookback_metrics}} 所示。

![][image11]{scale=1.0}

図: 系統持續記錄效能指標（取自 WWDC 2026 場次） {{fig:wwdc_lookback_metrics}}

使用方法只需在照常進行遊玩測試後，點按控制中心的 \`Performance Trace\` 控制項。從點按的時間點回溯設定的時間長度，開始收集與處理追蹤，完成後會收到通知並加入 \`Available Trace Files\`。

實際在搭載 iOS 26 的裝置上收集約 3 小時的 Lookback Collection 追蹤，並以 Instruments 開啟的範例如 {{fig:lookback_process_list}} 所示。回溯期間內啟動過的多個 App（處理程序）會列表顯示，可綜覽各自的 FPS 等變化。

![][image12]{scale=1.0}

図: 以 Instruments 開啟 Lookback Collection 追蹤的範例 {{fig:lookback_process_list}}

進一步展開處理程序後，可如 {{fig:lookback_frame_metrics}} 所示，以 \`CAMetalLayer\` 為單位確認 FPS、GPU Usage %、Frame Interval、CPU / GPU 時間等影格指標的變化。

![][image13]{scale=1.0}

図: 以 CAMetalLayer 為單位的影格指標變化 {{fig:lookback_frame_metrics}}

※ 在手邊的 iOS 27 環境中，彙整過程中顯示 \`Error processing Performance Trace\`，無法正常運作。關於此問題，即使在 iOS / iPadOS 27 正式發佈時，發行說明的 \`Control Center -> Known Issues\` 中仍記載著「收集 Power Profiler 或 Lookback 的追蹤時，可能不會產生追蹤」的已知問題（184729556）。{{fn:ios_ipados_27_release_notes}}

# **macOS 27 / iOS 27 新增的新功能**

如前章所介紹，遊戲執行期間系統會持續記錄並保存 Metal 效能指標。WWDC 2026 發表了以此機制為核心的新功能：為追蹤附加 App 專屬資訊的 \`StateReporter\` API，以及可從 CLI 分析追蹤的 \`metalperftrace\` 指令。

## **StateReporter API**

即使能從追蹤資料找出影格率下降的時刻，僅憑 Metal 效能指標也無法得知當時遊戲正在做什麼。這是因為「遊戲正開啟哪個場景」「物件數量有多少」「圖形設定為何」等 App 專屬資訊，不會自動包含在 Metal 效能指標中。

使用 iOS 27 新增的 \`StateReporting\` 框架 {{fn:statereporting_doc}} 的 \`StateReporter\` API {{fn:statereporter_doc}}，可將 App 的狀態回報給系統，為追蹤資料附加 App 專屬的情境資訊。

回報的狀態在 \`metalperftrace\` 中用於依狀態彙整，在 Instruments 中則用於顯示 Points of Interest。以 Instruments 開啟前章刊載之 System Activity 追蹤的範例如 {{fig:instruments_state_reporter}} 所示。回報的狀態顯示為 Points of Interest 的軌道。

![][image14]{scale=1.0}

図: 回報的狀態顯示為 Points of Interest 的範例 {{fig:instruments_state_reporter}}

\`StateReporter\` 由 {{table:state_reporter_elements}} 所示的 4 個元素構成。

表: 構成 StateReporter 的 4 個元素 {{table:state_reporter_elements}}

| 元素 | 概要 | 範例 |
| :---- | :---- | :---- |
| Domain | 作為回報範圍的功能領域。為反向 DNS 格式的字串，每個 Domain 同時處於作用中的狀態只有 1 個 | \`com.mygame.level\` |
| Label | 表示目前狀態的簡短字串 | \`Level 1\` |
| Stable metadata | 在狀態存續期間不變的中繼資料。與 Label 組合以唯一識別狀態 | \`{id: 1001}\` |
| Volatile metadata | 在狀態中可能變化的揮發性中繼資料。更新時不會發生狀態轉換 | \`{health: 20}\` |

Domain 可定義多個，例如可將「關卡進度」「圖形設定」「網路狀態」分別作為獨立的 Domain 回報，並組合起來分析。

從 Swift 使用時如 {{list:state_reporter_swift}} 所示。

CODE START swift | 從 Swift 使用 StateReporter 的範例 {{list:state_reporter_swift}}

import StateReporting  
// 取得對應 Domain 的 Reporter  
let reporter \= StateReporter.reporter(for: "com.mygame.gameplay")  
// 回報狀態轉換  
reporter.reportTransition(to: "Playing")  
// 若要結束狀態則傳入 nil  
reporter.reportTransition(to: nil)

CODE END

### **從 Unity 使用**

由於 \`StateReporting\` 是針對 Swift / Objective-C 的 API，要從 Unity 使用需要原生外掛程式。MetalPerfKit-Unity 將其包裝為 \`StateReporter\` 類別，作為預覽功能公開，可從 C\# 如 {{list:state_reporter_csharp}} 所示使用。

CODE START csharp | 從 C\# 使用 StateReporter 的範例 {{list:state_reporter_csharp}}

using MetalPerfKit;  
// 取得對應 Domain 的 Reporter  
var reporter \= StateReporter.ReporterForDomain($"{Application.identifier}.screen");  
// 回報狀態轉換（附帶揮發性中繼資料）  
reporter.ReportTransitionToStateLabel(  
   "Main",  
   volatileMetadata: new Dictionary\<string, StateReporterMetadataValue\>  
   {  
       \["CubeCount"\] \= cubeCount,  
   });  
// 維持狀態不變，僅更新揮發性中繼資料  
reporter.ReportVolatileMetadataUpdate(  
   new Dictionary\<string, StateReporterMetadataValue\>  
   {  
       \["CubeCount"\] \= cubeCount,  
   });

CODE END

此外，儲存庫的 \`Assets/\_Example/2\_MetalPerformanceMetrics\` 中準備了範例，將畫面切換（Title / Main / Settings）與圖形品質（URP 的 3 個等級）分別作為獨立的 Domain 回報。前章刊載的 System Activity 追蹤中的 \`\[States\]\`，就是以此範例記錄的。

## **metalperftrace**

\`metalperftrace\` 是 macOS 27 新增的 CLI 工具，除了分析追蹤檔案之外，還能收集 macOS 應用程式的效能指標。收集功能僅限於在 macOS 上執行的 App，因此本文不予討論，但在分析 iOS 裝置上收集的追蹤檔案時可活用此工具。

### **關於追蹤檔案的分析**

分析追蹤檔案時使用 \`overview\` 子指令（{{list:metalperftrace_overview}}）。

CODE START bash | 以 overview 子指令分析追蹤檔案 {{list:metalperftrace_overview}}

\# 分析已收集的追蹤檔案  
metalperftrace overview MyGameTrace.atrc

CODE END

輸出內容如前章所介紹。若再傳入 \`--include-state-transitions\`，還能確認以 \`StateReporter\` 記錄的狀態轉換詳情。{{list:overview_state_transitions}} 是分析前述範例 App 追蹤的例子，輸出了各圖形品質的變更歷程，以及在該狀態下的停留時間與比例。

CODE START text | 包含狀態轉換詳情的分析結果 {{list:overview_state_transitions}}

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

※ 輸出內容經部分節錄與整理後刊載。實際上相同的狀態也會沿時間序列重複輸出，並各自附有開始與結束的時間戳記。

此外，傳入 \`--aggregate\` 可依狀態彙整指標。指定 \`--domain com.mygame.graphics\` 可篩選特定的 Domain，再併用 \`--state-label "High"\` 則可進一步篩選到特定狀態。「只有圖形設定為 High 時平均 FPS 才下降」這類 App 狀態與效能的相關性，只需 1 個指令即可確認。

此外，這些輸出也可以透過傳入 \`--json\` 以 JSON 格式取得。演講中也說明「結構化的輸出可活用於以腳本進行的迴歸測試，或由 AI 代理自動分類效能問題」。Apple 的「Metal Developer Tools」頁面 {{fn:metal_developer_tools}} 中，也以「Hello, agentic debugging」為題，介紹了與代理結合的工作流程。

脚注: {{fn:metalperfkit_unity}} MetalPerfKit-Unity [https://github.com/mao-test-h/MetalPerfKit-Unity](https://github.com/mao-test-h/MetalPerfKit-Unity)

脚注: {{fn:hud_monitoring_doc}} Monitoring your Metal app’s graphics performance [https://developer.apple.com/documentation/xcode/monitoring-your-metal-apps-graphics-performance](https://developer.apple.com/documentation/xcode/monitoring-your-metal-apps-graphics-performance)

脚注: {{fn:hud_metrics_doc}} Understanding the Metal Performance HUD metrics [https://developer.apple.com/documentation/xcode/understanding-metal-performance-hud-metrics](https://developer.apple.com/documentation/xcode/understanding-metal-performance-hud-metrics)

脚注: {{fn:qiita_gpu_measurement}} 【Unity】在行動應用程式上測量 GPU 負載（日文） [https://qiita.com/mao\_/items/3b531d17cacb86036b7f](https://qiita.com/mao_/items/3b531d17cacb86036b7f)

脚注: {{fn:unitips_vol12}} UniTips Vol.12（日文，於「第 2 章：Metal 4 強化後的 Metal Performance HUD 之 Unity 活用技巧」中說明） [https://techbookfest.org/product/8mHpXz8LJCKAsLxENJKwaz?productVariantID=ewK5GtNTjkz4k8heM4BrxU](https://techbookfest.org/product/8mHpXz8LJCKAsLxENJKwaz?productVariantID=ewK5GtNTjkz4k8heM4BrxU)

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