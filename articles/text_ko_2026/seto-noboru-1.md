# **Renderer Shader User Value를 활용한 그래픽스 최적화**

Noboru Seto / 주식회사 사이버에이전트 SGE 코어 기술본부  
[https://github.com/ruccho](https://github.com/ruccho)  
---

# **들어가며**

Unity 6.3부터 Renderer Shader User Value(RSUV)라는 기능이 추가되었습니다.  
RSUV는 Renderer 컴포넌트마다 설정할 수 있는 32비트 정수 값으로, 셰이더에서도 참조할 수 있습니다. Unity 6000.3.0f1 이후에서는 MeshRenderer, SkinnedMeshRenderer에서 RSUV를 사용할 수 있으며, Unity 6000.5.0f1, 6000.4.4f1, 6000.3.13f1 이후에서는 SpriteRenderer{{fn:sprite_renderer}}, SpriteShapeRenderer, TilemapRenderer도 지원합니다. 사용법은 간단합니다. Renderer.SetShaderUserValue()로 렌더러에 RSUV를 설정하고, 셰이더에서는 unity\_RendererUserValue를 사용해 참조하기만 하면 됩니다.

CODE START csharp  
uint value \= /\* ... \*/;  
meshRenderer.SetShaderUserValue(value);  
value \= meshRenderer.GetShaderUserValue();  
CODE END

CODE START c  
uint value \= unity\_RendererUserValue;  
CODE END

이 예시와 같은 일은 머티리얼 파라미터로도 구현할 수 있습니다. 그러나 RSUV를 사용하면 **특히 대량 렌더링 상황에서 머티리얼보다 메모리 할당량과 프레임 레이트를 개선할 수 있을** 가능성이 있습니다. 이 장에서는 RSUV가 등장한 배경과 RSUV의 효과적인 활용 방법을 설명하고, 퍼포먼스 계측 결과도 소개합니다.

## **Unity의 대량 렌더링 기능과 RSUV가 등장한 배경**

먼저 RSUV가 필요해진 배경을 되짚어 보겠습니다.  
Unity에서 대량의 오브젝트를 동시에 표시할 때의 퍼포먼스는 항상 골칫거리였습니다. 대량 렌더링에서의 퍼포먼스 문제로는 주로 '머티리얼 전환에 따른 CPU 부하', 'DrawCall 증가', '머티리얼 복제에 따른 메모리 할당'의 3가지를 들 수 있습니다.

### **머티리얼 전환에 따른 CPU 부하**

서로 다른 머티리얼을 가진 여러 오브젝트를 렌더링할 때는 프레임을 그릴 때마다 오브젝트별로 머티리얼 데이터를 수집해 GPU로 전송해야 했고, 이때의 CPU 처리 비용이 크다는 문제가 있었습니다. SRP Batcher는 이 문제를 해결하여, 머티리얼 데이터를 GPU 메모리 상에 영속화함으로써 셰이더가 동일하다면 비교적 적은 비용으로 서로 다른 머티리얼을 가진 여러 오브젝트를 렌더링할 수 있게 되었습니다.

### **DrawCall 증가**

GPU 인스턴싱은 같은 셰이더, 같은 메시, 같은 머티리얼을 가진 여러 오브젝트를 하나의 DrawCall로 렌더링하는 기법입니다. SRP Batcher보다 적용 조건이 까다로운 대신, 대량의 오브젝트를 렌더링할 때는 SRP Batcher 이상의 효과를 발휘합니다. Unity 6에서 GPU Resident Drawer가 등장하면서, 일반 MeshRenderer를 사용한 렌더링에서도 조건을 만족하면 자동으로 GPU 인스턴싱이 수행됩니다.

### **머티리얼 복제에 따른 메모리 할당**

마지막 문제는 SRP Batcher나 GPU 인스턴싱을 사용하면서도 머티리얼에 변화를 주고 싶은 상황입니다. Unity에는 오래전부터 머티리얼 인스턴스를 복제하지 않고 일부 파라미터만 오버라이드하는 MaterialPropertyBlock이라는 기능이 있었지만, 이는 SRP Batcher 및 GPU Resident Drawer와 호환되지 않았습니다. RSUV는 이 문제를 해결하여, SRP Batcher와 GPU Resident Drawer를 사용하면서 오브젝트마다 다른 32비트 파라미터를 **머티리얼을 사용하지 않고** 셰이더에 전달할 수 있습니다.

RSUV는 위의 3가지 문제를 동시에 해결하기 위해 필요했던 마지막 조각이었습니다. SRP Batcher로 머티리얼 전환 비용을 억제하면서, GPU Resident Drawer로 효율적인 GPU 인스턴싱을 수행하고, 인스턴스마다 변화를 준 렌더링을 할 수 있습니다. GPU Resident Drawer에 의한 자동 인스턴싱이 적용되지 않는 상황에서도 머티리얼 인스턴스 복제를 피해 메모리 소비를 억제할 수 있다는 이점이 있습니다.

# **간단한 사용법**

실제로 RSUV의 사용법을 살펴보겠습니다. RSUV는 C\# / HLSL에서 32비트 정수(uint)로 표현되므로, 예를 들어 32비트 컬러(RGBA 4채널, 각 8비트)를 전달하려면 다음과 같이 비트 연산으로 패킹합니다.

CODE START csharp  
Color32 color \= /\* ... \*/;  
var colorEncoded \= (uint)(color.r \<\< 24 | color.g \<\< 16 | color.b \<\< 8 | color.a);  
meshRenderer.SetShaderUserValue(colorEncoded);  
CODE END

이어서 셰이더 쪽 구현입니다. 여기서는 ShaderGraph 사용을 가정합니다. ShaderGraph에는 RSUV를 가져오는 전용 노드가 없으므로, Custom Function 노드를 사용해 다음과 같이 설정하고({{fig:custom_node}}) 코드를 입력합니다.

![][image1]{scale=0.75}  
図: 노드 설정 {{fig:custom_node}}

CODE START c  
uint rsuv \= unity\_RendererUserValue;  
Value \= float4((float)((rsuv \>\> 24\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 16\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 8\) & 255\) \* (1.f / 255.f),  
                        (float)((rsuv \>\> 0\) & 255\) \* (1.f / 255.f));  
CODE END

노드를 연결하여 RSUV가 수신되는지 확인합니다.

# **퍼포먼스 계측**

RSUV가 대량 렌더링 상황에서 효과를 발휘하는지 확인하기 위해, 다음과 같은 조건으로 메모리 할당량과 프레임 레이트를 계측했습니다.

* Unity 6000.3.15f1  
* URP 17.3  
* iPhone 12  
* 씬: Sphere 10000개  
* SRP Batcher 활성화  
* 프레임마다 파라미터 변화가 있는 경우와 없는 경우를 비교  
* GPU Resident Drawer 활성화·비활성화를 비교  
* Sphere마다 다른 셰이더 파라미터를 3가지 방법으로 설정  
  * Renderer별 머티리얼: Material 10000개를 생성하여 할당  
  * RSUV: Material 1개로 Renderer마다 RSUV를 설정  
  * MaterialPropertyBlock: Material 1개와 MaterialPropertyBlock 10000개(SRP Batcher / GPU Resident Drawer의 대상에서 제외됨)

![][image2]  
図: 벤치마크 화면 {{fig:benchmark}}

{{pagebreak}}

먼저 각 파라미터 설정 기법별 메모리 할당량을 비교합니다({{table:material_memory}}).

表: 메모리 증가량 {{table:material_memory}}

| 조건 | 메모리 증가량 |
| :---- | :---- |
| Renderer별 머티리얼 | 15.79 MB |
| MaterialPropertyBlock | 3.32 MB |
| RSUV | 1.66 KB |

머티리얼이 복제되지 않으므로 당연히 메모리 할당량도 크게 줄었습니다.  
다음으로 프레임 레이트를 살펴봅니다. 먼저 프레임마다 파라미터가 변화하지 않는 경우입니다({{table:frame_time}}).

表: 프레임마다 파라미터가 변화하지 않는 경우의 프레임 레이트(FPS) {{table:frame_time}}

| 조건 | GPU Resident Drawer 없음 | GPU Resident Drawer 있음 |
| :---- | :---- | :---- |
| Renderer별 머티리얼 | 33.8 | 38.8 |
| RSUV | 49.9 | 59.8 |
| MaterialPropertyBlock | 28.9 | 28.9 |

이처럼 RSUV가 Renderer별 머티리얼보다 성능이 높고, GPU Resident Drawer를 함께 사용하면 그 효과가 더욱 커짐을 알 수 있습니다. RSUV 단독으로는 머티리얼 전환 비용이 절감되고, 여기에 GPU Resident Drawer를 사용한 GPU 인스턴싱이 가능해지면 렌더 스레드에서의 드로우 커맨드 발행 부하가 줄어듭니다.  
다음으로 프레임마다 파라미터가 변화하는 경우를 살펴봅니다({{table:frame_time_variance}}).

表: 프레임마다 파라미터가 변화하는 경우의 프레임 레이트(FPS) {{table:frame_time_variance}}

| 조건 | GPU Resident Drawer 없음 | GPU Resident Drawer 있음 |
| :---- | :---- | :---- |
| Renderer별 머티리얼 | 11.1 | 8.1 |
| RSUV | 48.0 | 60.0 |
| MaterialPropertyBlock | 26.1 | 25.2 |

파라미터 업데이트가 없는 경우와 비교하면 Renderer별 머티리얼에서는 성능이 크게 저하되는 반면, RSUV에서는 거의 저하되지 않습니다. 머티리얼 파라미터를 하나 변경하면 머티리얼 전체를 GPU에 다시 업로드해야 하는 반면, RSUV는 프레임마다·Renderer마다 항상 GPU로 전송되는 버퍼의 일부로 전송되므로{{fn:instanced_rsuv}}, RSUV 업데이트로 추가 발생하는 비용은 낮게 억제됩니다. 흥미롭게도 머티리얼 파라미터의 대량 업데이트와 GPU Resident Drawer는 궁합이 좋지 않아, GPU Resident Drawer가 비활성화된 경우와 비교해 오히려 성능이 저하되는 것으로 보입니다.  
이상과 같이 '파라미터 업데이트'와 '드로우 커맨드 발행'이라는 두 측면에서 RSUV가 유리함을 확인할 수 있습니다. 특히 RSUV로 GPU 인스턴싱이 가능해지는 상황(셰이더나 메시가 동일한 오브젝트가 대량으로 존재하는 경우 등)에서 RSUV의 효과가 크게 높아집니다.

# **32비트보다 큰 데이터 전달하기**

이처럼 퍼포먼스 향상에 도움이 되는 RSUV이지만, 32비트에 들어가는 값만 전달할 수 있다는 점이 큰 약점입니다. 이를 극복하는 기법을 소개합니다.  
RSUV로 32비트에 들어가지 않는 데이터를 전달하려면, 별도의 GraphicsBuffer에 목적 데이터를 저장하고 RSUV에는 해당 GraphicsBuffer에 대한 인덱스를 전달하는 방법이 효과적입니다. 이 방법은 GraphicsBuffer 업로드에 추가 비용이 들기 때문에, GraphicsBuffer의 업데이트 빈도와 크기에는 일정한 주의가 필요할 수 있습니다.

CODE START csharp  
\[StructLayout(LayoutKind.Sequential)\]  
struct VariantGpuData  
{  
    public Vector4 baseColor;  
    public Vector4 surface;  
}  
private static readonly int VariantBufferId \= Shader.PropertyToID("\_RsuvVariantBuffer");

// GraphicsBuffer의 업로드와 할당  
var buffer \=  new GraphicsBuffer(GraphicsBuffer.Target.Structured, count, UnsafeUtility.SizeOf\<VariantGpuData\>());  
var cpuBuffer \= new VariantGpuData\[count\];  
FillCpuBufferValues(cpuBuffer);  
buffer.SetData(cpuBuffer, 0, 0, count);  
Shader.SetGlobalBuffer(VariantBufferId, buffer);

// RSUV 설정  
renderer.SetShaderUserValue(index);

// ...

// 불필요해지면 해제  
buffer.Dispose();  
CODE END

CODE START c  
struct RsuvVariantRecord  
{  
    float4 baseColor;  
    float4 surface;  
};

StructuredBuffer\<RsuvVariantRecord\> \_RsuvVariantBuffer;

// 액세스  
RsuvVariantRecord record \= \_RsuvVariantBuffer\[unity\_RendererUserValue\];  
CODE END

## **정리**

RSUV가 등장한 배경, 퍼포먼스 계측, 기본적·발전적인 사용법을 소개했습니다.  
RSUV는 머티리얼과 비교하면 32비트까지라는 제한이 있지만, 상황에 따라 퍼포먼스를 크게 개선합니다. RSUV가 효과를 발휘하는 상황을 파악하여 효과적으로 활용하시기 바랍니다.

脚注: {{fn:sprite_renderer}} SpriteRenderer의 RSUV 가져오기·설정에는 2D Module이 필요합니다.  
脚注: {{fn:instanced_rsuv}} 인스턴싱되지 않는 경우에 한합니다. 인스턴싱되는 경우에는 다른 인스턴스 파라미터와 함께 GraphicsBuffer를 통해 전달되며, 전체 파라미터가 변경되어 GraphicsBuffer가 더티 상태가 된 시점에만 업로드됩니다.  


[image1]: assets/seto-noboru-1/media/image1.png

[image2]: assets/seto-noboru-1/media/image2.png