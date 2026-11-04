//raw[|latex|\reviewcompactschapterhead]
=[nonum] 前言

自 2022 年出版以來獲得許多迴響的前作《Unity Performance Tuning Bible》@<fn>{preface_prev_book}，為了讓任何人都能從零開始有系統地掌握效能調優，除了釐清原因的工作流程及其前提的基礎知識之外，也解說了從圖形到腳本等廣泛領域的調優實例。

//footnote[preface_prev_book][@<href>{https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningBible}]

到了 2026 年的今天，前作所述的內容仍有許多部分可以直接作為參考。另一方面，距出版已過了 4 年，從 Unity 6 世代的登場到 AI 所帶來的開發體驗本身的改變，各個領域都產生了巨大的變化。因此，為了停下腳步確認效能調優的現況，我們集結了公司內部的專家，彙整出本書《Unity Performance Tuning Tips 2026》。

本書將搭配實測值，探討廣泛的主題，從 Render Graph、GraphicsStateCollection、Android 的 Link Time Optimization 等 Unity 6 起新增的可選功能，到可在開發現場立即嘗試的實務主題。

調優的判斷最終取決於各專案的測量結果。本書所提供的是判斷的素材：要看哪裡、與什麼比較、在哪裡收手。若能對拿起本書的各位的開發有些許幫助，將深感榮幸。

==[nonum] 關於本書

本書的內容以行動裝置應用程式為前提。部分解說可能不適用於其他平台，敬請留意。

驗證使用的是 Unity 6 系列（6000.x），但所用版本依章節而異。除非另有說明，請參照各章所記載的版本。除了探討 Unity 6 之後新增功能的章節外，許多內容也適用於更早的版本。此外，本書刊載的數值皆為各章所記載的測量環境下的實測值或試算值。環境不同，數值與趨勢也會隨之改變，因此請不要只看數值本身，而是從中理解是什麼、如何產生了效果。

==[nonum] 本書的結構

本書由 4 個部分、21 章構成。各章彼此獨立，因此也可以只閱讀需要的部分。卷首的「依課題查詢的索引指南」彙整了代表性的症狀及探討該症狀的章節。若已經知道要處理的課題，從該指南查起是最快的途徑。「序章」則整理了自前作以來的這幾年間，Unity 與行動環境發生了什麼變化。

第 1 部「圖形最佳化」探討繪製相關的主題，包括以 Render Graph 節省圖塊記憶體、UnsafePass 的使用時機、省略 FinalBlitPass、著色器的輕量化與預熱等。

第 2 部「腳本與動畫最佳化」探討角色動畫的調優、以 VAT 減輕骨骼負載、ParticleSystem 的尖峰、非同步處理的生命週期管理，以及 UI 系統的取捨。

第 3 部「資產、設定與建置最佳化」探討 AssetBundle 的生命週期管理、資產膨脹的調查、依裝置切換繪製設定、Code Stripping 與 LTO，以及 iOS 的記憶體上限。

第 4 部「測量與自動化」探討 iOS 標準的測量功能、將每日建置的實機效能分析自動化的架構，以及將本書的知識轉換為 AI 程式設計代理技能的嘗試。

==[nonum] GitHub

本書的儲存庫@<fn>{preface_repo}已公開。我們預計會隨時增補修訂。此外，也可以透過 PR 或 Issue 指出修正之處或提出增補建議，歡迎多加利用。

//footnote[preface_repo][@<href>{https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026/}]
