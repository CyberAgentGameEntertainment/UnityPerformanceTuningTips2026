# Unity 的 LTO 設定與 Android、iOS 的效能比較

姓名 / 羅馬字：Minhyuk Kim  
所屬：株式會社 CyberAgent SGE 核心技術本部

Unity 6000.5 新增的 Android 連結時最佳化（LTO）設定只會套用到引擎程式碼。若要讓自行撰寫的 C\# 所產生的程式碼也套用 LTO，需要另外的設定。本章嘗試了這兩種設定，並在基準測試與實際專案中測量效能、二進位檔大小與建置時間。

## 1\. IL2CPP 的編譯與翻譯單元

在 Unity 的 Android 建置（IL2CPP）中，C\# 會依 {{list:il2cpp_build_flow}} 的流程轉換為機器碼。  
CODE START text | IL2CPP 建置的處理流程 {{list:il2cpp_build_flow}}  
C\# 原始碼  
  ↓ C\# 編譯  
IL 組件 (.dll)  
  ↓ IL2CPP 轉換  
大量的 C++ (.cpp)          ← 「IL2CPP 產生的程式碼」  
  ↓ clang 進行編譯（每個 .cpp 分別最佳化）  
  ↓ 連結  
libil2cpp.so               ← C\# 由程式碼產生的最終原生二進位檔  
CODE END  
clang 會以翻譯單元（通稱 TU）為單位編譯 cpp，並產生目的檔。TU 是指編譯器個別讀取並最佳化的單位。

在一般的 C++ 建置中，由於無法參照其他 TU 內的函式本體，即使是短小的函式有時也不會被內聯，而留下 call 指令。IL2CPP 產生的程式碼含有大量小函式與安全檢查，屬於容易受此限制影響的結構。例如 \`buf\[idx\]\` 這一行 C\#，大致會變成 {{list:il2cpp_bounds_check}} 的 C++。
CODE START cpp | IL2CPP 產生的陣列存取程式碼範例 {{list:il2cpp_bounds_check}}  
if (buf \== NULL) ThrowNullReferenceException();               // null 檢查  
if ((uint32\_t)idx \>= buf-\>max\_length) ThrowIndexOutOfRange(); // 邊界檢查  
val \= buf-\>m\_Items\[idx\];  
CODE END  
產生的程式碼規模越大，跨 TU 的呼叫也越多，呼叫所花費的時間總和也隨之增加。

## 2\. 什麼是 LTO / ThinLTO

LTO（Link Time Optimization）是 clang/LLVM 在連結時進行跨 TU 內聯、常數傳播與移除不需要程式碼的機制。編譯時會將目的檔輸出為 LLVM bitcode（最佳化前的中間表示）。連結時再將它們統一最佳化，並產生機器碼。

full LTO 會把所有中間表示合併為一個再進行最佳化。雖然可最佳化的範圍最廣，但中間表示的整合與最佳化集中在單一程序，大規模建置會使用大量記憶體。本次在實際專案的建置過程中，記憶體使用量也達到 20GB，程序被 OS 強制終止。

ThinLTO 會在編譯時為每個 TU 建立摘要。連結時從其他 TU 匯入（import）需要的函式，並平行進行最佳化。相較於 full LTO，較容易控制記憶體使用量與處理時間，廣泛用於大規模建置。

## 3\. 引擎程式碼的 ThinLTO 設定

Unity 6000.5 新增了 Link Time Optimization 開關。  
![][image1]{scale=0.85}  
図: Build Profiles ▸ Player Settings ▸ Android {{fig:lto_toggle}}  
啟用此開關後，會選用 Unity 提供、已套用 ThinLTO 的引擎二進位檔。
套用範圍是 Unity 引擎程式碼，不包含 IL2CPP 產生的程式碼（源自 C\# 的程式碼）。
編輯器的安裝資料夾中同時附有一般版與 ThinLTO 版的預先建置 so 檔。
CODE START text | Android Player 隨附的 libunity.so 變體 {{list:engine_binary_variations}}  
PlaybackEngines/AndroidPlayer/Variations/il2cpp/  
├── Release/Libs/arm64-v8a/libunity.so          29,032,960 bytes  
└── Release\_ThinLTO/Libs/arm64-v8a/libunity.so  30,042,536 bytes (+3.5%)  
CODE END  
![][image2]{scale=0.8}
図: 編輯器的安裝資料夾。\`Release\` 與 \`Release\_ThinLTO\` 皆隨附其中，建置時會選用其中一個 {{fig:il2cpp_variations}}

### 3.1 啟用條件與未滿足時的行為

開關生效的條件有以下 3 項。若有任一項未滿足，不會出現錯誤或警告，  
\`LinkTimeOptimization\` 會以 \`None\` 建置。  
1\. IL2CPP 腳本後端  
2\. Strip Engine Code 為停用（因為會直接使用未剝離的 prebuilt）  
3\. 非 Development 建置  
若啟用 Development Build，開關本身就不會顯示。
是否已套用，可從 {{list:engine_toggle_verification}} 的 2 點確認。  
CODE START bash | 引擎程式碼 LTO 開關的套用確認指令 {{list:engine_toggle_verification}}  
\# ① 建置輸入中是否指定了 ThinLTO  
grep \-o 'LinkTimeOptimization"\[: \]\*"\[A-Za-z\]\*"' Library/Bee/Player\*\-inputdata.json  
\# ② APK 內的 libunity.so 是否與 Release\_ThinLTO 版一致  
unzip \-p out.apk lib/arm64-v8a/libunity.so | md5  
CODE END

## 4\. 引擎程式碼 LTO 開關的效果

比較隨附的一般版與 ThinLTO 版二進位檔，並以變更較多的子系統為中心，在實機上測量效能。

### 4.1 比較隨附二進位檔的函式大小

Unity 在編輯器的安裝資料夾中隨附了兩種變體的含符號 \`.so\`。  
CODE START text | Android Player 隨附的含符號 libunity.so {{list:engine_symbol_variations}}  
PlaybackEngines/AndroidPlayer/Variations/il2cpp/  
├── Release/Symbols/arm64-v8a/libunity.sym.so  
└── Release\_ThinLTO/Symbols/arm64-v8a/libunity.sym.so  
CODE END  
比對以 \`llvm-nm \--defined-only \--print-size \--demangle\` 取得的函式名稱，並依命名空間與類別名稱按子系統彙總（{{table:engine_function_diff}}）。

表: 引擎程式碼 LTO 開關造成的各子系統函式差異 {{table:engine_function_diff}}{{tsize:38,30,20,16,16}}  
| 子系統 | 函式數 (Release → ThinLTO) | 大小改變的函式數 | 大小變化率 | \`.text\` 差 |  
|---|---:|---:|---:|---:|  
| Particle | 1,419 → 1,290 | 1,043 | 80.9% | \+49.66% |  
| Serialization/Asset | 2,890 → 1,387 | 1,040 | 75.0% | \-16.61% |  
| Mesh/Geometry | 1,055 → 765 | 483 | 63.1% | \+6.79% |  
| Scripting | 8,924 → 7,680 | 4,302 | 56.1% | \+10.27% |  
| Gfx/Shader/Texture | 5,062 → 3,935 | 2,044 | 52.1% | \+24.21% |  
| Physics | 2,680 → 2,439 | 1,229 | 51.1% | \+15.92% |  
| NavMesh | 739 → 582 | 296 | 51.1% | \+11.87% |  
| XR/VFX/Video/其他 | 3,828 → 2,483 | 1,230 | 49.6% | \+13.94% |  
| Transform/Scene/Obj | 2,027 → 1,630 | 781 | 47.9% | \+10.95% |  
| Animation/Mecanim | 3,226 → 2,428 | 1,134 | 46.8% | \+13.18% |  
| Box2D | 2,169 → 1,562 | 712 | 46.4% | \-0.43% |  
| JobSystem/Threading | 244 → 181 | 78 | 43.6% | \+3.16% |  
| UI/UIToolkit | 883 → 701 | 293 | 42.0% | \+1.38% |  
| TextCore/Font/ICU | 3,500 → 2,101 | 861 | 41.9% | \-5.73% |  
| （未分類） | 29,193 → 21,077 | 7,511 | 37.6% | \+3.37% |  
| Renderer/Culling | 2,789 → 2,199 | 785 | 35.7% | \+27.58% |  
| C++ 標準/壓縮/基礎設施 | 13,158 → 9,484 | 3,071 | 32.6% | \-13.31% |  
| curl/TLS/HTTP | 1,809 → 1,558 | 318 | 21.0% | \+1.17% |  
| Audio/FMOD | 2,996 → 2,680 | 496 | 18.5% | \+2.77% |  
| Android/JNI | 1,445 → 1,186 | 193 | 16.3% | \-5.52% |  
| PhysX | 6,000 → 5,997 | 0 | 0.0% | \-0.05% |  
| 合計 | 96,036 → 73,345 | 27,900 | 38.8% | \+4.41% |  
※「大小改變的函式數」為函式的個數。「大小變化率」表示在 Release 與 ThinLTO 兩者中都存在的函式中，大小有改變者所占的比例。

整體函式數從 96,036 減少到 73,345，但 \`.text\` 的大小增加了 4.41%。

{{pagebreak}}

### 4.2 以 13 個工作負載進行的基準測試

以 {{table:engine_function_diff}} 為基礎，製作了在 1 個 APK 內放入 13 個工作負載的基準測試場景。  
表: 引擎程式碼 LTO 開關的基準測試工作負載 {{table:engine_workloads}}  
| 工作負載 | 目標子系統 | 內容 |
|---|---|---|  
| \`particle\` | Particle | 20,000 個粒子 \+ noise/size/color 模組，\`Simulate\` |  
| \`gfx\` | Gfx/Shader | 對 \`MaterialPropertyBlock\` 寫入 16 個屬性 × float/vector \+ matrix 後 \`Clear\` |  
| \`cull\` | Renderer/Culling | \`CalculateFrustumPlanes\` \+ 4,096 個 AABB 的 \`TestPlanesAABB\` |  
| \`physbind\` | Physics 繫結 | \`Rigidbody\` / \`Collider\` 屬性往返 |  
| \`anim\` | Animation | legacy \`Animation\` 48 個 rig × 8 個骨骼 × 10 條曲線的 \`Sample()\` |  
| \`json\` | Serialization | \`JsonUtility\` 的往返 |  
| \`instantiate\` | Serialization | \`Instantiate\` → \`DestroyImmediate\`（原生的 clone/transfer 路徑） |  
| \`xform\` | Transform/Scene | 改寫具有階層的 Transform |  
| \`bind\` | Scripting 繫結 | 大量呼叫極小的繫結 |  
| \`mesh\` | Mesh/Geometry | 重建 96×96 的程序式網格 |  
| \`phys2d\` | Box2D | 2D 剛體的 \`Simulate\` |  
| \`text\` | TextCore/Font | \`TextGenerator.Populate\` |  
| \`physics\` | PhysX | 3D 剛體的 \`Simulate\` |  
※ {{table:engine_function_diff}} 的子系統中，難以從 C\# 建立每個樣本約 100ms 的穩定合成負載者，已排除在測量範圍之外。這些子系統未評估對效能的影響。

{{pagebreak}}

### 4.3 各工作負載的測量結果

在 Pixel 8a（Tensor G3）上，使用只有 \`libunity.so\` 不同的 APK 測量前述工作負載。以 Release / ThinLTO / ThinLTO / Release 的 ABBA 順序重複 3 次，共執行 12 個工作階段。每個工作階段都會重新安裝應用程式，中間間隔 90 秒的冷卻時間。

數值為 1 次操作所需的時間（ns/op）。操作的單位依工作負載而異，絕對值請只在同一列內比較。
表: 引擎程式碼 LTO 開關的各工作負載基準測試結果 {{table:engine_benchmark_results}}  
| 工作負載 | Release (ns/op) | ThinLTO (ns/op) | 差 |  
|---|---:|---:|---:|  
| \`gfx\` | 990.46 | 852.49 | \-13.93% |  
| \`cull\` | 96,520 | 87,499 | \-9.35% |  
| \`text\` | 275,043 | 265,123 | \-3.61% |  
| \`instantiate\` | 14,620 | 14,206 | \-2.83% |  
| \`xform\` | 6,071.8 | 6,008.1 | \-1.05% |  
| \`phys2d\` | 222,269 | 220,327 | \-0.87% |  
| \`mesh\` | 2,340,210 | 2,334,143 | \-0.26% |  
| \`json\` | 15,864.8 | 15,934.4 | \+0.44% |  
| \`bind\` | 140.80 | 141.47 | \+0.47% |  
| \`physbind\` | 607.28 | 610.74 | \+0.57% |  
| \`particle\` | 265,635 | 267,335 | \+0.64% |  
| \`anim\` | 28,100 | 28,563 | \+1.65% |  
| \`physics\` | 484,641 | 482,796 | \-0.38% |  
| \[合成: 幾何平均\] | 21,918 | 21,380 | \-2.46% |  
※ 由於各列單位不同，合成值僅作為比率才有意義。  

改善幅度較大的 \`gfx\` 與 \`cull\`，是從 C\# 每個樣本呼叫小函式數百萬次的處理。\`text\` 的 \`TextGenerator.Populate\` 也會在內部執行以字形為單位的迴圈。在單次處理小、呼叫次數多的路徑上，跨 TU 內聯的效果最為明顯。

### 4.4 導入時的效能、大小與記憶體比較

在本次測量中，繫結呼叫較多的部分路徑有縮短 CPU 時間的效果。不過，實際專案的改善幅度取決於這類呼叫在影格內所占的比例。

導入時必須付出的成本整理於 {{table:engine_adoption_costs}}。
表: 採用引擎程式碼 LTO 開關的成本 {{table:engine_adoption_costs}}  
| 類別 | 實測差 |  
|---|---:|  
| Strip Engine Code OFF 造成的大小增加 | APK \+5.67MB / 安裝 \+20.3MB |  
| Strip Engine Code OFF 造成的記憶體增加 | RSS \+18.0MB / 其中不易回收的區域 \+8.0MB |  
| engine ThinLTO 本身 | APK 大小 \+398KB（30,621,595 → 31,019,967）/ 記憶體未測量 |  
※ 安裝大小的增量，是 APK 的增量與裝置上解壓縮後的 \`.so\` 增量（arm64 的 \`.so\` 合計 \+14.65MB）之總和。  

Unity 針對同一開關，公開了在 5 個專案中各測量 10 次冷啟動的結果。CPU 時間從 13.88ms 縮短為 13.36ms（\-3.7%），到初次顯示的時間從 247.96ms 縮短為 229.90ms（\-7.3%），APK 大小 \+3.3%，對建置時間的影響則報告為輕微 {{fn:unity_lto}}。

CPU 的改善幅度與本次測量相近。不過，公開值的 APK 大小 \+3.3% 是在 Strip Engine Code 皆為停用的配置之間比較，因此不包含停用所伴隨的安裝大小 \+20.3MB、記憶體 \+8.0MB 的增加。

{{pagebreak}}

## 5\. 對 \`libil2cpp.so\` 套用 LTO 的方法

要對包含 C\# 轉換而來程式碼的 \`libil2cpp.so\` 套用 LTO，有使用 Unity 的 Master 配置，以及直接傳入 clang 旗標兩種方法。

### 5.1 Master 配置與 IL2CPP LTO Mode 的設定

![][image4]{scale=0.85}  
図: IL2CPP LTO Mode 與 C++ Compiler Configuration 的設定畫面 {{fig:il2cpp_player_configuration}}  
將 Player Settings ▸ Configuration 的 C++ Compiler Configuration 設為 Master，即會啟用對 \`libil2cpp.so\` 的 LTO。\`IL2CPP LTO Mode\` 則是用來選擇此時使用 Thin / Full 哪種模式的項目。

根據 Unity 官方文章，IL2CPP LTO Mode 自 6000.5 起新增。
在 6000.5 之前的 Android 上，Release 與 Master 在是否套用 LTO 上沒有差異。
不過在 6000.5 的 Android 平台上無法選擇模式，固定為 Thin；從 6000.6 起才能在 Master 配置中選擇 Thin / Full。截至 6000.7，仍沒有在維持 Release 配置下只啟用 LTO 的項目。

Player Settings 的 C++ Compiler Configuration 對應的旗標，可從 Bee 建置圖的 Il2CppCompilerConfiguration 確認。
表: Unity 各 C++ Compiler Configuration 的編譯器旗標 {{table:compiler_configuration_flags}}  
| 版本 | Release | Master |  
| \---------------------- | \------------ | \------------------- |  
| Android 6000.3 (arm64) | \-Os（大小優先）、無 LTO | \-Os、無 LTO（相同） |  
| Android 6000.5 (arm64) | \-Os | \-O2（在速度與程式碼大小之間取得平衡的最佳化） \+ ThinLTO |  
| iOS 6000.3 / 6000.5    | \-O3（以程式碼大小可能增加為代價，更積極追求速度的最佳化）、LTO OFF | \-O3、LTO OFF（相同）  |  

### 5.2 在 Release 配置中加入 \`-flto=thin\`

如前所述，切換到 Master 配置是最快獲得 LTO 效果的途徑，但在套用 LTO 的同時，最佳化等級也會從 \`-Os\` 變為 \`-O2\`（{{table:compiler_configuration_flags}}），缺點是大小與建置時間會增加。若在維持 Release 配置下只套用 ThinLTO，不僅能單獨測量 LTO 的效果，也能抑制大小與建置時間的增加。要對自行撰寫的 C\# 所產生的 \`libil2cpp.so\` 套用 ThinLTO，可考慮如 {{list:manual_thinlto_build}} 直接傳入 clang 旗標的方法。

CODE START csharp | 在 Release 配置中啟用 ThinLTO 的建置前處理 {{list:manual_thinlto_build}}  
// 先保存既有的額外引數 → 追加 → 務必還原  
var saved \= PlayerSettings.GetAdditionalIl2CppArgs();  
try  
{  
    var extra \= "--compiler-flags=-flto=thin \--linker-flags=-flto=thin";  
    PlayerSettings.SetAdditionalIl2CppArgs(  
        string.IsNullOrEmpty(saved) ? extra : saved \+ " " \+ extra);  
    BuildPipeline.BuildPlayer(options);  
}  
finally  
{  
    PlayerSettings.SetAdditionalIl2CppArgs(saved);  
}  
CODE END  
是否已套用，分 2 個階段確認（{{list:il2cpp_thinlto_verification}}）。

第 1 個是旗標是否已傳入建置指令。Unity 的建置系統（Bee）會將執行過的編譯與連結指令記錄在 \`Library/Bee/Player\*.dag.json\`。以 \`-flto=thin\` 搜尋此檔案，查看 IL2CPP 產生程式碼的編譯與 \`libil2cpp.so\` 的連結兩者是否都包含該旗標。

第 2 個是產生的目的檔是否為 LLVM bitcode。啟用 LTO 後，編譯結果的 \`.o\` 不會是機器碼，而是 bitcode。從第 1 步找到的編譯指令中選一個輸出檔案，顯示其開頭 4 個位元組。

CODE START bash | IL2CPP ThinLTO 的套用確認指令 {{list:il2cpp_thinlto_verification}}  
\# 1\. 編譯與連結指令中是否有 \-flto=thin  
rg \-n \-- '-flto=thin' Library/Bee/Player\*.dag.json  
\# 2\. 產生目的檔的開頭 4 個位元組（指定在 1\. 中找到的 .o）  
xxd \-l 4 Library/Bee/artifacts/Android/.../GeneratedCode.o  
CODE END  
若為 bitcode 會顯示 \`4243 c0de\`（\`BC..\`），若為一般機器碼的目的檔則顯示 \`7f45 4c46\`（\`.ELF\`）。若隨意挑選資料夾內的 \`.o\`，可能會選到不在 LTO 範圍內的第三方函式庫目的檔。第 2 個指令務必指定第 1 個搜尋結果中出現的檔案。

### 5.3 基準測試的設計

#### 5.3.1 合成程式碼的產生

此基準測試以一般編譯無法最佳化的跨 TU 呼叫作為主要測量目標。  
由於 asmdef 的邊界與 C++ 的 TU 邊界並非相同概念，因此也在 IL2CPP 轉換後產生的 C++ 與  
Bee 的編譯圖中，確認了呼叫端與被呼叫端位於不同 TU。  
程式碼產生器所產生的內容如下。  
\- 16 個 asmdef 組件 × 24 個類別 × 8 個方法 \= 3,072 個方法  
\- 呼叫鏈繞行 1 圈會穿越 16 個組件邊界，所有方法還會額外呼叫共用組件的輔助函式（\`Core.Mix\` / \`Core.Idx\` / 結構體運算）  
\- 基準測試會巡迴所有類別，因此不會被 managed 剝離移除  
\- 每個方法使用不同常數，以防止 ICF 摺疊

{{pagebreak}}

產生的 3,072 個方法範例如 {{list:generated_method_example}} 所示。
CODE START csharp | 合成基準測試所產生的方法範例 {{list:generated_method_example}}  
public static int M2(int sel, int x, int\[\] buf)  
{  
    x \= Core.Mix(x, 655663086);      // 跨邊界呼叫共用組件  
    x \+= buf\[Core.Idx(x, 7)\];        // 含邊界檢查的陣列存取（Idx 以 & 1023 遮罩）  
    return M3(sel, x, buf);          // 小函式鏈  
}  
CODE END

#### 5.3.2 2 個工作負載

表: IL2CPP ThinLTO 的合成基準測試工作負載 {{table:il2cpp_workloads}}  
| 工作負載 | 測量內容 |  
|---|---|  
| direct | 跨組件的靜態呼叫鏈（LTO 應最有效的形式） |  
| iface | 透過介面分派執行同類運算（LTO 難以將被呼叫端去虛擬化的形式） |  
兩者都只在 \`libil2cpp.so\` 內完成。各配置產生的二進位檔，  
每次都以從 APK 擷取的 \`libil2cpp.so\` 的 md5 確認。  
執行時會調整迭代次數，使每個樣本約為 100ms。由於每次執行的次數不同，會正規化為每 1 pass 的時間後再比較。

在 iface 中，會在每個組件各產生 4 個實作 \`IGenOp\` 的類別（共 64 個），基準測試透過介面依序呼叫（{{list:generated_iface_example}}）。方法本體與 direct 一樣呼叫 \`Core.Mix\` / \`Core.Idx\`，但呼叫的是哪個類別的 \`Apply\` 則在執行階段才決定。

{{pagebreak}}

CODE START csharp | iface 工作負載所產生的類別與呼叫迴圈 {{list:generated_iface_example}}  
sealed class Op0 : IGenOp  
{  
    int m\_State \= 20612;  
    public int Apply(int x, int\[\] buf)  
    {  
        x \= Core.Mix(x, 347275849);             // 跨邊界呼叫共用組件  
        x \+= buf\[Core.Idx(x ^ m\_State, 7)\];   // 與 direct 同類的陣列存取  
        m\_State \= (m\_State \+ x) | 1;  
        return x;  
    }  
}  
  
// 基準測試端：透過介面巡迴 64 個實作  
for (int i \= 0; i < ops.Count; i++)  
    a \+= ops\[i\].Apply(a ^ p, buf);       // 被呼叫端在執行階段才決定  
CODE END

### 5.4 CPU 實測（Pixel 8a）

表: Pixel 8a 上 IL2CPP ThinLTO 的 CPU 實測結果 {{table:il2cpp_cpu_results}}  
| 配置 | direct (us/pass) | iface (us/pass) |  
|---|---|---|  
| off(Release, \-Os) | 19.88 | 0.404 |  
| thin（手動 ThinLTO） | 15.54 (-21.8%) | 0.375 (-7.3%) |  
透過手動 ThinLTO，direct 縮短了 21.8%，iface 縮短了 7.3%。

#### direct 的函式呼叫與指令數

針對與 §5.3.1 相同的呼叫結構，比較了有無 LTO 的反組譯結果。
表: direct 工作負載的反組譯比較 {{table:direct_disassembly}}  
| \`C000\_M2\` | 無 LTO | ThinLTO |  
|---|---:|---:|  
| 指令數 | 15 | 11 |  
| 函式呼叫（\`bl\`） | 2 | 0 |  
| 堆疊框架 | 有 | 無 |  
| callee-saved 暫存器的保存 | x19、x20、x29、x30 | 無 |  
| 記憶體存取 | 5 次 | 1 次 |  
\`Core.Mix\` 與 \`Core.Idx\` 的本體被展開到呼叫端，不再需要 2 個 \`bl\` 指令以及堆疊與暫存器的保存處理。橫跨兩個函式的運算式也會一併最佳化。direct 的 21.8% 縮短，是這項變化在 3,072 個方法上累積的結果。

#### iface 改善較小的原因

IL2CPP 產生的 interface 呼叫，概略為 {{list:interface_invoke_code}} 的形式。  
CODE START cpp | IL2CPP 產生的 interface 呼叫程式碼概略 {{list:interface_invoke_code}}  
const VirtualInvokeData& invokeData \=  
    il2cpp\_codegen\_get\_interface\_invoke\_data(slot, obj, declaringInterface);  
return ((Func)invokeData.methodPtr)(obj, p1, p2, invokeData.method);  
CODE END  
由於被呼叫端是在執行階段取得的 \`invokeData.methodPtr\`，無法像一般 C++ vtable 呼叫那樣去虛擬化。不過，方法本體呼叫其他 TU 輔助函式的部分仍可內聯，因此 iface 也縮短了 7.3%。

若 interface / virtual 呼叫耗費較多時間，也可考慮 \`sealed\`、以具體型別接收、以泛型進行靜態分派。ThinLTO 容易改善的，是靜態呼叫其他 TU 小函式的部分。

### 5.5 大小與建置時間

表: IL2CPP ThinLTO 的大小與建置時間 {{table:il2cpp_size_build_time}}  
| 配置 | libil2cpp.so (MB) | 同 .text | 冷建置 |  
|---|---|---|---|  
| off | 37.36 | 基準 | 62s |  
| thin | 38.16 (\+2.2%) | \+2.3% | 71s (+15%) |  
在此配置中，受內聯展開影響，\`libil2cpp.so\` 增加了 2.2%。  
LTO 有時也會移除不需要的程式碼，因此大小變化的方向取決於程式碼庫與最佳化內容。
冷建置從 62s 增加到 71s，增加 15%。ThinLTO 會在連結時進行跨 TU 最佳化與機器碼產生，因此連結時間會相應延長。

## 6\. Master 配置所啟用的最佳化

在 Master 配置中，LTO 與最佳化等級會同時改變（{{table:compiler_configuration_flags}}）。為了釐清 CPU 時間的縮短來自哪個設定，分別進行了測量。
為了個別調查最佳化等級（\`-Os\` → \`-O2\`）與 LTO 的影響，實測了 {-Os, \-O2} × {無 LTO, ThinLTO} 的 2×2 組合（Android 6000.5 / arm64）。  
表: 最佳化等級與 ThinLTO 的 CPU 實測比較 {{table:master_cpu_matrix}}  
| direct (us/pass) | 無 LTO | ThinLTO |  
|---|---|---|  
| \-Os（Release） | 19.64（基準） | 15.54 (-20.9%) |  
| \-O2(Master) | 18.47 (-5.9%) | 15.54 (-20.9%) |  
表: 最佳化等級與 ThinLTO 的 libil2cpp.so 大小比較 {{table:master_size_matrix}}  
| libil2cpp.so | 無 LTO | ThinLTO |  
|---|---|---|  
| \-Os | 37.36MB | 38.16MB (+2.2%) |  
| \-O2 | 39.14MB | 44.14MB (\+12.8%) |  
CPU 時間的縮短率在僅 \`-O2\` 時為 5.9%，在包含 ThinLTO 的配置中為 20.9%。在此基準測試中，Master 配置的改善大部分來自 ThinLTO。  
Release \+ 手動 ThinLTO 與 Master 的 CPU 時間同為 15.54us，  
但最終 \`libil2cpp.so\` 的大小分別為 38.16MB 與 44.14MB，Release \+ 手動 ThinLTO 小了約 13.5%。  
表: Release \+ ThinLTO 與 Master 的冷建置時間 {{table:master_build_time}}  
| 配置 | 冷建置 |  
|---|---:|  
| Release（\`-Os\`、無 LTO） | 62s |  
| Release \+ ThinLTO | 71s (+15%) |  
| Master (\`-O2\` \+ ThinLTO) | 86s (+39%) |  
冷建置分別為 71s 與 86s，僅在 Release 加入 ThinLTO 的配置短了 15 秒。
在 Unity 的公開測量中，Master 配置（\`libil2cpp.so\` 側）也是 CPU \-2.3%、  
到初次顯示的時間 \-5.7%，相對地 \`libil2cpp.so\` 大小 \+18.7%、建置時間 \+30〜35%。  
CPU 的改善幅度，相對於 Unity 的 \-2.3%，本章的基準測試為 \-20.9%。  
不過，Unity 測量的是實際專案的整個主執行緒，本章只測量反覆呼叫小函式的路徑，  
因此無法直接比較數字。  
另一方面，大小與建置時間在本章的 Master 中也分別為 \+18.2%、\+38.7%，與 Unity 的公開值相近。  

{{pagebreak}}

## 7\. 實際專案的驗證

也在規模比基準測試更大的 2 個實際專案中嘗試了 ThinLTO。Unity 版本、組件數、原始碼量與二進位檔大小如 {{table:title_scale}} 所示。
表: 實際專案的 Unity 版本與程式碼規模 {{table:title_scale}}  
| 項目 | 專案 A | 專案 B |  
|---|---:|---:|  
| Unity 版本 | 6000.5.3f1 | 6000.3.15f1 |  
| 進入 Player 的組件數 | 158 | 285 |  
| 編譯的 \`.cs\` | 6,969 個檔案 | 16,700 個檔案 |  
| 同 行數 | 約 138 萬行 | 約 319 萬行 |  
| 其中自家程式碼 | 1,943 個檔案 / 約 35 萬行 | 8,606 個檔案 / 約 154 萬行 |  
| \`libil2cpp.so\`（未壓縮） | 189.0MB | 294.3MB |  

### 7.1 Release \+ ThinLTO 的大小與套用確認

表: 專案 A 的大小與建置時間 {{table:title_a_size_build_time}}  
| 項目 | baseline | ThinLTO | 差異 |  
|---|---|---|---|  
| libil2cpp.so（未壓縮） | 189.01MB | 194.70MB | \+5.69MB (+3.0%) |  
| 同 \`.text\` | 8.54MB | 8.98MB | \+0.44MB (+5.1%) |  
| APK | 165.85MB | 166.94MB | \+1.08MB (+0.65%) |  
| Player 建置部分 | 約 396s | 約 433s | 約 \+9% |  
\`.text\` \+5.1% 大於基準測試（+2.3%），符合 asmdef 分得越細、跨模組內聯越多的預期。  
即使是 189MB 的 libil2cpp.so，也能在本機完成 ThinLTO 的連結。

{{pagebreak}}

在專案 B 中，以 \`llvm-readelf \-S\` 比較了 \`libil2cpp.so\` 各節的大小（{{table:title_b_sections}}）。  
表: 專案 B 的 libil2cpp.so 各節差異 {{table:title_b_sections}}  
| 項目 | baseline | ThinLTO | 差異 |  
|---|---|---|---|  
| libil2cpp.so（整個檔案） | 294.30MB | 304.12MB | \+3.34% |  
| \`il2cpp\` | 145.85MB | 162.22MB | \+11.23% |  
| \`.text\` | 10.69MB | 11.14MB | \+4.18% |  
| \`.rela.dyn\` | 50.28MB | 45.01MB | \-10.48% |  
| \`.got\` | 1.76MB | 1.4KB | \-99.92% |  
| \`.gcc\_except\_table\` | 3.70MB | 3.62MB | \-2.32% |  
\`.text\` 的增加率 4.18%，介於專案 A（5.1%）與基準測試（2.3%）之間。\`.got\` 從 1.76MB 減少到 1.4KB。這表示 LTO 解析了跨模組的參照，將經由 GOT 的間接參照轉換為直接參照。包含動態重定位項目的 \`.rela.dyn\` 削減 10.5%，也對應這項變化。

### 7.2 實際專案中的 Master 與 full LTO（專案 A）

以實際專案的規模，嘗試了 §6 中 Master 配置與 full LTO 的比較。  
表: 專案 A 的建置配置比較 {{table:title_a_configuration_comparison}}  
| 專案 A | baseline | Release \+ thin | Master | full LTO |  
|---|---|---|---|---|  
| libil2cpp.so | 189.0MB | 194.7MB (+3.0%) | 222.2MB (+17.6%) | — |  
| 建置 | 453s | 465s | 537s（9 分鐘） | 2,903s（48 分鐘）後失敗 |  
| 連結器路徑的尖峰 RSS | — | — | 12.9GB | 20GB → \`Killed: 9\`（OOM） |  

full LTO 在建置 48 分鐘後，連結期間的記憶體使用量達到 20GB，被 OS 強制終止。在 §5.3 的基準測試中，full LTO 的 CPU 時間也與 ThinLTO 相當，但冷建置從 71s 增加到 247s，約 3.5 倍。此專案無法完成建置，ThinLTO 更為實用。

Master 的 \`libil2cpp.so\` 大小比 baseline 增加約 33MB，但 CPU 效能與 Release + thin 相當。Release + thin 的大小增加控制在約 5.7MB，因此在此專案中也能以更小的二進位檔獲得與 Master 相當的效能。

### 7.3 實際專案的 CPU 週期（專案 B）

在局外（out-game）的穩態畫面中，加總 Pixel 8a 所有執行緒的 CPU 週期進行測量。ThinLTO 與 baseline 交替各測量 3 個工作階段，各工作階段的值為 6 次 20 秒測量的平均。  
表: 專案 B 各工作階段的 CPU 週期 {{table:title_b_sessions}}  
| 工作階段 | 總週期 (Gcyc) | UnityMain (Gcyc) |  
|---|---|---|  
| ThinLTO \#1 | 44.88 | 27.48 |  
| ThinLTO \#2 | 45.18 | 27.36 |  
| ThinLTO \#3 | 45.54 | 27.60 |  
| baseline \#1 | 46.06 | 29.05 |  
| baseline \#2 | 45.71 | 27.89 |  
| baseline \#3 | 46.15 | 28.23 |  
| 平均（ThinLTO vs baseline） | 45.20 vs 45.97 → \-1.68% | 27.48 vs 28.39 → \-3.21% |

### 7.4 與 Unity 公開值的比較

將 Unity 公開的 Master 配置（\`libil2cpp.so\` 側）測量值與專案 B 的結果比較，並一併列出 §6 的基準測試結果。
表: Unity 公開值與本章實測結果的比較 {{table:unity_public_comparison}}  
| 指標 | Unity 公開（6 個專案、Master） | 基準測試 | 專案 B |  
|---|---|---|---|  
| CPU | 主執行緒 \-2.3% | \`direct\` \-21.8% | UnityMain \-3.21% / 總週期 \-1.68% |  
| \`libil2cpp.so\` | \+18.7% | \+2.2%(Release+thin)/ \+18.2%(Master) | \+3.0% |  
| 建置時間 | \+30〜35% | \+15%（Release+thin）/ \+38.7%（Master） | \+9〜15% |  
Unity 的 CPU 時間縮短率 \-2.3% 與專案 B 的 UnityMain \-3.21%，都是數個百分點的改善。與基準測試 \-21.8% 的差距，可由實際專案的測量包含不在 LTO 範圍內的處理來解釋。Unity 測量的 6 個專案也呈現同樣的趨勢。

大小與建置時間則依配置而有差異。Unity 的公開值在 Master 配置下分別為 \+18.7% / \+30〜35%。與本章測得的 Master 配置 \+18.2%（44.14MB / 37.36MB）/ \+38.7%（86s / 62s）也很接近。作為基準的 \`libil2cpp.so\` 大小也相近，為 37.26MB / 37.36MB。

若僅在 Release 加入 ThinLTO，則能以大小 \+2〜3%、建置時間 \+9〜15% 獲得同等的 CPU 效能。從 §6 的 2×2 比較可推測，Master 配置中大小與建置時間的增加，主要來自改為 \`\-O2\`。

### 7.5 最佳化的適用範圍與實際專案的改善幅度

專案 B 的改善集中在主執行緒。UnityMain 為 \-3.21%，而非 UnityMain 的整體為 \+0.8%，在工作階段之間 4% 的漂移範圍內。

在二進位檔方面，\`libil2cpp.so\` 以外的 \`.so\` 幾乎沒有變化。2 個原生外掛程式（native plugin）的 md5 完全一致，剩下 1 個的 \`.text\` 差異為 \+0.008%，在建置非確定性的範圍內。本次以 \`--compiler-flags\` 進行的最佳化並未及於這些外掛程式。

UnityMain 的 \-3.2% 小於基準測試的 \-22%。在此畫面中，表情、眨眼、頭髮與服裝的晃動、IK 等會以 Job 執行。這些包含在 \`libil2cpp.so\` 之外的原生二進位檔中，不在本次旗標最佳化的範圍內。

排除範圍外的處理後，僅 IL2CPP 產生程式碼的改善幅度應大於 UnityMain 整體的 \-3.21%。不過，要求得精確值，需要以 Profiler 分離範圍內外的處理時間。由於 Job 依排程也可能在主執行緒執行，UnityMain 的測量值同樣包含範圍外的處理。

{{pagebreak}}

## 8\. iOS 的 ThinLTO 套用

也在專案 B 的 iOS 版套用了 ThinLTO。在驗證用基準測試中可確認效果，但在實際專案中，未能獲得與建置時間增加相稱的改善。

### 8.1 iOS 上 LTO 的結構

表: Android 與 iOS 的 LTO 結構比較 {{table:android_ios_lto_structure}}  
| 觀點 | Android | iOS |  
|---|---|---|  
| il2cpp 的輸出 | \`libil2cpp.so\` | \`libGameAssembly.a\` |  
| 最終連結 | il2cpp 呼叫的 LLD | Xcode 建立 \`UnityFramework\` 時的 ld64 |  
| 傳入的引數 | \`--compiler-flags=-flto=thin \--linker-flags=-flto=thin\` | 僅 \`\--compiler-flags=-flto=thin\` |  
| 建置時間增加的位置 | Unity 的建置步驟 | \`Build ipa\`（xcodebuild）步驟 |  

在 iOS 上，il2cpp 只負責建立封存檔，因此不會傳入 \`--linker-flags\`。LTO 的程式碼產生，是在 Xcode 端讀取含有此 bitcode 的封存檔並連結時進行。

由於產生程式碼的編譯由 il2cpp 負責，旗標必須指定在 \`AdditionalIl2CppArgs\`。即使變更 Xcode 的 \`LLVM\_LTO = YES\_THIN\`，也不會套用到產生的程式碼。

引擎程式碼以預先建置的 \`libiPhone-lib.a\` 形式提供，不含 bitcode，因此不在 LTO 範圍內。也沒有像 Android 的引擎程式碼 LTO 開關（§3）那樣，可換成已套用 LTO 引擎的設定。

{{pagebreak}}

### 8.2 iPhone 13 的驗證用基準測試

首先，將與 §5.3 相同的驗證用基準測試移植到 iOS，確認對產生程式碼的 ThinLTO 在 iOS 上是否也能發揮作用。\`direct\` 會跨越分散在 16 個 asmdef 中的 3,072 個小方法  
進行呼叫，是 ThinLTO 最容易發揮效果的配置。因此，這裡的改善率  
並非一般專案可直接獲得的值，而應視為條件符合時接近上限的值。

裝置為 iPhone 13（\`iPhone14,5\` / iOS 18.7.8）。使用 Unity 6000.5.3f1、IL2CPP Release、OptimizeSpeed。baseline 與 ThinLTO 的差異只有是否加入 \`--compiler-flags=-flto=thin\`。Xcode 為 \`ReleaseForRunning\`，開發簽章為 \`get-task-allow=true\`，並停用 Unity 的 Development Build。

每個工作階段都會解除安裝、重新安裝並啟動，經 5 次預熱後記錄 30 個樣本。測量順序為 baseline / ThinLTO / ThinLTO / baseline 共 3 個區塊，合計 12 個工作階段。所有工作階段的熱狀態皆為 Nominal。由於處理次數會依工作階段自動調整，\`direct\` 與 \`iface\` 正規化為每 1 pass，Physics 正規化為每 1 step。

表: iPhone 13 的驗證用基準測試結果 {{table:ios_benchmark_results}}  
| 工作負載 | baseline | ThinLTO | 差異 |  
|---|---:|---:|---:|  
| \`direct\` | 21,231.31ns/pass | 16,184.31ns/pass | \-23.77% |  
| \`iface\` | 366.96ns/pass | 370.39ns/pass | \+0.93% |  
| Physics（範圍外的對照） | 147.82us/step | 147.36us/step | \-0.31%（分布重疊） |  
\`direct\` 在 3 個區塊中皆為 \-23.6〜-23.9%，baseline 與 ThinLTO 的所有工作階段  
完全分離。與 Android 的 \-21.8%（§5.4）也相近，可確認對產生程式碼的 ThinLTO 在 iOS 上  
也確實發揮作用。另一方面，\`iface\` 略微惡化，範圍外的 Physics 則  
沒有出現差異。ThinLTO 的效果偏向於靜態小函式呼叫跨越 TU 的路徑。  
同一基準測試的二進位檔大小如 {{table:ios_benchmark_size}} 所示。  
表: iPhone 13 驗證用基準測試的二進位檔大小 {{table:ios_benchmark_size}}  
| 項目 | baseline | ThinLTO | 差異 |  
|---|---:|---:|---:|  
| UnityFramework | 98.76MB | 107.52MB | \+8.76MB(+8.87%) |  
| \`\_\_text\` | 20.09MB | 20.46MB | \+0.37MB(+1.83%) |  
| \`il2cpp\` | 22.49MB | 30.78MB | \+8.30MB(+36.89%) |

### 8.3 專案 B 的結果與 iOS 上不採用的理由

接著，在專案 B 的 iOS 版套用了 ThinLTO。UnityFramework 的大小如 {{table:ios_sections}}、建置時間如 {{table:ios_build_time}} 所示。  
表: iOS UnityFramework 的各節差異 {{table:ios_sections}}  
| 節 | baseline | ThinLTO | 差異 |  
|---|---|---|---|  
| UnityFramework（整個檔案） | 316.53MB | 393.47MB | \+24.31% |  
| \`\_\_TEXT\`（段） | 261.83MB | 338.53MB | \+29.29% |  
| \`il2cpp\` | 167.02MB | 240.26MB | \+43.85% |  
| \`\_\_text\` | 57.99MB | 61.70MB | \+6.41% |  
| \`\_\_gcc\_except\_tab\` | 6.85MB | 6.63MB | \-3.16% |  
| \`\_\_unwind\_info\` | 3.44MB | 3.38MB | \-1.75% |  
| \`\_\_got\` | 18.8KB | 18.8KB | ±0 |  
| ipa（整個封存檔） | 233.83MB | 246.93MB | \+5.60% |  
表: iOS 的建置時間 {{table:ios_build_time}}  
| 步驟 | baseline | ThinLTO |  
|---|---|---|  
| \`Build ipa\`(xcodebuild) | 357s(5m57s) | 3,094s(51m34s) |  
UnityFramework 增加 \+24.31%，ipa 增加 \+5.60%。建置時間約為 9 倍，增加了 46 分鐘。增加的大部分來自 Xcode 端的連結與 \`dsymutil\`，相較於 Android 的 \+9〜15% 大幅拉長。即使將 \`GCC\_OPTIMIZATION\_LEVEL\` 降為 \`-Os\` 的配置（\`-Os\` + thin）也需要 53m51s，無法縮短。

在專案 B 的穩態畫面中，\`cpu/wall\` 的簡單平均在 ThinLTO 下為 \+1.08%，略微惡化；即使在熱狀態一致的組合中，也只有 \-1.45% 的改善。baseline 與 ThinLTO 的分布重疊，無法分離出明顯的 CPU 改善。記憶體差異 \+0.40% 也低於工作階段內的變動。驗證用基準測試的 \-23.77% 是集中了對 ThinLTO 有利的呼叫時的值，並不適用於整個實際專案。

由於在實際專案中無法確認明確的改善，且冷建置與 CI 每次都需要額外約 46 分鐘，本次驗證不採用 iOS 的手動 ThinLTO。

## 9\. 如何導入自己的專案

在 Android 上，建議先採用在 Release 配置中加入 \`\-flto=thin\` 的方法。在本次基準測試中，以更小的二進位檔與更短的建置時間，獲得了與 Master 相當的 CPU 效能。在實際專案的穩態畫面中，總週期也減少了 1.68%，UnityMain 減少了 3.21%（§7.3）。

引擎程式碼 LTO 開關（§3）因停用 Strip Engine Code 造成的大小與記憶體增加較大，在本次配置下不建議採用。

iOS 的手動 ThinLTO 沒有與建置時間增加相稱的改善，因此不採用。

採用前，請先以 Release 建置的 Profiler 擷取結果，調查 PlayerLoop 內的腳本執行時間。手動旗標的套用範圍在 \`libil2cpp.so\` 內，渲染執行緒、GPU、引擎內部與 GC 不在範圍內。以另一條 AOT 管線編譯的 Job 也不包含在內（§7.5）。大量使用 DOTS/Job 的專案，也要確認其處理時間。

例如在 33ms 的影格中腳本占 8ms，即使該部分縮短約 20%，整個影格也只縮短約 1.6ms。只有在腳本占整個影格的比例大、有改善空間，或必須維持高影格率的情況下，才值得考慮導入。

若建置時間的增加影響開發中的作業，也可以不套用於日常的開發建置，只套用於 staging 之後的建置。由於旗標是在建置前處理中加入（{{list:manual_thinlto_build}}），可依建置種類切換。

設定後，依 §3.1 與 §5.2 的步驟確認是否套用。確認處為 inputdata、二進位檔的 md5、編譯指令的 \`-flto\`，以及目的檔的 bitcode。即使未滿足 LTO 的條件，建置仍可能成功，因此無法僅憑建置結果判斷是否已套用。

脚注: {{fn:unity_lto}} Unity Android 平台團隊「Boosting Android performance with LTO optimizations in Unity 6.5」\<https://discussions.unity.com/t/boosting-android-performance-with-lto-optimizations-in-unity-6-5/1720008\>。以下本章所稱的「Unity 的公開值」即為此文章的數字。

[image1]: assets/kim-minhyuk-1/media/image1.png

[image2]: assets/kim-minhyuk-1/media/image2.png


[image4]: assets/kim-minhyuk-1/media/image4.png
