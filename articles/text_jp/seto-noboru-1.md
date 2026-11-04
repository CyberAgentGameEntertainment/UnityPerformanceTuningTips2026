# **Renderer Shader User Value を活用したグラフィックス最適化**

瀬戸 徳 / Noboru Seto / 株式会社サイバーエージェント SGEコア技術本部  
[https://github.com/ruccho](https://github.com/ruccho)  
---

# **はじめに**

Unity 6.3 より Renderer Shader User Value (RSUV) という機能が追加されました。  
RSUV は Renderer コンポーネントごとに設定できる32ビットの整数値で、シェーダーからも参照することができます。Unity 6000.3.0f1 以降では MeshRenderer, SkinnedMeshRenderer で RSUV を利用でき、Unity 6000.5.0f1, 6000.4.4f1, 6000.3.13f1 以降は SpriteRenderer{{fn:sprite_renderer}}, SpriteShapeRenderer, TilemapRenderer にも対応しています。使い方はシンプルで、 Renderer.SetShaderUserValue() でレンダラーに対して RSUV を設定し、シェーダーでは unity\_RendererUserValue を使って参照するだけです。

CODE START csharp  
uint value \= /\* ... \*/;  
meshRenderer.SetShaderUserValue(value);  
value \= meshRenderer.GetShaderUserValue();  
CODE END

CODE START c  
uint value \= unity\_RendererUserValue;  
CODE END

この例と同様のことはマテリアルパラメータを使っても実現できます。しかし、RSUV を使用すると、**特に大量描画系のシチュエーションでマテリアルよりもメモリ確保量やフレームレートを改善できる**可能性があります。この章では RSUV が登場した背景、RSUV の効果的な利用方法を説明し、パフォーマンス計測結果も紹介します。

## **Unityの大量描画機能とRSUVが登場した背景**

まずは RSUV が必要となった背景を振り返ってみます。  
Unity において大量のオブジェクトを同時に表示する際のパフォーマンスは常に頭痛の種でした。大量描画におけるパフォーマンスの問題は主に「マテリアル切り替えにおける CPU 負荷」「DrawCall の増大」「マテリアル複製によるメモリ確保」の3点が挙げられます。

### **マテリアル切り替えにおける CPU 負荷**

異なるマテリアルを持った複数のオブジェクトを描画する際、フレーム描画のたびにオブジェクトごとにマテリアルのデータを収集して GPU に送信する必要があり、この際の CPU 処理コストが大きい問題がありました。SRP Batcher はこの問題を解決し、マテリアルのデータをGPUメモリ上に永続化させることで、シェーダーが同一であれば比較的少ないコストで異なるマテリアルを持った複数のオブジェクトを描画できるようになりました。

### **DrawCall の増大**

GPU インスタンシングは、同じシェーダー・同じメッシュ・同じマテリアルを持った複数のオブジェクトを一つの DrawCall で描画する手法です。これは SRP Batcher より適用条件が厳しい代わりに、大量のオブジェクトを描画する際には SRP Batcher 以上の効果を発揮します。Unity 6 で GPU Resident Drawer が登場したことで、通常の MeshRenderer を使った描画でも条件を満たす場合には自動的に GPU インスタンシングが行われるようになりました。

### **マテリアル複製によるメモリ確保**

最後の問題は、SRP Batcher や GPU インスタンシングを利用しながら、マテリアルにも変化を持たせたいシチュエーションです。Unity には昔からマテリアルインスタンスを複製せずに一部のパラメータのみをオーバーライドする MaterialPropertyBlock という機能が存在しましたが、これは SRP Batcher や GPU Resident Drawer と互換性がありませんでした。RSUV はこの問題を解決し、SRP Batcher や GPU Resident Drawer を利用しながら、オブジェクトごとに異なる32ビットのパラメータを**マテリアルを使わずに**シェーダーに渡すことができます。

RSUV は上記の3つの問題を同時に解決するために必要だった最後のピースでした。SRP Batcher でマテリアル切り替え時のコストを抑えながら、GPU Resident Drawer で効率的な GPU インスタンシングを行いつつ、インスタンスごとにバリエーションのある描画を行えます。GPU Resident Drawer による自動インスタンシングが効かない状況でも、マテリアルインスタンスの複製を避けてメモリ消費を抑えられるという恩恵が受けられます。

# **シンプルな使い方**

実際に RSUV の使い方を見てみましょう。RSUV は C\# / HLSL において32ビットの整数（uint）として表現されるので、例えば32ビットのカラー（RGBAの4チャンネルが各 8ビット）を渡すには次のようにビット演算によるパッキングを行います。

CODE START csharp  
Color32 color \= /\* ... \*/;  
var colorEncoded \= (uint)(color.r \<\< 24 | color.g \<\< 16 | color.b \<\< 8 | color.a);  
meshRenderer.SetShaderUserValue(colorEncoded);  
CODE END

続いてシェーダー側の実装ですが、ここでは ShaderGraph の使用を想定します。 ShaderGraph には RSUV を取得する専用のノードが用意されていないため、Custom Function ノードを使用して次のように設定し（{{fig:custom_node}}）、コードを入力します。

![][image1]{scale=0.75}  
図: ノードの設定 {{fig:custom_node}}

CODE START c  
uint rsuv \= unity\_RendererUserValue;  
Value \= float4((float)((rsuv \>\> 24\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 16\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 8\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 0\) & 255\) \* (1.f / 255.f));  
CODE END

ノードを接続して RSUV が受け取れていることを確認します。

# **パフォーマンス計測**

RSUV が大量描画系のシチュエーションで効果を発揮することを確認するため、以下のような条件でメモリ確保量とフレームレートを計測しました。

* Unity 6000.3.15f1  
* URP 17.3  
* iPhone 12  
* シーン：10000個の Sphere  
* SRP Batcher 有効  
* フレームごとのパラメータ変化あり・なしで比較  
* GPU Resident Drawer 有効・無効で比較  
* Sphereごとに異なるシェーダーパラメータを3つの方法で設定  
  * Renderer ごとのマテリアル：10000個の Material を作成しアサイン  
  * RSUV：1個の Material で Renderer ごとに RSUV を設定  
  * MaterialPropertyBlock：1個の Material と10000個の MaterialPropertyBlock（SRP Batcher / GPU Resident Drawer の対象外となる）

![][image2]  
図: ベンチマーク画面 {{fig:benchmark}}

{{pagebreak}}

まずは各パラメータ設定手法におけるメモリ確保量の比較です（{{table:material_memory}}）。

表: メモリの増分 {{table:material_memory}}

| 条件 | メモリの増分 |
| :---- | :---- |
| Rendererごとのマテリアル | 15.79 MB |
| MaterialPropertyBlock | 3.32 MB |
| RSUV | 1.66 KB |

マテリアルが複製されないことにより、当然ながらメモリの確保量も大きく減少しています。  
次にフレームレートを見てみます。まずはフレームごとにパラメータが変化しない場合です（{{table:frame_time}}）。

表: フレームごとにパラメータが変化しない場合のフレームレート (FPS) {{table:frame_time}}

| 条件 | GPU Resident Drawer なし | GPU Resident Drawer あり |
| :---- | :---- | :---- |
| Renderer ごとのマテリアル | 33.8 | 38.8 |
| RSUV | 49.9 | 59.8 |
| MaterialPropertyBlock | 28.9 | 28.9 |

このように、RSUV の方が Renderer ごとのマテリアルよりも性能が高く、GPU Resident Drawer を併用することでその効果はより高まることがわかります。RSUV 単体ではマテリアルの切り替えコストが削減され、さらに GPU Resident Drawer を使った GPU インスタンシングが可能になることでレンダースレッド上での描画コマンドの発行負荷が軽減されます。  
次に、フレームごとにパラメータが変化する場合を見てみます（{{table:frame_time_variance}}）。

表: フレームごとにパラメータが変化する場合のフレームレート (FPS) {{table:frame_time_variance}}

| 条件 | GPU Resident Drawer なし | GPU Resident Drawer あり |
| :---- | :---- | :---- |
| Renderer ごとのマテリアル | 11.1 | 8.1 |
| RSUV | 48.0 | 60.0 |
| MaterialPropertyBlock | 26.1 | 25.2 |

パラメータ更新がない場合と比べると、Renderer ごとのマテリアルでは性能が大きく低下しているのに対し、RSUV ではほとんど劣化していません。1つのマテリアルパラメータを変更するとマテリアル全体の GPU への再アップロードが必要になるのに対し、RSUV はフレームごと・Renderer ごとに常に GPU に送信されるバッファの一部として送られる{{fn:instanced_rsuv}}ため、RSUV の更新によって追加で発生するコストは低く抑えられます。興味深いことに、マテリアルパラメータの大量更新と GPU Resident Drawer は相性が悪く、GPU Resident Drawer が無効の場合と比較してむしろ性能が低下するようです。  
以上のように「パラメータの更新」と「描画コマンド発行」の二面において RSUV が有利であることが見て取れます。特に RSUV によって GPU インスタンシングが可能になる状況（シェーダーやメッシュが同一のオブジェクトが大量に存在する場合など）で RSUV の効果は大きく高まります。

# **32ビットよりも大きいデータを渡す**

このようにパフォーマンス向上に役立つ RSUV ですが、32ビットに収まる値しか渡せないのが大きな弱点です。これを克服するテクニックを紹介します。  
RSUVで32ビットに収まらないデータを渡すには、別の GraphicsBuffer に目的のデータを格納しつつ、RSUV にはその GraphicsBuffer に対するインデックスを渡す、という方法が有効です。この方法では GraphicsBuffer のアップロードに追加のコストがかかるので、GraphicsBuffer の更新頻度やサイズには一定の配慮が必要かもしれません。

CODE START csharp  
\[StructLayout(LayoutKind.Sequential)\]  
struct VariantGpuData  
{  
    public Vector4 baseColor;  
    public Vector4 surface;  
}  
private static readonly int VariantBufferId \= Shader.PropertyToID("\_RsuvVariantBuffer");

// GraphicsBuffer のアップロードと割り当て  
var buffer \=  new GraphicsBuffer(GraphicsBuffer.Target.Structured, count, UnsafeUtility.SizeOf\<VariantGpuData\>());  
var cpuBuffer \= new VariantGpuData\[count\];  
FillCpuBufferValues(cpuBuffer);  
buffer.SetData(cpuBuffer, 0, 0, count);  
Shader.SetGlobalBuffer(VariantBufferId, buffer);

// RSUV のセット  
renderer.SetShaderUserValue(index);

// ...

// 不要になったら解放  
buffer.Dispose();  
CODE END

CODE START c  
struct RsuvVariantRecord  
{  
    float4 baseColor;  
    float4 surface;  
};

StructuredBuffer\<RsuvVariantRecord\> \_RsuvVariantBuffer;

// アクセス  
RsuvVariantRecord record \= \_RsuvVariantBuffer\[unity\_RendererUserValue\];  
CODE END

## **まとめ**

RSUV が登場した背景、パフォーマンス計測、基本的・発展的な使い方を紹介しました。  
RSUV はマテリアルと比べると32ビットまでという制限がつくものの、状況によってはパフォーマンスに大きな改善をもたらします。RSUV が効果を発揮する状況を見極めて有効に活用しましょう。

脚注: {{fn:sprite_renderer}} SpriteRenderer の RSUV 取得・設定は 2D Module が必要。  
脚注: {{fn:instanced_rsuv}} インスタンシングされない場合に限る。インスタンシングされる場合はほかのインスタンスパラメータとともに GraphicsBuffer を通して渡され、全体のパラメータが変更され GraphicsBuffer がダーティになったタイミングでのみアップロードされる。  


[image1]: assets/seto-noboru-1/media/image1.png

[image2]: assets/seto-noboru-1/media/image2.png