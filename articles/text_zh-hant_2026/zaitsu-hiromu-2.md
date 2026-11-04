# **UI 系統的選用 — IMGUI / uGUI / UI Toolkit**

姓名 / 羅馬字：Zaitsu Hiromu  
所屬：株式會社 Sumzap  
X 帳號 / 網站 / GitHub 帳號（選填）：@RomGameclient

前作《Unity 效能調優寶典》（日文）{{fn:tuning_bible}}討論了 Canvas 的分割與 Raycast Target 的調整等針對 uGUI 負載的因應方式。本章則往上游退一步，探討 UI 系統本身的選擇。因為選擇哪一種，會決定 UI 的負載結構與可採取的因應方式種類。  
Unity 有 IMGUI、uGUI、UI Toolkit 三種 UI 系統。本章依序介紹這三者，整理各自目前在執行階段與編輯器擴充中的定位。本章內容以 Unity 6.5 時點的資訊為準。

# **IMGUI**

執行階段的 IMGUI 已不在遊戲內 UI 的選項之中。官方手冊明確指出，它並非用於玩家操作的一般遊戲內 UI，並將遊戲內的偵錯顯示與工具、自訂 Inspector、編輯器擴充列為預期用途。在偵錯覆蓋層與執行階段日誌檢視器等用途上，目前仍在使用。  
另一方面，在編輯器擴充中，IMGUI 在官方立場上仍是現役。官方的 UI 系統比較頁面{{fn:ui_system_compare}}將 IMGUI 列為替代方案，並指出兩項優點：可不受限制地存取編輯器的擴充功能，以及它是能快速將 UI 繪製到畫面上的輕量 API。  
官方的 IMGUI 遷移指南{{fn:imgui_migration}}明確列出了 UI Toolkit 中沒有對應項的 API。實務上影響較大的是 InspectorTitlebar()、Toolbar()、IntPopup()、LinkButton() 等。這些都是用來讓編輯器視窗的外觀貼近 Unity 本體的 API，在需要的場合目前仍得使用 IMGUI。  
不過，作為編輯器擴充使用時，有一項負載特性需要先了解。IMGUI 是在 OnGUI() 中每次都重新描述 UI 的即時模式系統，不保留 UI 樹的資訊，每次事件都反覆進行建構與銷毀。它與保留模式的 UI Toolkit 的差異如 {{fig:render_cycle}} 所示。OnGUI() 在 1 個影格內會執行多次，每次都容易發生堆積配置。在持續重繪的編輯器視窗中，這會成為 GC 的負載。  
![][image1]  
図: 即時模式與保留模式的繪製週期 {{fig:render_cycle}}

# **uGUI**

這是以 Canvas 與 GameObject 為基礎的系統。每一個元素都是 GameObject，並擁有 Transform 階層。僅限執行階段使用，無法用於編輯器擴充。  
在執行階段，它是事實上的標準。優點有三項。  
・可直接在 Scene 視圖中放置，並當場預覽  
・可用 Animator 與 Timeline 驅動 UI  
・Asset Store 上有大量以 uGUI 為前提的套件  
在負載方面，元素有變更的 Canvas 會重新建構網格，且 Transform 階層的更新與 Raycast Target 的掃描成本會隨元素數量成比例增加。前作討論的 Canvas 分割與 Raycast Target 調整，都是針對源自此結構之負載的代表性因應方式。

# **UI Toolkit**

這是將 VisualElement 的樹建構一次，之後僅更新變化部分的保留模式系統。邏輯以 C\#、UI 結構以 UXML、樣式以 USS 分開撰寫。執行階段與編輯器擴充皆可使用，在編輯器擴充中是官方的第一推薦。不過，官方的寫法是「若要製作複雜的編輯器工具」。若只是排列幾個欄位的工具，不引入 UXML 與 USS、僅以 C\# 組建也不成問題。  
在負載方面，與 uGUI 的差異直接就是因應方式的差異。不同於只要改變一個元素就會重建整個 Canvas 網格的 uGUI，UI Toolkit 僅更新變化的部分，因此不需要相當於 Canvas 分割的因應方式。不過，元素數量增加時，版面配置計算的成本同樣會增加，所以減少元素數量的設計考量仍然必要。在 Unity 6 系列中，事件分派的加速、幾何產生的 Job 化、版面配置計算的快取改善等效能改善也持續進行中。

## **與 uGUI 相比不足之處**

截至 Unity 6.5，無法做到的事項包括以下幾點。這些都是 Unity 列為開發中的項目{{fn:uitk_status}}，但若現在要判斷是否採用，必須以無法做到為前提來考量。  
・在 Animation 視窗中處理 VisualElement 關鍵影格動畫的功能  
・在 Scene 視圖中編輯 UI  
・從 MonoBehaviour 腳本參照 UXML 中定義之 VisualElement 的機制  
・針對自訂著色器的頂點資料支援  
生態系也仍然單薄。Asset Store 的資產多以 GameObject 階層為前提，無法直接運作。TextMeshPro 也無法從 UI Toolkit 使用，UI Toolkit 使用專用的文字系統（6.5 起成為預設的 Advanced Text Generator）。DOTween 雖已新增 VisualElement 用的捷徑，但僅支援 Unity 6.3 以上{{fn:dotween}}。  
從 uGUI 遷移時，錨點與樞軸沒有直接的對應項。UI Toolkit 的版面配置，是將子元素依序流入父元素中排列的方式。與針對每個元素指定位置與大小的 uGUI 相比，組建方式本身就不同。

# **官方推薦的變遷**

官方的 UI 系統比較頁面依手冊的各版本分別公開，對照閱讀可發現執行階段 UI 的推薦理由已有所更替{{fn:ui_system_compare_60}}。其明細如 {{table:recommendation_shift}} 所示。  
表: 執行階段 UI 推薦理由的變化 {{table:recommendation_shift}}

| 需求、用途 | Unity 2022.3～6.1 | Unity 6.5 |
| :---- | :---- | :---- |
| 處理大量 UI | UI Toolkit | UI Toolkit |
| 需要面向美術與設計師的製作工作流程 | UI Toolkit | UI Toolkit |
| 無紋理的 UI 繪製 | UI Toolkit | UI Toolkit |
| 放置於 3D 空間並受光照的 UI | uGUI | UI Toolkit |
| 以自訂著色器與材質呈現 | uGUI | UI Toolkit |
| 易於從 MonoBehaviour 參照 | uGUI | uGUI |

從 2022.3 到 6.1 一直位於 uGUI 側的兩項{{fn:ui_system_compare_2022}}，隨著世界空間 UI 在 6.2、自訂著色器在 6.3 針對 URP 實作完成，移到了 UI Toolkit 側。剩下的「易於從 MonoBehaviour 參照」，其參照 UXML 中定義之元素的機制也已列入開發中的項目。不過，比較頁面的結構本身在 Unity 6.5 仍未改變。執行階段 UI 仍以 uGUI 作為預設來介紹，並將 UI Toolkit 列為其替代方案。更替的只有推薦的理由，將何者置於預設並未變動。同一頁面上依用途分類的一覽，其標題也從 2022.3 至 6.2 的「Recommendation（推薦）」，在 6.3 以後改為「Often used for（常用於）」。Unity 自身已不再稱之為推薦。

# **併用 UI 系統時的限制**

在一個專案中使用多個 UI 系統本身是可行的，但組合使用時會出現一些問題。以下分為執行階段與編輯器擴充來看。

## **執行階段**

uGUI 與 UI Toolkit 可在同一專案中使用，但無法將一方嵌入另一方之中。導覽與樣式的不一致，官方也列為限制。  
繪製順序只能在各自的系統內控制（{{fig:sort_order}}）。要以 UI Toolkit 將 UI 顯示到畫面上，需在 GameObject 上附加 Panel Renderer 元件。此角色在 Unity 6.4 以前由 UIDocument 擔任，6.5 起被取代{{fn:ui_document_component}}。若使用 LTS 的 6.3，則為 UIDocument。這個 Panel Renderer 是排序的單位，面板之間由 Panel Settings 的 Sort Order 決定順序，同一面板內的 Panel Renderer 之間則由 PanelRenderer.sortingOrder 決定。uGUI 則以根 Canvas 的 Sort Order 排序。沒有可跨這兩者指定順序的屬性。  
從 uGUI 遷移的指南{{fn:ugui_migration}}也提到以 Render Texture 合成繪製結果的方法，但明確指出輸入事件不會送達。由於無法用於需要操作的 UI，請避免將 uGUI 與 UI Toolkit 疊加的設計。  
![][image2]  
図: UI Toolkit 與 uGUI 繪製順序的決定方式 {{fig:sort_order}}

## **編輯器擴充**

在編輯器擴充中，兩個系統可巢狀的方向僅限單向（{{fig:propertydrawer_nesting}}）。在 UI Toolkit 的樹中放置 IMGUI 的繪製，可透過 IMGUIContainer 做到；但反過來，在 IMGUI 的繪製中放置 UI Toolkit 的樹則無法做到。  
這會成為問題的情況，是自行製作 Inspector 顯示的時候。Inspector 的自訂分為兩層：負責整個元件的自訂 Inspector，以及以欄位為單位替換外觀的 PropertyDrawer；內層的 PropertyDrawer 無法選擇自己是從哪一種系統的 Inspector 被呼叫。僅以 UI Toolkit 實作的 PropertyDrawer，若從以 IMGUI 撰寫的自訂 Inspector 或 IMGUI 的 PropertyDrawer 中被呼叫，該欄位的值欄會被替換為「No GUI Implemented」（未實作 GUI）的顯示。Unity 2022.2 以後 Inspector 的預設為 UI Toolkit，因此多數情況下不會發生；但在仍留有 IMGUI 自訂 Inspector 或 PropertyDrawer 的專案中，需要在 PropertyDrawer 端也準備 IMGUI 版的繪製。  
![][image3]  
図: PropertyDrawer 巢狀的可行性 {{fig:propertydrawer_nesting}}

# **結語**

調查後印象深刻的是，官方的推薦理由在 Unity 6.5 幾乎都已移到 UI Toolkit 側。執行階段仍以 uGUI 為標準，沒有理由遷移正在運作的東西，但在新專案開發中避開 UI Toolkit 的理由確實在減少。若難以抉擇，依據對 Animator 與 Timeline 的動畫表現以及 Asset Store 資產的依賴程度來判斷較為妥當。編輯器擴充方面，以複雜工具用 UI Toolkit、需貼近 Unity 本體外觀的場合用 IMGUI 來區分，目前應不會有困擾。UI 系統的選擇，是決定是否需要前作所討論之因應方式的最上游調優。

脚注: {{fn:tuning_bible}} https://cyberagentgameentertainment.github.io/UnityPerformanceTuningBible/ja/preface.html  
脚注: {{fn:ui_system_compare}} https://docs.unity3d.com/6000.5/Documentation/Manual/UI-system-compare.html（2026 年 8 月 6 日存取）  
脚注: {{fn:imgui_migration}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-IMGUI-migration.html  
脚注: {{fn:uitk_status}} https://discussions.unity.com/t/ui-toolkit-development-status-and-next-milestones-november-2025/1698009  
脚注: {{fn:dotween}} https://dotween.demigiant.com/download.php  
脚注: {{fn:ui_system_compare_60}} https://docs.unity3d.com/6000.0/Documentation/Manual/UI-system-compare.html（2026 年 8 月 6 日存取）  
脚注: {{fn:ui_system_compare_2022}} https://docs.unity3d.com/2022.3/Documentation/Manual/UI-system-compare.html（2026 年 8 月 6 日存取）  
脚注: {{fn:ui_document_component}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-create-ui-document-component.html  
脚注: {{fn:ugui_migration}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-Transitioning-From-UGUI.html

[image1]: assets/zaitsu-hiromu-2/media/image1.png

[image2]: assets/zaitsu-hiromu-2/media/image2.png

[image3]: assets/zaitsu-hiromu-2/media/image3.png