# **利用 Renderer Shader User Value 进行图形优化**

Noboru Seto / 株式会社 CyberAgent SGE 核心技术本部  
[https://github.com/ruccho](https://github.com/ruccho)  
---

# **前言**

从 Unity 6.3 起新增了名为 Renderer Shader User Value（RSUV）的功能。  
RSUV 是可按 Renderer 组件逐个设置的 32 位整数值，并且可以在着色器中引用。Unity 6000.3.0f1 及以后版本可在 MeshRenderer、SkinnedMeshRenderer 中使用 RSUV，Unity 6000.5.0f1、6000.4.4f1、6000.3.13f1 及以后版本还支持 SpriteRenderer{{fn:sprite_renderer}}、SpriteShapeRenderer、TilemapRenderer。用法很简单：通过 Renderer.SetShaderUserValue() 为渲染器设置 RSUV，在着色器中使用 unity\_RendererUserValue 引用即可。

CODE START csharp  
uint value \= /\* ... \*/;  
meshRenderer.SetShaderUserValue(value);  
value \= meshRenderer.GetShaderUserValue();  
CODE END

CODE START c  
uint value \= unity\_RendererUserValue;  
CODE END

与本例相同的效果也可以通过材质参数实现。不过，使用 RSUV 后，**尤其是在大量绘制的场合，内存分配量和帧率都有可能优于使用材质的做法**。本章将说明 RSUV 出现的背景、RSUV 的有效用法，并介绍性能测量结果。

## **Unity 的大量绘制功能与 RSUV 出现的背景**

首先回顾一下为什么需要 RSUV。  
在 Unity 中同时显示大量对象时的性能一直是令人头疼的问题。大量绘制中的性能问题主要有三点：“切换材质时的 CPU 开销”“DrawCall 增多”“复制材质导致的内存分配”。

### **切换材质时的 CPU 开销**

绘制多个拥有不同材质的对象时，每帧绘制都需要逐个对象收集材质数据并发送到 GPU，此时的 CPU 处理成本很高。SRP Batcher 解决了这一问题：通过将材质数据持久化保存在 GPU 内存中，只要着色器相同，就能以相对较低的成本绘制多个拥有不同材质的对象。

### **DrawCall 增多**

GPU 实例化（GPU Instancing）是将拥有相同着色器、相同网格、相同材质的多个对象用一个 DrawCall 绘制的方法。它的适用条件比 SRP Batcher 更严格，但在绘制大量对象时能发挥比 SRP Batcher 更大的效果。Unity 6 引入了 GPU Resident Drawer，使得即使是使用普通 MeshRenderer 的绘制，只要满足条件也会自动进行 GPU 实例化。

### **复制材质导致的内存分配**

最后一个问题出现在既想利用 SRP Batcher 或 GPU 实例化、又想让材质有所变化的场合。Unity 很早就提供了 MaterialPropertyBlock 功能，可以在不复制材质实例的情况下仅覆盖部分参数，但它与 SRP Batcher 和 GPU Resident Drawer 不兼容。RSUV 解决了这一问题：在利用 SRP Batcher 和 GPU Resident Drawer 的同时，可以**不借助材质**将每个对象各不相同的 32 位参数传递给着色器。

RSUV 是同时解决上述三个问题所需的最后一块拼图。它可以在用 SRP Batcher 降低材质切换成本、用 GPU Resident Drawer 进行高效 GPU 实例化的同时，按实例绘制出有差异的效果。即使在 GPU Resident Drawer 的自动实例化不生效的情况下，也能享受到避免复制材质实例、从而减少内存消耗的好处。

# **简单用法**

下面来看 RSUV 的实际用法。RSUV 在 C\# / HLSL 中表示为 32 位整数（uint），因此，例如要传递 32 位颜色（RGBA 4 个 channel 各 8 位）时，需要像下面这样通过位运算进行打包。

CODE START csharp  
Color32 color \= /\* ... \*/;  
var colorEncoded \= (uint)(color.r \<\< 24 | color.g \<\< 16 | color.b \<\< 8 | color.a);  
meshRenderer.SetShaderUserValue(colorEncoded);  
CODE END

接下来是着色器侧的实现，这里假定使用 ShaderGraph。ShaderGraph 没有提供获取 RSUV 的专用节点，因此使用 Custom Function 节点，按如下方式设置（{{fig:custom_node}}）并输入代码。

![][image1]{scale=0.75}  
図: 节点设置 {{fig:custom_node}}

CODE START c  
uint rsuv \= unity\_RendererUserValue;  
Value \= float4((float)((rsuv \>\> 24\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 16\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 8\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 0\) & 255\) \* (1.f / 255.f));  
CODE END

连接节点，确认已能接收到 RSUV。

# **性能测量**

为了确认 RSUV 在大量绘制的场合能发挥效果，在以下条件下测量了内存分配量和帧率。

* Unity 6000.3.15f1  
* URP 17.3  
* iPhone 12  
* 场景：10000 个 Sphere  
* 启用 SRP Batcher  
* 比较参数每帧变化与不变化两种情况  
* 比较启用与禁用 GPU Resident Drawer 两种情况  
* 用三种方法为每个 Sphere 设置不同的着色器参数  
  * 每个 Renderer 一个材质：创建 10000 个 Material 并分配  
  * RSUV：使用 1 个 Material，按 Renderer 设置 RSUV  
  * MaterialPropertyBlock：1 个 Material 加 10000 个 MaterialPropertyBlock（不在 SRP Batcher / GPU Resident Drawer 的适用范围内）

![][image2]  
図: 基准测试画面 {{fig:benchmark}}

{{pagebreak}}

首先比较各种参数设置方法的内存分配量（{{table:material_memory}}）。

表: 内存增量 {{table:material_memory}}

| 条件 | 内存增量 |
| :---- | :---- |
| 每个 Renderer 一个材质 | 15.79 MB |
| MaterialPropertyBlock | 3.32 MB |
| RSUV | 1.66 KB |

由于材质不会被复制，内存分配量自然也大幅减少。  
接下来看帧率。首先是参数每帧不变化的情况（{{table:frame_time}}）。

表: 参数每帧不变化时的帧率（FPS）{{table:frame_time}}

| 条件 | 无 GPU Resident Drawer | 有 GPU Resident Drawer |
| :---- | :---- | :---- |
| 每个 Renderer 一个材质 | 33.8 | 38.8 |
| RSUV | 49.9 | 59.8 |
| MaterialPropertyBlock | 28.9 | 28.9 |

可以看出，RSUV 的性能高于每个 Renderer 一个材质的做法，并且与 GPU Resident Drawer 配合使用时效果更加明显。单独使用 RSUV 就能削减材质切换成本，再加上可以通过 GPU Resident Drawer 进行 GPU 实例化，渲染线程上发出绘制命令的开销也随之减轻。  
接下来看参数每帧变化的情况（{{table:frame_time_variance}}）。

表: 参数每帧变化时的帧率（FPS）{{table:frame_time_variance}}

| 条件 | 无 GPU Resident Drawer | 有 GPU Resident Drawer |
| :---- | :---- | :---- |
| 每个 Renderer 一个材质 | 11.1 | 8.1 |
| RSUV | 48.0 | 60.0 |
| MaterialPropertyBlock | 26.1 | 25.2 |

与不更新参数的情况相比，每个 Renderer 一个材质的做法性能大幅下降，而 RSUV 几乎没有劣化。修改一个材质参数就需要将整个材质重新上传到 GPU，而 RSUV 是作为每帧、每个 Renderer 都会发送到 GPU 的缓冲区的一部分一起发送的{{fn:instanced_rsuv}}，因此更新 RSUV 所产生的额外成本很低。有趣的是，大量更新材质参数与 GPU Resident Drawer 似乎并不合拍，与禁用 GPU Resident Drawer 相比性能反而下降。  
如上所述，在“参数更新”和“绘制命令发出”两方面，RSUV 都更具优势。尤其是在 RSUV 使 GPU 实例化成为可能的情况下（例如存在大量着色器和网格相同的对象时），RSUV 的效果会显著提升。

# **传递大于 32 位的数据**

RSUV 虽然有助于提升性能，但只能传递 32 位以内的值，这是它的一大弱点。下面介绍克服这一限制的技巧。  
要通过 RSUV 传递 32 位放不下的数据，有效的方法是将目标数据存放在另一个 GraphicsBuffer 中，并把该 GraphicsBuffer 的索引传给 RSUV。这种方法需要额外的 GraphicsBuffer 上传成本，因此可能需要对 GraphicsBuffer 的更新频率和大小有所考虑。

CODE START csharp  
\[StructLayout(LayoutKind.Sequential)\]  
struct VariantGpuData  
{  
    public Vector4 baseColor;  
    public Vector4 surface;  
}  
private static readonly int VariantBufferId \= Shader.PropertyToID("\_RsuvVariantBuffer");

// 上传并绑定 GraphicsBuffer  
var buffer \=  new GraphicsBuffer(GraphicsBuffer.Target.Structured, count, UnsafeUtility.SizeOf\<VariantGpuData\>());  
var cpuBuffer \= new VariantGpuData\[count\];  
FillCpuBufferValues(cpuBuffer);  
buffer.SetData(cpuBuffer, 0, 0, count);  
Shader.SetGlobalBuffer(VariantBufferId, buffer);

// 设置 RSUV  
renderer.SetShaderUserValue(index);

// ...

// 不再需要时释放  
buffer.Dispose();  
CODE END

CODE START c  
struct RsuvVariantRecord  
{  
    float4 baseColor;  
    float4 surface;  
};

StructuredBuffer\<RsuvVariantRecord\> \_RsuvVariantBuffer;

// 访问  
RsuvVariantRecord record \= \_RsuvVariantBuffer\[unity\_RendererUserValue\];  
CODE END

## **总结**

本章介绍了 RSUV 出现的背景、性能测量，以及基本和进阶用法。  
与材质相比，RSUV 虽然有最多 32 位的限制，但在某些情况下能带来显著的性能改善。请判断 RSUV 能发挥效果的场合，并有效加以利用。

脚注: {{fn:sprite_renderer}} SpriteRenderer 的 RSUV 获取与设置需要 2D Module。  
脚注: {{fn:instanced_rsuv}} 仅限于未被实例化的情况。被实例化时，RSUV 会与其他实例参数一起通过 GraphicsBuffer 传递，并且仅在整体参数发生变更、GraphicsBuffer 变为脏状态时才会上传。  


[image1]: assets/seto-noboru-1/media/image1.png

[image2]: assets/seto-noboru-1/media/image2.png