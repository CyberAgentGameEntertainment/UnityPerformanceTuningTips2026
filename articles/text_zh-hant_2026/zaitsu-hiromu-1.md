# **營運階段的資產膨脹與使用 Project Auditor 進行調查**

姓名 / 羅馬字：Zaitsu Hiromu  
所屬：株式會社 Sumzap  
X 帳號 / 網站 / GitHub 帳號（選填）：@RomGameclient

在持續營運的專案中，安裝包大小與記憶體用量會逐漸增加。本章討論的是：即使團隊已知道對策，膨脹為何仍會再次發生、如何阻止它，以及找出持續增加的資產的方法。個別資產的輕量化本身不在本章範圍內。

# **營運階段膨脹持續擴大的理由**

膨脹是在日常營運中一點一滴累積的。為季節活動新增的高解析度橫幅、期間限定角色的語音與特寫演出、聯動企劃中接收的外部製作素材、宣傳結束後仍殘留的活動用資產。單獨來看每一項都在可接受範圍內，但隨著版本不斷釋出，便累積成無法忽視的量。而且累積得越多，重新檢視的成本就越高，整理的優先順序也隨之下降。  
原因與其說在技術，不如說在於營運規範的鬆動。在營運階段，下列 4 種情況會反覆發生。  
・規範過時：裝置或需求改變，規範與實際狀況已有落差，卻沒有人重寫規範。以「僅此一次」為由放行的例外不斷累積。  
・檢查變得寬鬆：臨近截止日時省略了交付素材的確認，負責人更換時也交接得不完整。  
・臨時處置一直殘留：為了趕時程放入未壓縮紋理或個別輸出的 Sprite 之後，事後還原的作業一直沒有進行。  
・判斷基準未留下記錄：設定的依據沒有留在文件中，負責人更換後便無法重現相同的基準。  
這些都不是只靠導入工具就能解決的。

# **讓規範持續運作**

決定想達成的指標、選定保證運作的裝置、在負載最高的場景中驗證。這種規範的制定方法本身，前作《Unity 效能調優寶典》（日文）的第 1 章已有說明。在營運階段成為問題的，是決定之後的事。  
規範一旦定下，就會開始與實際狀況產生落差。因此，要事先把保證運作裝置的變更、Unity 或渲染管線的更新、測量方法的變更等時間點，定為重新檢視的契機。尤其是測量方法改變時，同一個資產的數值也會變動，所以測量方法也要與規範一起保存。  
規範中的數值，要做成在匯出、匯入或自動建置的任一環節會執行檢查的形式。要阻擋還是放行，依項目逐一決定；即使放行，也請把實測值以建置記錄或報告的形式，留在全體團隊成員都看得到的地方。只在本人畫面上顯示警告的形式會被略過不看。  
修正超出規範的資產，並非改好數值就結束。若要更改壓縮設定，需確認外觀是否仍在可接受範圍內；若要重組 Bundle，需確認參照是否脫落，而且必須在與正式環境相同的條件下確認。正如前作所警告的，量產後的重做是一道包含發包、品質檢查到除錯的流程。理想做法不是集中一次修正，而是每次釋出版本時都進行測量，隨時修正增加的部分。

# **膨脹的模式分類**

若依素材類型整理對策，雖能知道哪種素材較重，卻無法知道為何變重。只要原因還在，相同的膨脹就會再次發生。因此，這裡跨越素材類型，依原因分為 6 類。  
表: 膨脹的 6 種模式 {{table:bloat_taxonomy}}

| 模式 | 發生了什麼 |
| :---- | :---- |
| 壓縮設定錯誤 | 匯入設定維持未壓縮或預設值未處理，佔用了所需數倍的容量 |
| 解析度過高 | 未指定最大解析度就匯入，容量隨面積成比例增加 |
| 多餘的附帶資料 | 用不到的輔助資料保持啟用狀態一併打包，並且也保留在記憶體中 |
| 格式選擇錯誤 | 選擇了不符合用途或平台的格式，不是多佔用容量，就是在執行階段被解壓而消耗記憶體 |
| 打包方式不一致 | Sprite Atlas 或 Bundle 的打包方式與實際使用單位不符，例如只為顯示 1 張 Sprite 就載入整個 Atlas，把用不到的內容也一起載入 |
| Bundle 間的重複 | 未登錄共用資產就直接參照，導致它依引用方 Bundle 的數量被重複打包 |

前 4 種可在資產 Import 時判定，但後 2 種取決於與其他資產的關係，在該時間點無法判定。這個差異，正是下一節把對策分為兩個階段的理由。

# **Import 時自動套用與建置前檢查**

要阻止再次發生，需在資產 Import 時套用正確的設定，再於建置前找出漏網之魚。  
自動套用的手段有 2 種。若要依條件改變數值，使用可在 Import 前後插入處理的 AssetPostprocessor。若只是固定各型別的初始值，用 Preset 就足夠。  
不過，只靠 Import 時的套用仍會出現漏網之魚。連同 .meta 檔案一起帶入的資產會保留原有設定進入專案，而且前一節的後 2 種在這個階段無法判定。  
因此，在建置前或自動建置的階段，檢查是否混入違反規範的資產。只要把紋理最大解析度、網格多邊形數等限制規範寫成測試，編輯器與自動建置就能使用相同的基準。像 AssetRegulationManager 這類用於此用途的 OSS 也已公開{{fn:asset_regulation_manager}}。  
不把這類檢查做成機制，只把規範寫成文件並宣導的營運方式，會直接導致開頭提到的規範過時，以及判斷基準未留下記錄的狀態。制定規則是人的工作，但讓規則被遵守的部分，請交給機器處理。

# **Bundle 間重複發生的機制**

6 種模式中，最難察覺的是 Bundle 間的重複。因為原因不在個別資產的設定，而在資產之間的參照關係。Addressables 以群組為單位管理資產，建置時將各群組打包成 Bundle。此時，未登錄為 Addressable 的資產會被視為隱含相依，複製到每一個參照它的 Bundle 中。  
![][image1]  
図: 未登錄的共用資產被複製（上）與登錄到專用群組的情況（下） {{fig:dup_mechanism}}  
在圖的例子中，只是 2 個 Prefab 參照同一張 4MB 的紋理，Bundle 總大小就變成 10MB。若把這張紋理登錄到專用群組，使其成為明確的相依，重複便會消除，總大小降到 6MB。引用方越多，複製的數量就越多，同時載入時也會在記憶體中展開同樣的份數，因此放置越久影響越大。另外，實際的 Bundle 會經過壓縮，這些數值只是用來說明機制的參考值。  
這類重複無法靠目視追蹤完整，需以工具偵測。若使用 Addressables，可以使用 Addressables Analyze{{fn:addressables_doc}}（在 2.0 系列中曾一度移除，於 2.3.1 恢復）。它也提供自動消除偵測到的重複的功能，但由於群組配置會被改寫，套用後請確認到建置結果為止。

# **調查已膨脹的資產**

規範與檢查可以阻止新的膨脹，但已經累積的資產需要另外找出。資產的效能分析有多種用途不同的工具。  
表: 用於調查容量與記憶體的工具 {{table:tools}}

| 工具 | 可得知的內容 |
| :---- | :---- |
| Build Report / Editor Log | 建置大小依類別的明細 |
| Addressables Analyze | 偵測與修正 Addressables 的 Bundle 相依重複 |
| Project Auditor | 整個專案的匯入設定與靜態配置問題 |
| Memory Profiler | 執行階段的記憶體用量與物件的參照關係 |

其中 Memory Profiler 在前作第 3 章「效能分析工具」已有說明。本章介紹前作未提及的 Project Auditor。

# **用 Project Auditor 全面清查**

Project Auditor 是對專案內的資產、程式碼、設定進行靜態分析，並將有問題的地方以清單形式輸出的工具。即使在尚未來得及落實為規範的階段，也能掌握哪些地方可能成為問題。它長期是實驗階段的套件，但在 2025 年 2 月釋出 1.0，成為正式版。以下的畫面與項目是在 Unity 6.4 與 Rules 套件 1.0.3 上確認的。

## **導入方法依 Unity 版本而異**

在 Unity 6.4 之前，從 Package Manager 新增 com.unity.project-auditor{{fn:pa_package_doc}}。6.4 之後因已內建於編輯器而不需新增{{fn:pa_manual}}，但判定用的規則已分離到 com.unity.project-auditor-rules，不導入該套件就無法分析（{{fig:pa_rules_package}}）。  
![][image2]  
図: Rules 套件。分析本體以編輯器模組的形式存在 {{fig:pa_rules_package}}  
由於規則已從本體分離，不必升級編輯器版本也能只更新規則。

## **Assets 檢視的使用方式**

開啟 Window \> Analysis \> Project Auditor 並執行分析後，結果會分為 Summary、Code、Assets、Shaders、Project、Build 等檢視（檢視可能依 Unity 版本而增加）。調查容量與記憶體時使用的是 Assets 檢視，其中分為 Asset Issues、Textures、Sprite Atlases 等 9 項。  
首先開啟的是 Asset Issues（{{fig:pa_asset_issues}}）。偵測到的問題依類型彙整，並顯示各類型找到了幾項。  
![][image3]  
図: Asset Issues 檢視。在為驗證準備的專案中的偵測範例 {{fig:pa_asset_issues}}  
展開類型後會顯示個別資產，選取後右側會顯示詳細資訊與建議的處置。掌握整體狀況後，再到 Textures 等個別檢視查看其明細（{{fig:pa_textures}}）。  
![][image4]  
図: Textures 檢視。設定與結果的大小並排顯示 {{fig:pa_textures}}  
這個檢視的優點是設定與結果位於同一列。同樣是 2048×2048 的紋理，Compressed 的列佔 2.7MB，Uncompressed 的列佔 16.0MB。不必開啟 Inspector，就能看出僅 1 處設定的差異會造成近 6 倍的差距。將 Size 欄降冪排序，從上方依序確認 Compression、Resolution、Format，就能找出壓縮設定錯誤、解析度過高與格式選擇錯誤。

## **減少偵測結果**

在營運中的專案執行分析，偵測結果可能超過數千項。在這種狀態下想從頭逐一處理是行不通的。請用前述{{fig:pa_asset_issues}}中的 Filters 縮小顯示範圍。勾選 Only Major/Critical 便只保留嚴重程度高的項目，在 Areas 選擇要查看的領域，就能只顯示與安裝包大小相關的問題。  
偵測到的問題會附上嚴重程度。文件中說明為 Major、Moderate、Minor、Ignored 4 種，但定義上也存在 Critical。Ignored 並非嚴重程度的等級，而是指被指定為忽略的狀態。處置從 Major 依序進行，若偵測到 Critical 則以它為最優先。靜態分析也會指出刻意的設定，因此逐項確認後再修正較為安全。  
對判斷為誤報的項目加上 Ignore。Ignore 是記錄在目前開啟的報告中的狀態，不會寫出到設定檔，重新分析後就會失效。偵測閾值這類判定參數會儲存在 ProjectSettings/ProjectAuditorSettings.asset，提交後即可讓全員一致。若想長期抑制某些問題的檢出，則在自動建置端的腳本中，將停用規則傳給 AnalysisParams.WithAdditionalDiagnosticRules。

## **整合到自動建置**

Project Auditor 也可以從命令列執行{{fn:pa_cli}}。整合到自動建置並保留每個版本的偵測數量，就能追蹤是在哪個版本增加的。  
Preferences 中有建置後將偵測數量輸出到記錄的設定，以及將偵測視為錯誤的設定。在自行撰寫失敗判定之前，請先確認這些是否足夠。另一方面，由於未提供比較 2 份已儲存報告的 API，要取得與上次的差異需自行實作。  
另外，也有建置後的自動分析無法完成而停住、分析中耗盡記憶體等回報{{fn:pa_github_issues}}。請設定逾時，避免分析失敗讓整個建置停擺。

# **結語**

膨脹對策這個主題，比起讓個別資產變輕的技術，更重視建立團隊層級阻止再次發生的機制。在營運中規範逐漸鬆動這件事本身，任何專案都會發生。正因如此，我認為把人的時間用在制定與重新檢視基準上，讓規則被遵守的部分交給機器，終究是最能長久持續的形式。  
Project Auditor 雖是剛成為正式版的工具，但作為尚未來得及制定規範的專案掌握現狀的入口，已足夠實用。請先執行一次分析，從觀察自己專案的傾向開始。

脚注: {{fn:asset_regulation_manager}} https://github.com/CyberAgentGameEntertainment/AssetRegulationManager/blob/main/README\_JA.md  
脚注: {{fn:pa_package_doc}} https://docs.unity3d.com/Packages/com.unity.project-auditor@1.0/manual/index.html  
脚注: {{fn:pa_manual}} https://docs.unity3d.com/6000.4/Documentation/Manual/project-auditor/project-auditor-introduction.html  
脚注: {{fn:pa_cli}} https://docs.unity3d.com/6000.4/Documentation/Manual/project-auditor/run-from-command-line.html  
脚注: {{fn:pa_github_issues}} https://github.com/Unity-Technologies/ProjectAuditor/issues  
脚注: {{fn:addressables_doc}} https://docs.unity3d.com/Packages/com.unity.addressables@latest

[image1]: assets/zaitsu-hiromu-1/media/image1.png

[image2]: assets/zaitsu-hiromu-1/media/image2.png

[image3]: assets/zaitsu-hiromu-1/media/image3.png

[image4]: assets/zaitsu-hiromu-1/media/image4.png