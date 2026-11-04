# **運用 Renderer Shader User Value 進行圖形最佳化**

Noboru Seto / 株式會社 CyberAgent SGE 核心技術本部  
[https://github.com/ruccho](https://github.com/ruccho)  
---

# **前言**

Unity 6.3 新增了名為 Renderer Shader User Value（RSUV）的功能。  
RSUV 是可針對每個 Renderer 元件設定的 32 位元整數值，也可以在著色器中參照。Unity 6000.3.0f1 以後，MeshRenderer、SkinnedMeshRenderer 可使用 RSUV；Unity 6000.5.0f1、6000.4.4f1、6000.3.13f1 以後也支援 SpriteRenderer{{fn:sprite_renderer}}、SpriteShapeRenderer、TilemapRenderer。用法很簡單，只要以 Renderer.SetShaderUserValue() 對 Renderer 設定 RSUV，並在著色器中透過 unity\_RendererUserValue 參照即可。

CODE START csharp  
uint value \= /\* ... \*/;  
meshRenderer.SetShaderUserValue(value);  
value \= meshRenderer.GetShaderUserValue();  
CODE END

CODE START c  
uint value \= unity\_RendererUserValue;  
CODE END

此範例的效果同樣可以用材質參數實現。不過使用 RSUV 時，**尤其在大量繪製的情境下，記憶體配置量與影格率都有可能比材質更好**。本章將說明 RSUV 出現的背景與有效的使用方式，並介紹效能測量結果。

## **Unity 的大量繪製功能與 RSUV 出現的背景**

首先回顧一下為何需要 RSUV。  
在 Unity 中同時顯示大量物件時的效能一直是令人頭痛的問題。大量繪製的效能問題主要有三點：「切換材質的 CPU 負載」、「DrawCall 增加」、「複製材質造成的記憶體配置」。

### **切換材質的 CPU 負載**

繪製多個持有不同材質的物件時，每次繪製影格都必須逐一收集各物件的材質資料並送往 GPU，此時的 CPU 處理成本很高。SRP Batcher 解決了這個問題，它將材質資料常駐於 GPU 記憶體中，只要著色器相同，就能以相對較低的成本繪製多個持有不同材質的物件。

### **DrawCall 增加**

GPU Instancing 是將多個擁有相同著色器、相同網格、相同材質的物件以一次 DrawCall 繪製的方法。它的適用條件比 SRP Batcher 嚴格，但在繪製大量物件時能發揮超越 SRP Batcher 的效果。Unity 6 推出 GPU Resident Drawer 後，即使是使用一般 MeshRenderer 的繪製，只要符合條件也會自動進行 GPU Instancing。

### **複製材質造成的記憶體配置**

最後一個問題是：想在使用 SRP Batcher 或 GPU Instancing 的同時，也讓材質帶有變化的情境。Unity 很早就有 MaterialPropertyBlock 這項功能，能夠不複製材質實例而只覆寫部分參數，但它與 SRP Batcher 和 GPU Resident Drawer 並不相容。RSUV 解決了這個問題，可以在使用 SRP Batcher 和 GPU Resident Drawer 的同時，**不透過材質**就將每個物件各自不同的 32 位元參數傳給著色器。

RSUV 是同時解決上述三個問題所需的最後一塊拼圖。既能以 SRP Batcher 壓低切換材質的成本，又能以 GPU Resident Drawer 進行高效率的 GPU Instancing，同時還能讓每個實例呈現不同的繪製變化。即使在 GPU Resident Drawer 的自動 Instancing 無法生效的情況下，也能避免複製材質實例，獲得抑制記憶體消耗的好處。

# **簡單的用法**

實際來看 RSUV 的用法。RSUV 在 C\# / HLSL 中以 32 位元整數（uint）表示，因此若要傳遞例如 32 位元的顏色（RGBA 4 個 channel 各 8 位元），需如下以位元運算進行打包。

CODE START csharp  
Color32 color \= /\* ... \*/;  
var colorEncoded \= (uint)(color.r \<\< 24 | color.g \<\< 16 | color.b \<\< 8 | color.a);  
meshRenderer.SetShaderUserValue(colorEncoded);  
CODE END

接著是著色器端的實作，此處假設使用 ShaderGraph。ShaderGraph 並未提供取得 RSUV 的專用節點，因此使用 Custom Function 節點，依下列方式設定（{{fig:custom_node}}）並輸入程式碼。

![][image1]{scale=0.75}  
図: 節點的設定 {{fig:custom_node}}

CODE START c  
uint rsuv \= unity\_RendererUserValue;  
Value \= float4((float)((rsuv \>\> 24\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 16\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 8\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 0\) & 255\) \* (1.f / 255.f));  
CODE END

連接節點，確認已能接收到 RSUV。

# **效能測量**

為了確認 RSUV 在大量繪製的情境下能發揮效果，以下列條件測量了記憶體配置量與影格率。

* Unity 6000.3.15f1  
* URP 17.3  
* iPhone 12  
* 場景：10000 個 Sphere  
* 啟用 SRP Batcher  
* 比較每影格參數有變化與無變化的情況  
* 比較啟用與停用 GPU Resident Drawer 的情況  
* 以 3 種方法為每個 Sphere 設定不同的著色器參數  
  * 每個 Renderer 一個材質：建立並指派 10000 個 Material  
  * RSUV：使用 1 個 Material，為每個 Renderer 設定 RSUV  
  * MaterialPropertyBlock：1 個 Material 與 10000 個 MaterialPropertyBlock（不在 SRP Batcher / GPU Resident Drawer 的適用範圍內）

![][image2]  
図: 基準測試畫面 {{fig:benchmark}}

{{pagebreak}}

首先比較各種參數設定方法的記憶體配置量（{{table:material_memory}}）。

表: 記憶體增量 {{table:material_memory}}

| 條件 | 記憶體增量 |
| :---- | :---- |
| 每個 Renderer 一個材質 | 15.79 MB |
| MaterialPropertyBlock | 3.32 MB |
| RSUV | 1.66 KB |

由於不會複製材質，記憶體配置量理所當然地大幅減少。  
接著來看影格率。首先是每影格參數不變的情況（{{table:frame_time}}）。

表: 每影格參數不變時的影格率（FPS）{{table:frame_time}}

| 條件 | 無 GPU Resident Drawer | 有 GPU Resident Drawer |
| :---- | :---- | :---- |
| 每個 Renderer 一個材質 | 33.8 | 38.8 |
| RSUV | 49.9 | 59.8 |
| MaterialPropertyBlock | 28.9 | 28.9 |

由此可知，RSUV 的效能高於每個 Renderer 一個材質的做法，而且搭配 GPU Resident Drawer 後效果更為明顯。單獨使用 RSUV 即可削減切換材質的成本，再加上能以 GPU Resident Drawer 進行 GPU Instancing，渲染執行緒上發出繪製命令的負載也得以減輕。  
接著來看每影格參數會變化的情況（{{table:frame_time_variance}}）。

表: 每影格參數變化時的影格率（FPS）{{table:frame_time_variance}}

| 條件 | 無 GPU Resident Drawer | 有 GPU Resident Drawer |
| :---- | :---- | :---- |
| 每個 Renderer 一個材質 | 11.1 | 8.1 |
| RSUV | 48.0 | 60.0 |
| MaterialPropertyBlock | 26.1 | 25.2 |

與沒有參數更新的情況相比，每個 Renderer 一個材質的做法效能大幅下降，而 RSUV 幾乎沒有劣化。變更 1 個材質參數就必須將整個材質重新上傳至 GPU，相對地，RSUV 是作為每影格、每個 Renderer 都一定會送往 GPU 的緩衝區的一部分傳送{{fn:instanced_rsuv}}，因此更新 RSUV 所額外產生的成本可以壓得很低。有趣的是，大量更新材質參數與 GPU Resident Drawer 似乎搭配不佳，效能反而比停用 GPU Resident Drawer 時更低。  
如上所述，可以看出 RSUV 在「參數更新」與「發出繪製命令」兩方面都較為有利。特別是在 RSUV 使 GPU Instancing 成為可能的情況下（例如存在大量著色器與網格相同的物件），RSUV 的效果會大幅提升。

# **傳遞大於 32 位元的資料**

RSUV 雖然有助於提升效能，但只能傳遞 32 位元以內的值是它的一大弱點。以下介紹克服此限制的技巧。  
要用 RSUV 傳遞 32 位元放不下的資料，有效的做法是將目標資料存放在另一個 GraphicsBuffer 中，RSUV 則傳遞該 GraphicsBuffer 中的索引。這個方法會因上傳 GraphicsBuffer 而產生額外成本，因此對 GraphicsBuffer 的更新頻率與大小可能需要有一定的考量。

CODE START csharp  
\[StructLayout(LayoutKind.Sequential)\]  
struct VariantGpuData  
{  
    public Vector4 baseColor;  
    public Vector4 surface;  
}  
private static readonly int VariantBufferId \= Shader.PropertyToID("\_RsuvVariantBuffer");

// 上傳並配置 GraphicsBuffer  
var buffer \=  new GraphicsBuffer(GraphicsBuffer.Target.Structured, count, UnsafeUtility.SizeOf\<VariantGpuData\>());  
var cpuBuffer \= new VariantGpuData\[count\];  
FillCpuBufferValues(cpuBuffer);  
buffer.SetData(cpuBuffer, 0, 0, count);  
Shader.SetGlobalBuffer(VariantBufferId, buffer);

// 設定 RSUV  
renderer.SetShaderUserValue(index);

// ...

// 不再需要時釋放  
buffer.Dispose();  
CODE END

CODE START c  
struct RsuvVariantRecord  
{  
    float4 baseColor;  
    float4 surface;  
};

StructuredBuffer\<RsuvVariantRecord\> \_RsuvVariantBuffer;

// 存取  
RsuvVariantRecord record \= \_RsuvVariantBuffer\[unity\_RendererUserValue\];  
CODE END

## **總結**

本章介紹了 RSUV 出現的背景、效能測量，以及基本與進階的用法。  
RSUV 與材質相比雖有 32 位元的限制，但視情況能為效能帶來大幅改善。請看清 RSUV 能發揮效果的情境，並善加運用。

脚注: {{fn:sprite_renderer}} SpriteRenderer 的 RSUV 取得與設定需要 2D Module。  
脚注: {{fn:instanced_rsuv}} 僅限未進行 Instancing 的情況。進行 Instancing 時，會與其他實例參數一起透過 GraphicsBuffer 傳遞，且只在整體參數變更、GraphicsBuffer 變為 dirty 的時間點才上傳。  


[image1]: assets/seto-noboru-1/media/image1.png

[image2]: assets/seto-noboru-1/media/image2.png