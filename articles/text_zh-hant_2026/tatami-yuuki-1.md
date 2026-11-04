# **以 VAT 減輕骨骼動畫負載**

Tatami Yuuki/株式會社 CyberAgent SGE 核心技術本部

X:@tatamin0207

## **前言**

在 Unity 一般的骨骼動畫中，骨骼更新等播放用的計算是由 CPU 每影格執行的。因此，播放動畫的物件越多，CPU 負載就越高，成為影格率下降的原因。本章介紹 VAT，這是一種把執行階段的計算整個替換成預先計算，藉此減輕負載的手法。

## **什麼是 VAT**

VAT 是 Vertex Animation Texture 的縮寫，是先把動畫造成的頂點變形資訊烘焙到紋理中，再由頂點著色器讀取並播放的手法。紋理的每個像素各對應「某個頂點在某一影格的位置」，隨著播放時間移動讀取位置，網格看起來就會動起來（{{fig:vat_texture}}）。此外，為了確保精度，位置是以正規化到 0 到 1 的值儲存，並在頂點著色器中還原為實際座標。

![][image1]

図: VAT 紋理的機制。橫軸對應頂點，縱軸對應影格 {{fig:vat_texture}}

使用 VAT，可以把骨骼更新等執行階段的播放計算整個替換成預先計算。這是連 GPU Skinning 也做不到的效率提升。

另一方面，它有以下缺點。

* 需要準備 VAT 專用的資產  
* 紋理大小會隨頂點數與影格數增加  
* 由於需要精確的值，與紋理壓縮不相容，實際上只能以未壓縮方式處理  
* 動作在烘焙進紋理的時點就固定了，雖然透過一些技巧可以做到某種程度的控制，但不適合精細的控制

基於這些特性，VAT 特別適合讓大量物件播放固定動畫的情況。例如建築物的破壞效果、群眾、背景裝飾物等不需要個別控制的視覺效果（{{fig:vat_demo}}）。![][image2]

図: 以 VAT 播放的動畫（左：剛體，右：柔體） {{fig:vat_demo}}

## **效能驗證**

以下介紹在實機上將 VAT 與其他動畫方式比較的效能結果。

### **測量環境與比較條件**

測量環境如 {{table:test_env}} 所示。

表: 測量環境 {{table:test_env}}

| 項目 | 內容 |
| :---- | :---- |
| 實機 | Pixel 8a（GPU：Mali-G715） |
| 圖形 API | Vulkan |
| Unity 版本 | 6000.3.2f1 |
| 測量 API | FrameTimingManager、ProfilerRecorder |
| 模型 | 建築物倒塌動畫（85,209 個頂點、1,315 個碎片） |
| 動畫 | 300 影格 |
| 測量方法 | 物件數從 0 變化到 16 個，各條件取 300 影格 × 3 次的中位數 |

比較的 3 種方式如 {{table:test_conditions}} 所示。

表: 比較條件 {{table:test_conditions}}

| 方式 | 組成 | 啟用的最佳化 |
| :---- | :---- | :---- |
| FBX 方式 | 各碎片配置為個別的 Mesh Renderer，以 Animator 播放 | SRP Batcher |
| Skin 方式 | 1 個碎片對應 1 根骨骼，整合為 1 個 SkinnedMeshRenderer，以 Animator 播放 | SRP Batcher |
| VAT 方式 | 把所有碎片的動作烘焙到紋理，作為 1 個網格繪製（Rigid-Body Dynamics 模式） | GPU Instancing |

FBX 方式是各碎片各自擁有個別的 Mesh Renderer，並以 Animator 播放的組成（碎片是以 Transform 移動的剛體，不使用蒙皮）。Skin 方式是為每個碎片分配 1 根骨骼，整體彙整為 1 個 Skinned Mesh 的組成。1,315 個碎片直接成為 1,315 根骨骼，Renderer 只有 1 個。動畫播放由 CPU 的 Animator 負責，頂點變形則由 GPU Skinning 執行。VAT 方式是把所有碎片的動作烘焙到紋理，作為 1 個網格繪製。3 種方式的外觀完全相同。

繪製的效率化設定，是在各方式啟用對自己有利的設定的狀態下進行比較。由於 SkinnedMeshRenderer 不支援 GPU Instancing，Skin 方式能用的最佳選項是 SRP Batcher。此外，Skin 方式的骨骼化也需要專用的轉換管線，這一點與 VAT 的條件相同。

### **CPU 處理時間**

CPU 處理時間（主執行緒）的測量結果如 {{fig:cpu_time}} 所示。顯示 16 個時，FBX 方式增加到 154.3ms，而 Skin 方式為 16.5ms、VAT 方式為 12.6ms，幾乎沒有隨物件數增加。

![][image3]

図: 各物件數的 CPU 主執行緒處理時間 {{fig:cpu_time}}

細項如 {{table:cpu_breakdown}} 所示。FBX 方式與其他 2 種方式差距的最大因素是 Draw Call 的送出處理。FBX 方式每個模型有 1,315 個碎片需要繪製，而彙整為 1 個網格的 2 種方式則幾乎沒有這項處理。各碎片的包圍盒計算與剔除也是同樣情況。另一方面，Skin 方式與 VAT 方式的差距在於 Animator 的更新。Skin 方式由 CPU 負責播放，16 個時仍殘留 4.5ms，而 VAT 方式的播放本身移到了 GPU，因此這部分完全消失。這個差距會隨物件數擴大。

表: 顯示 16 個時的 CPU 處理時間細項（ms） {{table:cpu_breakdown}}

| 處理 | FBX 方式 | Skin 方式 | VAT 方式 |
| :---- | :---- | :---- | :---- |
| 主執行緒合計 | 154.30 | 16.53 | 12.56 |
| 其中 Draw Call 送出（SRPBatcher.Flush） | 76.17 | 0.03 | 0.02 |
| 其中 Animator 更新 | 5.30 | 4.54 | 0.02 |

### **GPU 處理時間**

GPU 處理時間的測量結果如 {{fig:gpu_time}} 所示。16 個時，相對於 FBX 方式的 77.1ms，Skin 方式以 22.2ms 最快，VAT 方式為 32.5ms。

![][image4]

図: 各物件數的 GPU 處理時間 {{fig:gpu_time}}

FBX 方式與其他 2 種方式差距的主因是繪製命令的分割數。FBX 方式的碎片在每個繪製通道都個別發出繪製命令，每影格達到約 21 萬次，而 VAT 方式彙整為實例化繪製，只有 99 次。Skin 方式每個物件只有 1 個 Renderer，因此也落在同一水準。繪製命令每次都有固定成本，分割數的差距會直接反映為處理時間的差距。

Skin 方式與 VAT 方式的差距來自變形計算的次數。GPU Skinning 每影格只計算 1 次變形，其結果由陰影、深度等所有繪製通道重複利用。相對地，VAT 每個通道都要在頂點著色器中重新從紋理讀取並計算變形。本次環境是陰影 Cascade 為 4 層、通道數較多的場景組成，因此與 Skin 方式的差距相應擴大（陰影貼圖繪製：FBX 方式 44.3ms、Skin 方式 7.4ms、VAT 方式 16.0ms）。在執行頂點計算的通道數較少的組成中，這個差距會縮小。

### **GPU 記憶體配置量**

GPU 記憶體的測量結果如 {{fig:gpu_memory}} 所示。數值是 OS 統計的圖形驅動程式實測值（GL mtrack）。從 0 個到 16 個的增量，FBX 方式為 +411.8MB（每個約 26MB），Skin 方式為 +38.2MB（每個約 2.4MB），VAT 方式幾乎為 ±0。

![][image5]

図: 各物件數的 GPU 記憶體配置量 {{fig:gpu_memory}}

這個差距來自「每增加 1 個物件時會增加什麼」的結構差異。FBX 方式會在 GPU 記憶體中配置與碎片數量相同的矩陣等繪製用資料。Skin 方式增加的只有每個物件 1 條用來寫出蒙皮變形結果的頂點緩衝區。VAT 方式只是當場計算變形，不持有新的緩衝區，因此不會增加。此外，VAT 的紋理未壓縮約為 6.6MB，但相對於增量的差距而言是足夠小的值。

### **驗證結果總結**

顯示 16 個時的實測值整理於 {{table:result_summary}}。

表: 顯示 16 個時的實測值總結 {{table:result_summary}}

| 指標 | FBX 方式 | Skin 方式 | VAT 方式 |
| :---- | :---- | :---- | :---- |
| CPU 主執行緒（ms） | 154.3 | 16.5 | 12.6 |
| GPU 影格（ms） | 77.1 | 22.2 | 32.5 |
| 繪製命令數（次/影格） | 約 210,000 | 與 VAT 方式同水準 | 99 |
| GPU 記憶體增量（0→16 個，MB） | \+411.8 | \+38.2 | 幾乎 ±0 |

相對於 FBX 方式，Skin 方式與 VAT 方式在所有指標上都大幅領先。原因可歸結為「能作為 1 個網格處理」。

Skin 方式與 VAT 方式的比較各有優劣，GPU 處理時間以 Skin 方式有利，CPU 主執行緒與記憶體則以 VAT 方式有利。VAT 方式的 GPU 處理時間，很大程度取決於通道組成是否需要大量頂點變形處理。因此，在執行頂點計算的通道數較少的組成中，或以削減 CPU 負載為目的時，VAT 會是第一候選。

此外，本次比較的題材是剛體破壞，這是容易以骨骼表現的題材。像布料或人體這類骨骼數會變多或只能近似處理的模型，以及物件數很多的情況，CPU 負載會變得顯著，因此可以認為 VAT 會更具優勢。

## **建立 VAT 資產**

建立 VAT 資產，使用 Houdini 附帶的 Labs Vertex Animation Textures ROP（以下稱 VAT ROP）是較簡便的方法{{fn:vat_rop_version}}。Houdini 是 SideFX 公司的 DCC 工具，強項在於程序化生成。VAT ROP 備有 Unity 用的 Preset 與對應的著色器，不僅可輸入 Houdini 內的模擬結果，也能以其他 DCC 工具製作的 FBX 動畫作為輸入。

VAT ROP 有 {{table:vat_modes}} 所列的 4 種模式。

表: VAT ROP 的模式 {{table:vat_modes}}

| 模式 | 用途 |
| :---- | :---- |
| Soft-Body Deformation | 基本的頂點變形。條件是拓撲不變 |
| Rigid-Body Dynamics | 針對剛體破壞最佳化。以碎片為單位記錄動作，因此可以把紋理大小壓得較小 |
| Dynamic Remeshing | 支援液體或撕裂的布料等拓撲會變化的模型 |
| Particle Sprites | 用於粒子模擬 |

步驟只需在 Input Geometry 指定要 VAT 化的動畫節點、在 Mode 指定要使用的模式、在 Target Engine 指定 Unity，然後按下 Render All 按鈕即可。{{table:vat_assets}} 所列的資產會一次輸出。

表: 播放 VAT 所需的資產 {{table:vat_assets}}

| 資產 | 作用 |
| :---- | :---- |
| 位置用紋理 | 各頂點在各影格的位置（正規化到 0〜1） |
| 旋轉用紋理 | 變形後的面的朝向。重建法線與切線以保持陰影 |
| 頂點色用紋理 | Houdini 端屬性的烘焙目標。可用於提升插值品質等的自由欄位 |
| FBX | UV 中嵌入了表示紋理參照位置的識別編號的網格 |
| 材質 | 已設定影格數、FPS、包圍盒等輔助資料 |
| 著色器 | 對應各模式的播放計算本體（需另外安裝） |

著色器不包含在輸出檔案中，因為計算處理與動畫無關、是固定的。需從 Unity 的 Package Manager 另外安裝 SideFX Vertex Animation Textures 套件（存放位置可在 VAT ROP 的 Real-Time Shaders 分頁確認）。套件除了支援 4 種模式的 Shader Graph 之外，還附有文件與匯入設定用的 Preset。

## **導入時的注意事項**

VAT 的精度不足時動畫會損壞，因此需要為紋理指定適當的格式。同樣地，FBX 端也有建議的匯入設定。請依照套件附帶的「VAT Import Settings Guide」，套用停用紋理壓縮等專用的匯入設定。

另外，官方的 Shader Graph 僅定位為範本，實際使用時需要自訂。不應直接使用範本，而是複製到專案用後再使用。不過，複製出來的 Shader Graph 仍會參照套件端的 Sub Shader Graph，請注意要把參照改為指向複製後的版本。

此外，輸出檔案的匯入設定，以及把紋理、著色器指派給材質等參照設定需要手動進行，是容易因設定遺漏而發生問題的部分。在需要量產的專案中，有餘地以 Scripted Importer 等方式把匯入設定與參照解析自動化。關於這方面的改良，請參閱上傳至 CEDiL 的資料{{fn:cedil}}。

## **總結**

VAT 是把動畫烘焙到紋理、在 GPU 上播放的手法。特點是能大幅削減 Draw Call，相對於每個碎片各持有 Renderer 的組成，與 Skin 方式同樣大幅提升了效率。兩者各有優劣，GPU 處理時間以 Skin 方式有利，CPU 處理時間與記憶體則以 VAT 方式有利。在執行頂點計算的通道數較少的組成中，或以削減 CPU 負載為目的時，VAT 會是第一候選。雖然有需要準備專用資產、動作固定等限制，但在讓大量物件播放固定動畫的場合，它是強力的選項。

脚注: {{fn:vat_rop_version}} 這是 SideFX Labs 套件中包含的工具。本章的驗證使用 3.0 版。

脚注: {{fn:cedil}}   
[https://cedil.cesa.or.jp/cedil\_sessions/view/3263](https://cedil.cesa.or.jp/cedil_sessions/view/3263)  


[image1]: assets/tatami-yuuki-1/media/image1.png

[image2]: assets/tatami-yuuki-1/media/image2.png

[image3]: assets/tatami-yuuki-1/media/image3.png

[image4]: assets/tatami-yuuki-1/media/image4.png

[image5]: assets/tatami-yuuki-1/media/image5.png