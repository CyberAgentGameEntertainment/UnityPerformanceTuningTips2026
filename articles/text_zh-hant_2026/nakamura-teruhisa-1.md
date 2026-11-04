# **將調優寶典製作成 AI 程式設計代理的技能**

姓名 / 羅馬字: Teruhisa Nakamura  
所屬: 株式會社 Applibot

# **前言**

像本書這樣的調優知識，有一個結構性的弱點：只對讀過的人有效。  
各章都充滿了以實測為依據的知識，但這些知識能否在專案的程式碼審查中實際被指出，取決於審查者是否讀過該章，並且能在審查的當下想起來。我們無法期待團隊全員背下所有章節，而進入營運階段後若人員更替，知識也會逐漸流失。  
另一方面，寫程式的現場已經常駐著 AI 程式設計代理（Claude Code、OpenAI Codex、GitHub Copilot 等）。只要整合進審查請求或 CI，就能以相同的基準檢查所有變更。缺少的只是一套關於 Unity 效能的系統化評估基準。  
因此本章將介紹如何把調優寶典的知識轉換為「AI 代理可執行的評估項目」，並封裝成不依賴特定工具的通用技能。筆者實際將基於前作已公開知識的 77 條規則，與從本書（2026 年版）原稿中萃取的 86 條規則整合，製作成共 163 條評估規則的技能；本章將以此案例為基礎，整理轉換的設計方針與實際運用上的注意事項。

# **名為 Agent Skills 的共通格式**

## **SKILL.md 的規範**

作為讓 AI 代理具備專業知識的方法，2025 年底公開了名為 Agent Skills{{fn:agentskills}} 的開放規範。技能的實體是以 SKILL.md 這個 Markdown 檔案為中心的目錄，採用如下的組成。  
CODE START text | 技能目錄的組成 {{list:skill_layout}}  
unity-perf-review/  
├── SKILL.md          \# 中繼資料 \+ 執行步驟（必要）  
├── references/       \# 規則集等參考文件  
├── scripts/          \# 可執行腳本（選用）  
└── assets/           \# 範本等（選用）  
CODE END  
SKILL.md 的開頭要寫稱為 front matter 的中繼資料。必要欄位只有 name 與 description 兩個。  
CODE START yaml | SKILL.md 的 front matter（description 為配合版面而縮短，metadata 等選用欄位已省略） {{list:skill_frontmatter}}  
\---  
name: unity-perf-review  
description: \>  
  Unity 專案的程式碼、資產與設定，從效能觀點  
  進行審查。將 C\# 腳本、著色器、.meta、ProjectSettings 與  
  163 條規則比對，輸出 Critical / Warning / Info 等級的審查意見。  
  Unity 效能審查或最佳化調查的需求出現時使用。  
license: MIT  
\---  
CODE END  
重要的是 description 的寫法。代理會將使用者的請求內容與 description 比對後自動觸發技能，因此不只寫「做什麼」，連「何時使用」也寫上，可以提高觸發的準確度。

## **主要代理的支援狀況**

這個規範的價值在於多個代理都能讀取同一種格式。{{table:agent_support}}列出了截至 2026 年 8 月的支援狀況。  
表: 主要程式設計代理的技能支援（截至 2026 年 8 月） {{table:agent_support}}

| 代理 | 支援 | 放置位置 |
| :---- | :---- | :---- |
| Claude Code | 原生支援 SKILL.md{{fn:claudecode}} | .claude/skills/ 或 \~/.claude/skills/ |
| OpenAI Codex | 支援 SKILL.md | .agents/skills/ 或 \~/.agents/skills/ |
| GitHub Copilot | 支援 SKILL.md{{fn:copilot}} | .github/skills/ |
| Gemini CLI | 支援 SKILL.md{{fn:gemini}} | .gemini/skills/ 或 \~/.gemini/skills/ |

Codex / Copilot / Gemini CLI 也支援共通的 .agents/skills/ 放置位置。此外 Copilot 也會讀取專案的 .claude/skills/，因此只要放置在 .claude/skills/ 一處，就能同時供 Claude Code 與 Copilot 使用（Copilot 的個人用放置位置為 \~/.copilot/skills/ 或 \~/.agents/skills/）。支援狀況與放置位置會隨時間變動，導入時請在各代理的官方文件確認第一手資訊。  
只要將調優知識整理成 SKILL.md 格式一次，就能把相同的審查基準（規則與步驟）發布給使用任何代理的成員。Agent Skills 統一的是封裝格式與步驟，模型或工具權限的差異仍會使執行結果不盡相同。即便如此，「審查基準放在儲存庫，執行交給代理」的分工仍然成立。

# **將知識轉換為「評估項目」**

技能化的核心並不是撰寫 SKILL.md，而是把各章的知識分解到代理能機械式套用的粒度。筆者統一採用了下列格式。  
CODE START text | 規則格式（從 ParticleSystem 章節萃取的範例。grep\_pattern 的排除條件與處置的細節為配合版面而縮短） {{list:rule_format}}  
\#\#\# US11 對 ParticleSystem.Simulate 指定過大的秒數  
\- 等級：Warning（Noise 模組啟用時為 Critical）  
\- 適用範圍：程式碼審查 / 資產設定  
\- 偵測模式：將過大的秒數傳給 Simulate 的第 1 個引數。  
  特別是與啟用 NoiseModule 的 ParticleSystem 併用時  
\- grep\_pattern: \\.Simulate\\(  
\- 理由：Simulate 會一次模擬到指定時間為止。  
  實測中指定 20 秒會使 NoiseJob 被呼叫約 2000 次，  
  產生約 50ms 的尖峰  
\- 處置：在啟用 Noise 的粒子上避免使用 Simulate  
CODE END  
這個格式包含了 4 項設計決策。

## **明示偵測能否機械化**

每條規則都以 grep\_pattern 的形式，附上針對 C\# 程式碼或 Unity 的 YAML（.meta、Prefab、ProjectSettings）可機械執行的搜尋條件。多數是一條正規表示式，但也包含跨多行比對，或偵測「應有的敘述不存在」的規則。代理先依此條件做初步偵測，再只精讀命中的位置來判斷。如此即使是大規模儲存庫，也能以實際可行的成本進行審查。  
同樣重要的是，不要勉強寫出無法以正規表示式表達的項目。例如「未登錄至 Addressables 的共用資產被多個 Bundle 參照」就無法用 grep 取得。這類規則要明確標示為（手動確認），並在處置欄寫上 Addressables Analyze 等確認手段。若偽造可偵測性，代理不是大量產生誤判，就是默默略過無法偵測的規則。

## **在理由欄保留實測值**

「理由：因為效能會惡化」這樣的規則沒有說服力。要把原稿中的實測值原封不動地引用到理由欄。

* 未壓縮紋理：2048x2048 時 Compressed 2.7MB / Uncompressed 16.0MB（約 6 倍差距）  
* 每個角色一張 Graph 的手動 Playable：600 個角色時 8.396ms，對比整體一張 Graph 的 4.500ms（1.87 倍差距）  
* Simulate(20f) 與 Noise 併用：NoiseJob 約 2000 次呼叫、約 50ms 的尖峰

有了數值，代理的審查意見本身就成了「為什麼該修」的說明，也成為受審查的開發者回頭查閱原稿該章的引導。不過，這些都是原稿所載特定環境下的實測值。規則的理由欄只引用代表值，測量條件的細節則採取參照出處章節的做法。

## **以等級與覆寫機制讓規則經得起實際運用**

規則附有 Critical / Warning / Info 三個等級（Critical＝效能劣化的確定原因，一旦確認適用條件成立就必須修正；Warning＝視條件可能成為問題的潛在風險；Info＝選擇性的改善建議），並另外準備可由專案端覆寫的機制（rules-overrides）。例如即使「使用 LINQ 為 Critical」，在初始化時（init 情境）放寬為 Warning、在 Editor 工具（tooling）則 Skip，諸如此類的調整。  
調優規則本質上依賴情境。沒有覆寫機制的話，就會變成「不適合我們專案的意見太多」而把整個技能停用。基準共通、套用依專案而定，這樣的雙層結構是長久運作的訣竅。

## **無法靜態偵測的知識另外歸入診斷知識**

原稿的知識中，有許多是讀程式碼也看不出來的。例如「削減了 CPU 處理但影格時間不變，時間轉移到 Semaphore.WaitForSignal 等等待標記時，應懷疑渲染執行緒端位於關鍵路徑上，並在 Timeline 檢視確認等待的對象」這類關於如何解讀 Profiler 的知識。「光憑等待標記無法斷定瓶頸因素」這樣的限制，也作為知識的一部分保留下來。  
這些知識與評估規則分開，整理成「症狀 → 原因 → 處置」的表格放在另一個檔案（{{table:diagnosis_example}}）。在筆者的儲存庫中，只有效能分析資料解析用的代理（供 Claude Code 使用）會在診斷時讀取。本章發布的技能中則作為參考資料一併附上，在靜態審查無法判斷、需以「建議進行效能分析測量」回報疑點時，成為其依據。  
表: 診斷知識的範例 {{table:diagnosis_example}}

| 症狀 | 原因 | 處置 |
| :---- | :---- | :---- |
| CPU 圖表呈現谷底與尖峰交錯的梳狀 | 降頻執行時忘了錯開相位 | 以 index % interval 分散相位 |
| MeshSkinning.CalcMatrices 偏大 | 骨骼 Transform 階層被實體化 | 套用 Optimize Game Objects |
| 僅 iOS，沒有錯誤與 Shader.CreateGPUProgram，只出現 GpuProgramMetal.GetCachedPipeline | 懷疑著色器在 AssetBundle 中重複打包 | 確認重複的實例並改為專用 Bundle |
| 僅 iOS，預熱完成後 SubPass 內的著色器仍發生 PSO 編譯 | SubPass（Native Render Pass）內著色器的預熱失敗 | 重新檢視 SubPass，或在載入中預先渲染 |

# **組裝成技能**

## **漸進式載入與檢查清單方式**

若把 163 條規則全部寫進 SKILL.md 本文，會壓迫代理的上下文。Agent Skills 的設計思想是漸進式載入（progressive disclosure）。

1. 代理平時只看 front matter 的 name 與 description  
2. 技能觸發後才讀取 SKILL.md 本文（執行流程）  
3. 只從 references/ 讀取需要用到的參考文件

初期讀取哪些內容（是否包含路徑等）取決於代理的實作，但「本文與參考資料在觸發前不會被讀取」這個原則是共通的。  
這裡要注意的是完整性。直觀的做法是「依目標檔案類型整個載入對應類別的規則檔案」，但在這種方式下，代理在讀程式碼之前先讀的規則量最多會超過 2 萬 token（由字數概算，token 數依模型而異）。若把「全部記住並套用」數十條規則交給模型的注意力，就會有部分規則被默默略過。  
因此，筆者從全部規則自動產生一份將「ID / 等級 / 適用條件 / grep\_pattern / 規則名稱 / 詳細檔名」壓縮成一條規則一行的檢查清單，並將審查分為兩個階段。

1. **初步掃描**：隨附的腳本 scan.py 對目標檔案機械式執行檢查清單全部行的 grep\_pattern，輸出命中清單（檔案:行）與 Coverage（沒有 python3 的環境則改由代理直接執行全部行的 grep 作為後備方案）  
2. **二次精查**：只針對命中的規則，從詳細檔案讀取偵測條件、理由與處置，排除誤判

無法以 grep 表達的少數規則，在檢查清單上明確標示為 MANUAL，只在符合目標類型時才精讀。以「應有的敘述不存在」作為判定條件的規則則標示 ABSENT，在 grep 未命中時確認詳細內容（這是為了避免把命中與否和判定條件的對應關係弄反而做的明示）。之所以把初步掃描做成腳本，是因為讓 LLM 逐一執行數十條 grep 可能會略過部分項目，而且正規表示式的方言差異（PCRE 的前瞻在 macOS 標準 grep 無法運作等）會使結果依環境而異。如此一來，初步掃描的完整性就與模型的記憶力脫鉤。審查意見的正確性（二次精查中的誤判排除）仍由代理判斷。事前讀取的只有檢查清單（程式碼用約 16KB、資產用約 8KB。token 數依模型而異，合計概算約 1 萬 token 上下），詳細內容只讀命中的部分。檢查清單由規則本體自動產生，因此新增規則時也不需要手動同步。

## **依環境排除規則**

規則中有些依賴 Unity 版本或配置。本技能收錄的 RenderGraph 相關規則（面向 URP）以 Unity 6 以後為範圍，LTO 系列專屬於 IL2CPP，iOS 特有的 PSO 問題則與僅有 Android 的遊戲無關。比對不適用的規則只會徒增雜訊與工夫。  
因此，筆者讓依賴環境的規則帶有適用條件標籤（unity\>=6000.3、urp、il2cpp、ios 等），與從專案設定檔產生的環境設定檔比對，只排除「明確不符合條件的規則」。標籤的比對由初步掃描腳本以確定性方式處理，無法偵測的項目（unknown）則偏向安全側（維持為比對範圍），略過的規則 ID 會列在報告中以便稽核。產生步驟將在導入步驟一節說明。

## **強制在輸出中附上規則 ID**

輸出格式中，規定每條審查意見都必須附上規則 ID。  
CODE START text | 審查輸出的範例 {{list:review_output}}  
\#\#\# Critical  
\- Assets/Scripts/EffectPlayer.cs:42 \-  
  對啟用 Noise 的粒子呼叫 Simulate(20f) \[US11\]  
  \- 依據：NoiseJob 被呼叫約 2000 次，產生約 50ms 的尖峰（實測）  
  \- 建議：移除 Noise 或將秒數減到最小  
CODE END  
有了 ID，就能把審查意見的依據回溯到規則檔案進行驗證，區分出「代理只是說得像那麼回事」的情況。若是誤判，也能明確知道該修正哪條規則的偵測模式。

# **發布與放置到各代理**

技能的公開只需整個目錄分享即可成立。GitHub 儲存庫或 zip 都可以。使用者只要複製到{{table:agent_support}}的放置位置，各代理就會發現技能。  
實際運用上的重點是維持規則的單一來源。在筆者的儲存庫中，規則本體只放在 rules/ 目錄，發布用的套件則由腳本組裝（使用者端的導入步驟見下一節）。  
CODE START bash | 發布套件的組裝（供維護者參考） {{list:packaging}}  
./package-skill.sh  
cp \-r dist/unity-perf-review /path/to/repo/.claude/skills/   \# Claude Code  
cp \-r dist/unity-perf-review /path/to/repo/.agents/skills/   \# Codex / Copilot / Gemini CLI  
cp \-r dist/unity-perf-review /path/to/repo/.github/skills/   \# Copilot 專用放置位置（上面兩行任一都能送達，因此為選用）  
CODE END  
一旦開始為每個代理各自管理一份規則，每次更新基準都會逐漸偏離。建議一開始就定下「來源只有一處，放置交給腳本」。

# **導入到自己的專案**

以下是在讀者的專案中運作上述機制的步驟。若已安裝 python3，複製與初次產生大約需要 10 分鐘。加入儲存庫的只有技能目錄與環境設定檔（tuning-env.md），不會變更 Unity 的 Assets / ProjectSettings。

## **取得與放置技能**

技能全套（SKILL.md、附有 163 條規則的 references/、環境偵測、初步掃描、檢查清單重新產生等 3 個腳本，以及導入步驟的 README）公開於本書儲存庫的 skills/unity-perf-review/{{fn:repo}}。只要取得後複製到所用代理的技能目錄即可。另外，技能是包含「對代理的指示與可執行腳本」的發布物。來自不可信來源的技能，建議先確認 SKILL.md 與 scripts/ 的內容再導入，並固定版本。  
CODE START bash | 取得與放置技能 {{list:install_steps}}  
git clone https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026.git  
cd UnityPerformanceTuningTips2026/skills

\# 依所用的代理複製到其中之一  
cp \-r unity-perf-review /path/to/your-repo/.claude/skills/   \# Claude Code  
cp \-r unity-perf-review /path/to/your-repo/.agents/skills/   \# Codex / Copilot / Gemini CLI（共通放置位置）  
cp \-r unity-perf-review /path/to/your-repo/.github/skills/   \# Copilot 專用放置位置（上面兩行任一都能送達，因此為選用）  
CODE END  
放置在儲存庫根目錄下，就能把相同的審查基準發給團隊全員。若只是個人試用，放在家目錄端（\~/.claude/skills/ 等）也能運作。

## **放置後的狀態**

以 Claude Code 為例，完成放置與下一項的環境設定檔產生後，儲存庫會成為如下狀態（{{list:installed_layout}}）。  
CODE START text | 放置後的儲存庫組成 {{list:installed_layout}}  
your-repo/  
|-- .claude/  
|   \`-- skills/  
|       \`-- unity-perf-review/  
|           |-- SKILL.md                      \# name/description \+ 執行流程  
|           |-- README.md                     \# 供人閱讀的導入步驟  
|           |-- references/  
|           |   |-- checklist-code.md         \# 初步掃描用（程式碼類）  
|           |   |-- checklist-asset.md        \# 初步掃描用（資產/設定類）  
|           |   |-- checklist-profile.md      \# 效能分析資料解析用（靜態審查未使用的附屬資料）  
|           |   |-- rules-csharp.md 等共 13 個檔案  \# 詳細規則（共 163 條）  
|           |   |-- diagnosis-knowledge.md    \# 症狀-\>原因-\>處置的診斷集  
|           |   \`-- rules-overrides.example.md  
|           \`-- scripts/  
|               |-- detect-env.py             \# 產生環境設定檔  
|               |-- scan.py                   \# 機械式執行初步掃描  
|               \`-- gen-checklists.py         \# 修改規則時重新產生檢查清單  
|-- tuning-env.md                             \# 環境設定檔（於下一項產生）  
\`-- （Unity 專案本體不變更）  
CODE END  
3 份檢查清單依範圍區分（程式碼 / 資產 / 效能分析），一條規則可能同時列在多份清單中，因此行數合計與規則數 163 不一致。  
放置後載入代理的，原則上只有 front matter 的 name 與 description（合計數百 token。如前所述，依實作可能再加上路徑等）。請求審查後，會依前述的漸進式載入，按 SKILL.md 本文 → 依目標對應的檢查清單 → 命中規則的詳細內容的順序，只讀取需要的部分。通常不必一次投入 163 條規則的全文（約 125KB，概算 4 萬多 token）。  
rules-overrides.example.md 直接放著並不會生效。開始進行專案專屬調整時，重新命名為 rules-overrides.md 再編輯（後述）。

## **產生環境設定檔**

如前所述，163 條規則中包含依賴環境的規則。為了讓代理判斷並排除不適用於專案的規則，要產生環境設定檔。  
CODE START bash | 產生環境設定檔（在 clone 下來的儲存庫的 skills/ 執行） {{list:detect_env}}  
python3 unity-perf-review/scripts/detect-env.py \\  
    /path/to/your-repo /path/to/your-repo/tuning-env.md  
CODE END  
若在放置後執行，請將路徑改為放置位置（.claude/skills/ 或 .agents/skills/ 等所用代理的技能目錄）。即使是 Unity 專案不在儲存庫根目錄而在子目錄的 monorepo 配置，只要傳入儲存庫根目錄，也會遞迴搜尋設定檔並偵測。  
執行腳本需要 python3（沒有的環境可以手動建立 tuning-env.md，或略過此步驟改以比對全部規則的方式運作。初步掃描的 scan.py 也使用 python3，若沒有，則以代理直接執行 grep 的後備方案運作）。腳本會從 ProjectVersion.txt、Packages/manifest.json、GraphicsSettings.asset、ProjectSettings.asset 推定 Unity 版本、渲染管線、Scripting Backend 以及是否使用 Addressables。這終究只是從設定檔推定的值，確定的資訊以手動填入 tuning-env.md 為準。無法偵測的項目（例如 Scripting Backend 依平台分別設定而混用時）會成為 unknown，並以安全側（維持為比對範圍）處理。請開啟輸出的 tuning-env.md 確認內容，特別是發行平台與是否使用 SideFX VAT 要手動修正，也請確認 render\_pipeline 是否為預期的值。

## **專案專屬的調整（選用）**

若有刻意偏離規則的地方，就在 rules-overrides.md 記錄等級的覆寫。例如允許初始化時使用 LINQ、在 Editor 工具停用日誌規則等調整。環境設定檔負責排除「在該環境原理上無法適用」的規則，overrides 則負責「專案因素的調整」，是這樣的分工。  
若想新增或修改規則本身，編輯 references/ 的 rules-\*.md，再以 python3 scripts/gen-checklists.py references/ 重新產生檢查清單。檢查清單是自動產生物，不直接編輯。只靠發布物就能完成這個新增與重新產生的循環。

## **執行**

只要向代理提出請求即可。通常會依請求文字與 description 的吻合自動觸發（觸發是由模型判斷，若想確保使用，請在請求文字中明確寫出技能名稱 unity-perf-review，或使用各代理的明示呼叫方式）。  
CODE START text | 審查請求的範例 {{list:invoke_example}}  
請從效能觀點審查這份變更  
Assets/Scripts/Battle/ 底下請以調優寶典的規則檢查  
CODE END  
在輸出最後附上的 Coverage 區段中，可以確認比對的規則數、因環境而略過的規則數（附 ID）以及 grep 命中數。這些數量是轉錄自初步掃描腳本的輸出，不依賴代理的記憶（只有二次精查中採用與誤判排除的數量由代理統計）。若覺得排除得太多，請調整 tuning-env.md；若覺得意見太多，請調整 rules-overrides.md。

# **實際運用時的注意事項**

以下列出實際導入專案後發現的注意事項。  
**第一次不要從整個儲存庫開始，而是從變更或特定目錄開始。** 初步掃描本身即使針對整個專案也能在短時間內完成，但在已營運一段時間的程式碼庫中，命中的規則會達到數十項的規模，二次精查（排除誤判）的負擔會集中在那裡。先在較小的範圍精查，把長期允許的項目寫入 overrides 之後再擴大範圍，審查意見才不會被雜訊淹沒而能夠落實。  
**靜態分析也會指出刻意的設定。** 例如刻意不壓縮的紋理也會被 AS02（未設定紋理壓縮）命中。不要全盤接受偵測結果一次修完，而要逐項確認意圖後再修正。長期允許的項目附上理由記錄到 overrides，就能減少下次以後的雜訊。overrides 的調整單位是整條規則或情境（in-game＝熱路徑 / init＝初始化 / tooling＝Editor），僅針對特定資產的例外則在備註欄寫上路徑或理由，作為代理的判斷材料（目前版本尚未支援專用的適用範圍指定）。  
**實測值附帶測量環境。** 規則中引用的數值是在特定 Unity 版本與裝置上的實測。「在 Pixel 8a 的實測」無法外推到最新高階裝置的判斷。規則本身要寫上適用版本，並以 Unity 或渲染管線的更新作為重新檢視規則的觸發點。  
**最後做判斷的是人。** 借用本書其他章節的說法，「決定與重新檢視規約的是人，讓規約被遵守的是機器」。AI 代理是「讓規約被遵守的機器」的新實作，對規約＝評估項目的妥當性負責的，依然是人。若無法認同代理的意見，請把它視為應改善規則的偵測模式或理由欄的訊號。

# **結語**

嘗試把調優寶典的知識規則化後，筆者深切體會到「實測值與處置成對寫下的原稿，幾乎可以直接成為機器可讀的評估項目」。反過來說，沒有數值依據的經驗法則，在規則化的階段就會暴露其弱點。這也是對撰寫方的回饋。明確寫出測量環境與數值的知識，不只會傳達給書籍的讀者，還會透過 AI 代理抵達所有的程式碼審查。  
本章介紹的技能與規則集，與其說是唯一的正解，不如說是起點。期待它成為各專案的 overrides 逐漸成長、新章節的知識持續作為規則加入的、活的規約集。  
脚注: {{fn:repo}} https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026/tree/main/skills/unity-perf-review  
脚注: {{fn:agentskills}} Agent Skills 規範 https://agentskills.io/specification  
脚注: {{fn:claudecode}} Claude Code 的 Skills 文件 https://code.claude.com/docs/en/skills  
脚注: {{fn:copilot}} GitHub Docs "About agent skills" https://docs.github.com/en/copilot/concepts/agents/about-agent-skills  
脚注: {{fn:gemini}} Gemini CLI 的 Agent Skills 文件 https://geminicli.com/docs/cli/using-agent-skills/