# **스타일 가이드**

# **Unity 6 Render Graph 타일 메모리 최적화**

이름: Kiyohara Takayuki  
소속：주식회사 사이버에이전트 SGE 코어 기술본부  
X 계정: nezumimusume

## **들어가며**

이 장에서는 Unity 6의 Render Graph를 이용해 모바일 GPU(타일 기반 렌더링 아키텍처)의 타일 메모리를 활용하고, GPU 시간, 메모리 대역폭, 메모리 사용량을 절감하는 방법을 설명합니다.  
모바일 게임 개발에서는 GPU의 연산 성능보다 메모리 대역폭이 먼저 병목이 되는 경우가 많습니다. 또한 메모리 대역폭 소비는 소비 전력과 발열에 직결되므로, 프레임 레이트뿐 아니라 기기의 발열과 배터리 소모에도 영향을 줍니다.  
이 장에서는 다음 네 가지를 주제로 설명합니다. 각 주제에는 같은 이름의 절을 할당했습니다.

1. 타일 메모리의 로드/스토어  
2. Render Pass/Subpass  
3.  프레임 버퍼 페치  
4. 메모리리스

# **타일 기반 렌더링의 기초**

이 절에서는 모바일 GPU가 타일 메모리를 가지고 있는 이유를 설명합니다.

## **모바일 GPU와 메모리 대역폭**

현대 GPU는 연산 속도와 메모리 읽기/쓰기 속도의 격차가 계속 커지고 있어, 메모리 액세스가 큰 병목이 되고 있습니다. GPU 코어의 연산 성능(FLOPS)은 세대마다 크게 향상되는 반면, 메모리 대역폭의 향상은 이를 따라가지 못합니다.  
PC의 디스크리트 GPU는 GDDR과 같은 그래픽스 전용 광대역 메모리를 탑재하여 이 문제를 완화합니다. 그러나 모바일 SoC에서는 그렇게 할 수 없습니다.

| 항목 | PC(디스크리트 GPU) | 모바일 SoC |
| :---- | :---- | :---- |
| 그래픽스 메모리 | 전용(GDDR6 등, 수백 GB/s) | 없음(LPDDR을 CPU/GPU/ISP 등이 공유) |
| 대역폭 규모 | 수백 GB/s~1TB/s | 수십 GB/s(공유) |
| 전력 제약 | 수백 W급 전원과 냉각 | 수 W, 패시브 냉각만 사용 |

모바일 GPU에는 전용 그래픽스 메모리가 없으며, CPU와 같은 LPDDR 메모리를 공유합니다. 게다가 공유 대상은 CPU뿐만 아니라 카메라 ISP나 머신러닝 가속기 등 SoC 상의 모든 블록입니다. 대역폭 자체가 좁은 데다 서로 경합합니다.  
또한 메인 메모리(DRAM)에 대한 액세스는 SoC 안에서도 특히 전력을 많이 소비하는 동작입니다. 대역폭을 많이 사용할수록 소비 전력이 늘고 발열이 생기며, 서멀 스로틀링으로 CPU/GPU 클럭이 떨어지고 프레임 레이트가 하락합니다. 즉 모바일에서는 다음과 같은  
\`\`\`  
메모리 대역폭 소비 → 소비 전력 증가 → 발열 → 스로틀링 → 성능 저하  
\`\`\`  
흐름이 발생합니다. 처음 몇 분은 60FPS로 동작하다가 한동안 플레이하면 프레임 레이트가 떨어지는 경우, 이 대역폭과 발열이 원인인 일이 많습니다.  
이러한 제약 속에서 화면을 그려내기 위해 등장한 것이 타일 기반 렌더링(TBR: Tile-Based Rendering) 아키텍처의 GPU입니다.

## **IMR과 TBR**

TBR을 이해하기 위해 먼저 PC GPU의 주류인 이미디어트 모드 렌더링(IMR: Immediate Mode Rendering)과 비교해 보겠습니다.  
IMR은 드로우 콜로 투입된 프리미티브를 그 자리에서 래스터라이즈하고 프래그먼트 셰이더를 실행한 뒤, 결과를 메모리 상의 렌더 타깃에 기록하는 방식입니다. 깊이 테스트나 블렌딩 때마다 렌더 타깃에 대한 읽기/쓰기가 발생하지만, PC GPU는 광대역 그래픽스 메모리와 대용량 캐시로 이를 감당합니다.  
반면 TBR은 화면을 작은 타일로 분할하고 타일 한 장씩 렌더링을 완결하는 방식입니다. 각 셰이더 코어에는 타일 메모리라는 작은 온칩 메모리가 탑재되어 있으며, 타일 내 렌더링(컬러, 깊이, 스텐실의 읽기/쓰기, 블렌딩)은 모두 이 타일 메모리 상에서 이루어집니다. 메인 메모리 액세스는 원칙적으로 타일 렌더링 시작 시(로드)와 종료 시(스토어)의 두 번뿐입니다.  
![][image1]  
図: IMR과 TBR의 비교   
다음 그림은 TBR GPU의 셰이더 코어 개념도입니다. 셰이더 코어에 타일 메모리가 직결되어 있습니다. 온칩이고 물리적 거리가 가깝기 때문에, 타일 메모리 액세스는 메인 메모리 액세스에 비해 자릿수가 다를 정도로 빠르고 소비 전력도 낮습니다. 실제 GPU에서도 이 구조를 확인할 수 있으며, 예를 들어 ARM Mali-G71의 공식 자료에 실린 셰이더 코어 설계도에는 타일 메모리가 셰이더 코어에 직결된 모습이 나타나 있습니다.  
![][image2]  
図: 셰이더 코어와 타일 메모리(개념도)  
이처럼 IMR에서 메모리 액세스가 가장 많았던 깊이 테스트와 블렌딩이 TBR에서는 타일 메모리 안에서 완결됩니다.

## **TBR의 2단계**

TBR GPU는 한 프레임의 렌더링을 크게 두 단계로 나누어 실행합니다.

### **비닝 단계(지오메트리 단계)**

먼저 프레임 내 모든 드로우 콜의 버텍스 셰이더(위치 계산 부분)를 실행하여, 각 프리미티브가 어느 타일에 걸치는지 분류(비닝)합니다. 분류 결과는 "타일별 프리미티브 리스트"로 메인 메모리에 기록됩니다.  
여기서 주의할 점은 비닝 단계 자체도 메인 메모리 대역폭을 소비한다는 것입니다. 버텍스 데이터가 많을수록 프리미티브 리스트의 읽기/쓰기가 늘어납니다. TBR GPU에서 하이 폴리곤 씬이 불리해지기 쉬운 것은 이 때문입니다.

### **래스터라이즈 단계(렌더링 단계)**

이어서 타일을 한 장씩 처리합니다. 각 타일에 대해 다음과 같이

1. 필요하면 렌더 타깃의 기존 내용을 타일 메모리로 로드  
2.  해당 타일에 걸친 프리미티브를 래스터라이즈하고 프래그먼트 셰이더를 실행(읽기/쓰기는 모두 타일 메모리 상)  
3.  완성된 타일의 내용을 메인 메모리에 스토어

순서로 렌더링이 진행됩니다. 이 순서의 1번과 3번이 이 장의 주제인 로드/스토어입니다. 자세한 내용은 다음 절 "타일 메모리의 로드/스토어"에서 다루지만, 타일 렌더링 자체보다 타일 경계에서 메인 메모리와 주고받는 작업이 더 비싸다는 것이 TBR의 비용 구조입니다.  
![][image3]  
図: 비닝 단계와 래스터라이즈 단계

### **벤더별 명칭과 특성**

TBR의 기본 구조는 공통이지만, GPU 벤더마다 명칭과 세부 구현이 다릅니다. 이 장에서는 이후 총칭으로 타일 메모리/비닝이라는 용어를 사용합니다.

| 벤더(GPU) | 단계/기구 명칭 | 특기 사항 |
| :---- | :---- | :---- |
| Qualcomm Adreno | Binning / GMEM | GMEM이라는 온칩 메모리를 분할하여 사용한다. 렌더링 내용에 따라 IMR과 유사한 동작으로 전환되는 모드가 있다 |
| ARM Mali | Tiler / Tile Memory | 타일은 16×16 픽셀이 기본 |
| Imagination(PowerVR/IMG) | TBDR | 은면 제거(HSR)를 하드웨어로 수행하는 TBDR. Pixel 10의 IMG DXT도 이 계보 |
| Apple GPU | Tile Memory | PowerVR 계보의 TBDR. Metal에서 타일 메모리를 명시적으로 다룰 수 있다 |

## **이 장에서 다루는 네 가지 주제**

지금까지 타일 경계의 로드/스토어가 비용이 된다는 것을 알아보았습니다. 그렇다면 그 비용은 어떻게 줄일 수 있을까요? 이 장에서 다루는 네 가지 주제는 모두 이 질문에 대한 수단입니다.  
![][image4]  
図: 어태치먼트의 수명과 네 가지 주제의 관계  
어떤 렌더 타깃(어태치먼트)의 한 프레임 라이프사이클을 생각해 보겠습니다.  
\`\`\`  
확보 → 로드 → 렌더링 → 스토어 → (후속 패스에서 텍스처로) 샘플링 → 해제  
\`\`\`  
이 라이프사이클의 어느 부분을 줄이는가에 따라 네 가지 주제를 다음과 같이 정리할 수 있습니다.

| 주제 | 줄이는 대상 | 접근 방식 | 절 |
| :---- | :---- | :---- | :---- |
| 로드/스토어 | 불필요한 로드와 불필요한 스토어 | "모든 픽셀을 덮어쓰면 로드 불필요", "나중에 사용하지 않으면 스토어 불필요"를 명시한다 | "타일 메모리의 로드/스토어" |
| Render Pass/Subpass | 패스와 패스 사이의 스토어→로드 왕복 | 여러 패스를 하나의 Render Pass로 통합(머지)하고 중간 결과를 타일 메모리에 남긴다 | "Render Pass / Subpass" |
| 프레임 버퍼 페치 | 스토어→샘플링 왕복 | 이전 서브패스의 결과를 타일 메모리에서 직접 읽는다 | "프레임 버퍼 페치" |
| 메모리리스 | 메인 메모리 확보 자체 | 로드도 스토어도 하지 않으면 메인 메모리 상의 실체는 불필요하다 | "메모리리스" |

이 네 가지는 독립된 기법이 아닙니다. 패스가 머지되어야 프레임 버퍼 페치를 사용할 수 있고, 로드도 스토어도 사라져야 메모리리스가 성립합니다. 이 장의 절 구성도 비용 모델(로드/스토어) → 구조(Subpass) → 활용(페치) → 마무리(메모리리스)라는 이 의존 관계를 따릅니다.  
또한 Render Graph는 패스별 리소스 사용 정보(무엇을 읽고 무엇에 쓰는가)를 수집한 뒤 렌더링 실행 계획을 세우는 시스템입니다. 따라서 패스 머지, 로드/스토어 액션 결정, 메모리리스화와 같은 최적화를 자동으로 수행할 수 있습니다. 이 장에서 설명하는 내용은 이러한 최적화가 작동하도록 리소스 사용 방식을 Render Graph에 올바르게 전달하는 방법이라고 할 수 있습니다.

## **검증 도구 사용법**

이 장에서는 도구로 파이프라인 상태를 확인한 뒤 계측하는 흐름을 반복합니다. 주로 사용하는 도구는 다음 세 가지입니다.

## **Frame Debugger**

Unity의 Frame Debugger(Window \> Analysis \> Frame Debugger)는 패스 구조를 확인하는 기본 도구입니다. 이 장에서 특히 보는 것은 이벤트 이름의 \`(RP n:m)\` 표기입니다. \`RP 4:0\`, \`RP 4:1\`처럼 같은 Render Pass 번호(n)에 서브패스 번호(m)가 연번으로 나열되어 있다면, 해당 패스들은 하나의 Native Render Pass로 머지된 것입니다.  
한편 패스 머지(Native Render Pass)가 동작하는 것은 Vulkan/Metal 등의 환경입니다. 에디터의 그래픽스 API가 D3D 등으로 되어 있으면 머지를 관찰할 수 없으므로, 에디터에서 확인할 때는 Graphics API를 Vulkan으로 변경하거나 실기기 빌드에 Frame Debugger를 어태치하시기 바랍니다.  
![][image5]  
図:  Frame Debugger에서의 머지 상태

## **Render Graph Viewer**

Render Graph Viewer(Window \> Analysis \> Render Graph Viewer)는 Render Graph가 구축한 실행 계획 자체를 시각화하는 도구입니다.

*  패스 목록과, 각 패스가 액세스하는 리소스의 타임라인(어떤 리소스가 언제 생성되고 언제 파괴되는가)  
*  어태치먼트별 Load Action / Store Action 및 Memoryless 상태 표시. 이 장에서는 이 표시를 사용해 최적화 결과를 확인합니다  
*  패스가 머지되었는지 여부의 표시  
*  머지되지 않은 이유(Pass Break Reasoning)의 표시

네 번째인 Pass Break Reasoning은 "머지되어야 할 패스가 머지되지 않을 때" 가장 먼저 확인하는 정보입니다. "Render Pass / Subpass" 절 이후에서 반복해서 사용합니다.  
![][image6]  
 図: Render Graph Viewer에서 머지 상태와 로드/스토어 액션 확인 

## **RenderDoc**

RenderDoc으로 Vulkan 캡처를 수행하면 각 Render Pass의 어태치먼트에 지정된 \`loadOp\`/\`storeOp\` 값을 API 수준에서 확인할 수 있습니다. Render Graph Viewer의 표시를 한 단계 더 파고들고 싶을 때나, Unity의 표시와 드라이버에 전달된 값을 대조하고 싶을 때 사용합니다.

# **타일 메모리의 로드/스토어**

이 절에서는 이 장 전체의 토대가 되는 로드/스토어 액션에 대해 설명합니다.

## **로드/스토어 액션이란**

TBR GPU는 타일 렌더링 시작 시 렌더 타깃의 기존 내용을 타일 메모리로 읽어 들이고(로드), 렌더링 종료 시 타일 메모리의 내용을 메인 메모리에 기록합니다(스토어).  
다만 로드와 스토어가 항상 필요한 것은 아닙니다. 그래픽스 API에는 어태치먼트별로 "시작 시 어떻게 할 것인가", "종료 시 어떻게 할 것인가"를 지정하기 위한 액션이 마련되어 있습니다.

### **로드 액션**

| 액션 | 동작 | 대역폭 비용 |
| :---- | :---- | :---- |
| Load | 메인 메모리의 기존 내용을 타일 메모리로 읽어 들인다 | 높음(해상도×포맷만큼 읽기) |
| Clear | 타일 메모리를 지정한 색으로 클리어한다 | 거의 0(타일 메모리 내 동작만) |
| DontCare | 아무것도 하지 않는다(타일 메모리의 초깃값은 불정) | 0 |

### **스토어 액션**

| 액션 | 동작 | 대역폭 비용 |
| :---- | :---- | :---- |
| Store | 타일 메모리의 내용을 메인 메모리에 기록한다 | 높음(해상도×포맷만큼 쓰기) |
| Resolve | MSAA 샘플을 리졸브한 결과만 기록한다 | 중간(리졸브 후 1샘플분만) |
| DontCare | 아무것도 기록하지 않는다(내용은 폐기된다) | 0 |

즉 대역폭을 소비하는 것은 Load와 Store(및 Resolve)뿐입니다. 반대로 말하면 Clear/DontCare로 시작해서 DontCare로 끝낼 수 있는 어태치먼트는 메인 메모리와 전혀 주고받지 않습니다. 이 상태가 나중에 다룰 메모리리스의 전제 조건이 됩니다.  
로드/스토어를 최적화하려면 각 어태치먼트에 대해 다음 두 가지를 확인해야 합니다.

1. 시작 시 이전 내용이 필요한가? 모든 픽셀을 덮어쓰는 패스라면 불필요합니다(DontCare). 클리어로 시작하면 Clear, 이전 렌더링 결과 위에 덧그릴 때만 Load가 됩니다.  
2. 종료 시 이 내용을 나중에 사용하는가? 후속 패스나 다음 프레임에서 사용하면 Store, 이 Render Pass 안에서 사용이 끝나면 DontCare입니다.

## **대역폭 비용 산정**

로드/스토어 1회의 비용은 단순한 곱셈으로 어림할 수 있습니다.  
\`\`\`  
1회 로드(또는 스토어)의 데이터량 \= 가로 해상도 × 세로 해상도 × 1픽셀의 바이트 수  
\`\`\`  
1080p(1920×1080)에서 대표적인 포맷의 값을 나타냅니다.

| 포맷 | 1픽셀 | 1회 로드/스토어 | 60FPS 시 대역폭 |
| :---- | :---- | :---- | :---- |
| R8G8B8A8 (RGBA8) | 4바이트 | 약 7.9MB | 약 475MB/s |
| B10G11R11\_Float | 4바이트 | 약 7.9MB | 약 475MB/s |
| R16G16B16A16\_Float | 8바이트 | 약 15.9MB | 약 950MB/s |
| D24S8 / D32(깊이) | 4바이트 | 약 7.9MB | 약 475MB/s |

예를 들어 "카메라 컬러(RGBA8)를 스토어하고 다음 패스에서 로드"하기만 해도 왕복 약 16MB, 60FPS라면 약 950MB/s의 대역폭을 소비합니다. 모바일 SoC의 실효 대역폭이 수십 GB/s이고 CPU와 경합한다는 점을 고려하면 무시할 수 있는 양이 아닙니다.  
또한 다음 사항에도 주의하시기 바랍니다.

* MSAA는 배율이 곱해집니다. 4xMSAA 컬러 버퍼를 Resolve하지 않고 Store하면 데이터량이 4배가 됩니다(MSAA와 스토어의 관계는 "메모리리스" 절에서 다룹니다).  
* 로드와 스토어는 둘 다 계산합니다. 패스를 넘을 때마다 "이전 패스의 Store + 다음 패스의 Load"가 한 쌍으로 발생합니다.  
* 스토어한 결과를 후속 패스에서 샘플링하면 (캐시에 올라가지 않는 분량은) 추가로 읽기가 발생합니다.

개발 중인 게임의 렌더링 파이프라인에서 한 프레임에 몇 MB가 타일 메모리와 메인 메모리 사이를 왕복하는지 한 번 조사해 보시기 바랍니다. Render Graph Viewer에서 각 패스의 어태치먼트와 액션을 확인하고 위 표를 곱하기만 해도, 어느 패스부터 줄여야 하는지 우선순위가 보입니다.  
![][image7]  
図: 타일 메모리⇔메인 메모리의 로드/스토어와 대역폭 비용 

## **AccessFlags를 통한 리소스 사용 신고**

그렇다면 이 로드/스토어 액션을 Render Graph에서는 어떻게 지정할까요?  
사실 Render Graph에서는 로드/스토어 액션을 직접 지정하지 않습니다. 패스를 추가할 때 빌더에 리소스 사용 방식을 신고하면, Render Graph가 그 정보로 액션을 결정합니다. 이때 신고에 사용하는 것이 \`AccessFlags\`입니다.  
CODE START csharp | AccessFlags를 통한 리소스 신고 {{list:access_flags_usage}}  
public override void RecordRenderGraph(RenderGraph renderGraph, ContextContainer frameData)  
{  
    using (var builder \= renderGraph.AddRasterRenderPass\<PassData\>("Example", out var passData))  
    {  
        // 출력 어태치먼트를 선언한다.  
        // AccessFlags.WriteAll = 이 패스는 모든 픽셀을 기록한다(이전 내용은 불필요)  
        builder.SetRenderAttachment(\_targetHandle, 0, AccessFlags.WriteAll);  
        ...  
    }  
}  
CODE END  
\`AccessFlags\`의 주요 값과 의미는 다음과 같습니다.

| AccessFlags | 의미 | 액션에 미치는 영향 |
| :---- | :---- | :---- |
| Read | 이전 내용을 읽는다 | 로드 액션이 Load가 된다 |
| Write | 기록한다(일부 픽셀일 수 있다) | 이전 내용의 보존이 필요해져 로드가 Load가 될 수 있다 |
| ReadWrite | 읽기/쓰기 모두 | 로드가 Load가 된다 |
| Discard | 이전 내용을 폐기해도 된다 | 로드를 DontCare로 할 수 있다 |
| WriteAll | 모든 픽셀을 기록한다(Write \+ Discard) | 로드를 DontCare로 할 수 있다 |

여기서 특히 주의할 점은 Write와 WriteAll의 차이입니다. 단순한 \`Write\` 신고는 "일부 픽셀에만 쓸 수도 있다"는 신고이므로, Render Graph는 기록되지 않은 픽셀의 내용을 보존하기 위해 로드 액션을 Load로 할 수밖에 없습니다. 전체 화면 Blit처럼 반드시 모든 픽셀을 덮어쓰는 패스에서는 \`WriteAll\` 지정을 사용하시기 바랍니다. 1080p RGBA8이라면 이 지정의 차이만으로 약 8MB/프레임의 로드가 발생하는지 여부가 달라집니다.  
한편 스토어 액션을 결정하는 것은 해당 리소스가 후속 패스에서 사용되는지 여부입니다. 이는 개발자가 지정하는 것이 아니라 Render Graph가 그래프 전체를 보고 판단합니다. 다음 항에서 그 규칙을 살펴보겠습니다.

## **Render Graph가 액션을 결정하는 규칙**

Render Graph는 기록된 모든 패스의 리소스 사용 정보로 Native Render Pass를 구축할 때 각 어태치먼트의 로드/스토어 액션을 결정합니다. 판단 규칙을 정리하면 다음과 같습니다.

### **로드 액션의 결정**

| 조건 | 결정되는 액션 |
| :---- | :---- |
| AccessFlags.WriteAll(또는 Discard)로 신고되었다 | DontCare |
| 리소스의 최초 사용이며 클리어가 요청되었다 | Clear |
| 이전 내용에 대한 읽기/부분 쓰기가 있다 | Load |

### **스토어 액션의 결정**

| 조건 | 결정되는 액션 |
| :---- | :---- |
| 후속 어떤 패스에서도 사용되지 않는다 | DontCare |
| 후속 패스에서 UseTexture(텍스처로 사용)된다 | Store |
| 후속 패스에서 SetInputAttachment(입력 어태치먼트)로만 사용되며 동일한 Native Render Pass 안에 머지되었다 | DontCare(타일 메모리 내에서 전달) |
| 글로벌 텍스처로 설정되어 있다 | Store(후속 사용이 Render Graph에서 보이지 않으므로 보수적으로 스토어) |
| MSAA에서 리졸브 대상이 있다 | Resolve |

이 규칙에서 실무상의 지침을 두 가지 도출할 수 있습니다.

* 글로벌 텍스처화는 최후의 수단으로 삼습니다. \`SetGlobalTextureAfterPass\` 등으로 글로벌화된 텍스처는 Render Graph가 사용 상황을 추적할 수 없게 되므로 항상 스토어되며, 메모리리스화 대상에서도 제외됩니다. 일단 글로벌로 두자는 식으로 작성하면 최적화가 작동하지 않게 됩니다.  
* \`UseTexture\`과 \`SetInputAttachment\`은 대역폭 측면에서 완전히 다릅니다. 같은 "이전 패스의 결과를 읽는다"라도 전자는 스토어→샘플링 왕복이 발생하고, 후자는 타일 메모리 안에서 완결될 수 있습니다("프레임 버퍼 페치" 절에서 자세히 다룹니다).

### **임포트한 텍스처의 취급**

\`RTHandle\` 등 애플리케이션 측에서 관리하는 텍스처를 \`renderGraph.ImportTexture\`로 임포트한 경우, Render Graph는 그 텍스처가 그래프 밖에서 어떻게 사용되는지 알지 못합니다. 따라서 기본적으로 보수적으로 취급됩니다(최초 사용 시에도 로드되고, 마지막 사용 후에도 스토어됩니다).  
임포트 시 \`ImportResourceParams\`를 전달하면 이 보수적인 동작을 완화할 수 있습니다.  
CODE START csharp | 임포트 시 파라미터 설정 {{list:import_params}}  
var importParams \= new ImportResourceParams  
{  
    clearOnFirstUse \= true,   // 첫 사용 시 클리어한다(로드를 Clear로 할 수 있다)  
    discardOnLastUse \= true,  // 마지막 사용 후에는 파괴해도 된다(스토어를 DontCare로 할 수 있다)  
};  
var handle \= renderGraph.ImportTexture(\_rtHandle, importParams);  
CODE END  
임포트한 텍스처를 "매 프레임 다시 만들기 때문에 이전 내용이 불필요"한 경우라면 \`clearOnFirstUse\`/\`discardOnLastUse\`을 설정하시기 바랍니다. 한편 Render Graph가 관리하는 텍스처(\`renderGraph.CreateTexture\`로 생성한 것)라면 이 모든 것이 자동으로 판단됩니다. 라이프사이클 관리가 정말 필요한 텍스처가 아니라면 Render Graph 관리에 맡기는 것이 기본입니다.

### **로드/스토어 액션 읽어 내기**

규칙을 알았으니 실제 프레임을 읽어 보겠습니다. URP의 표준 포워드 렌더링 프레임을 Render Graph Viewer로 보면, 예를 들어 다음과 같은 구조를 관찰할 수 있습니다.

| 패스 | 어태치먼트 | Load | Store |
| :---- | :---- | :---- | :---- |
| MainLightShadow | 섀도 맵(깊이) | Clear | Store |
| DrawOpaqueObjects | 카메라 컬러 | Clear | (후속에 따라 다름) |
|  | 카메라 깊이 | Clear | (후속에 따라 다름) |
| DrawTransparentObjects | 카메라 컬러 | (머지 시 계속) | Store |
| FinalBlit | 백 버퍼 | DontCare | Store |

읽어 낼 때의 포인트는 다음과 같습니다.

* 섀도 맵은 "그리고 나중에 샘플링"하는 것이므로 Store 외에는 될 수 없습니다. 이는 줄일 수 없는 스토어입니다.  
* 불투명→반투명이 하나의 Native Render Pass로 머지되어 있다면 그 사이의 카메라 컬러 스토어/로드는 발생하지 않습니다. 반대로 사이에 무언가가 끼어 머지가 끊기면 거기에 왕복이 생깁니다.  
* 카메라 깊이의 스토어 액션에도 주목하시기 바랍니다. Depth Texture를 켜면 카메라 깊이가 Store되게 됩니다. 셰이더나 포스트 프로세스가 \`\_CameraDepthTexture\`을 요구하지 않는데 설정만 켜져 있는 것은 실제 프로젝트에서 자주 보이는 낭비입니다.

![][image8]  
図: Copy Depth로 인해 Draw Opaque Objects와 Draw Transparent Objects가 분리됨  
![][image9]  
図: 분리되어 있으므로 Draw Opaque Objects 종료 시 Store  
이처럼 각 어태치먼트의 Store에 정당한 이유(후속 사용)가 있는지 하나씩 확인해 나가는 것이 타일 메모리 최적화의 기본 흐름입니다.

## **AccessFlags를 통한 절감 예**

마지막으로 \`AccessFlags\` 지정만으로 액션이 바뀌는 것을 실제 예로 확인합니다. 전체 화면 Blit을 수행하는 패스에서 출력 어태치먼트의 신고를 \`Write\`로 한 경우와 \`WriteAll\`로 한 경우를 비교합니다.  
CODE START csharp | AccessFlags의 Write와 WriteAll 비교 {{list:write_vs_writeall}}  
// 패턴 A: Write(모든 픽셀 기록을 선언하지 않음)  
builder.SetRenderAttachment(\_targetHandle, 0, AccessFlags.Write);

// 패턴 B: WriteAll(모든 픽셀 기록을 선언함)  
builder.SetRenderAttachment(\_targetHandle, 0, AccessFlags.WriteAll);  
CODE END  
렌더링 결과는 둘 다 같습니다. 그러나 Render Graph Viewer로 확인하면 패턴 A에서는 로드 액션이 \`Load\`이고, 패턴 B에서는 \`DontCare\`입니다. 전체 화면 Blit이므로 로드된 내용은 모든 픽셀이 덮어써져 버려집니다. 즉 패턴 A의 로드는 낭비입니다.

# **Render Pass / Subpass**

지금까지 패스와 패스 사이의 스토어→로드 왕복이 비용이 된다는 것을 알아보았습니다. 이 절에서는 그 비용을 없애기 위한 구조인 Render Pass/Subpass와 Render Graph의 패스 머지에 대해 설명합니다.

## **리소스 의존성과 DAG**

게임 화면을 완성하기까지의 한 프레임 공정을 렌더링 파이프라인, 이를 구성하는 개별 처리를 렌더링 패스라고 부릅니다. 각 패스는 "어떤 리소스를 읽고 어떤 리소스에 쓰는가"라는 리소스 의존성을 가집니다.  
예를 들어 디퍼드 렌더링을 포함한 전형적인 지오메트리 렌더링은 다음과 같은 의존 관계를 가집니다.

1.  깊이 프리패스로 깊이 버퍼 생성  
2.  G-Buffer에 불투명 오브젝트 렌더링(깊이 버퍼 사용)  
3. 불투명 버퍼에 디퍼드 라이팅(G-Buffer와 깊이 버퍼 사용)  
4. 반투명 버퍼에 반투명 오브젝트 렌더링(깊이 버퍼 사용)  
5. 불투명 버퍼와 반투명 버퍼를 합성하여 프레임 버퍼에 렌더링

패스를 노드, 리소스 의존성을 에지로 그리면 이 파이프라인은 유향 비순환 그래프(DAG: Directed Acyclic Graph)가 됩니다. 4번 패스는 2번, 3번의 출력을 사용하지 않으므로 의존성만 GPU에 전달되면 병렬 실행이 가능합니다.  
![][image10]  
図: 렌더링 패스의 DAG와 병렬화   
Render Graph라는 이름의 "Graph"는 이 DAG를 가리킵니다. Render Graph는 패스의 리소스 의존성을 수집해 DAG를 구축하고, 이를 바탕으로 실행 계획을 세우는 시스템입니다.

## **Vulkan의 RenderPass/Subpass**

리소스 의존성을 GPU에 전달하는 구조로서 Vulkan에는 RenderPass/Subpass라는 개념이 있습니다. Render Graph가 최종적으로 구축하는 것이 이 Vulkan의 RenderPass이므로, API 수준의 이야기를 조금 하겠습니다.  
Vulkan의 RenderPass는 다음 요소로 구성됩니다.

* 어태치먼트: RenderPass에서 사용하는 렌더 타깃의 선언. 각 어태치먼트에는 \`loadOp\`(로드 액션)과 \`storeOp\`(스토어 액션)을 지정합니다. 앞 절 "타일 메모리의 로드/스토어"에서 본 액션의 정체가 이것입니다.  
* 서브패스: RenderPass 내 렌더링의 단위. 각 서브패스는 출력 어태치먼트(컬러/깊이)와 입력 어태치먼트를 선언합니다.  
* 서브패스 의존성: 서브패스 간 실행 순서와 메모리 의존 관계의 선언.

여기서 알아 두어야 할 성질이 두 가지 있습니다.

1. 로드/스토어는 RenderPass의 경계에서만 발생하며 서브패스 간에는 발생하지 않습니다. 즉 여러 처리를 하나의 RenderPass의 서브패스로 구성할 수 있다면, 그 사이의 중간 결과는 타일 메모리에 남습니다.  
2. 입력 어태치먼트를 사용하면 이전 서브패스의 결과를 같은 픽셀 위치에 한해 읽을 수 있습니다. 이것이 다음 절에서 다룰 프레임 버퍼 페치의 API상 모습입니다.

앞서 본 지오메트리 렌더링을 RenderPass/Subpass로 표현하면 다음과 같습니다.

* 지오메트리 렌더링(RenderPass)  
  *  깊이 프리패스(Subpass) ― 출력: 깊이  
  * G-Buffer 렌더링(Subpass) ― 입력: 깊이 / 출력: G-Buffer  
  * 디퍼드 라이팅(Subpass) ― 입력: G-Buffer / 출력: 불투명 버퍼  
  * 반투명 렌더링(Subpass) ― 입력: 깊이 / 출력: 반투명 버퍼  
  * 합성(Subpass) ― 입력: 불투명/반투명 버퍼 / 출력: 프레임 버퍼

이 구성이라면 깊이, G-Buffer, 불투명 버퍼, 반투명 버퍼는 모두 서브패스 간 전달이며, RenderPass 경계를 넘는 것은 최종 출력인 프레임 버퍼뿐입니다. 스토어가 5회 발생할 수 있던 구성이 스토어 1회가 됩니다.  
참고로 Metal에도 동등한 구조가 있습니다. Metal의 Render Pass는 로드/스토어 액션을 가지며, 타일 메모리 상의 컬러 어태치먼트를 프래그먼트 함수에서 직접 읽는 것(및  
타일 셰이더)이 가능합니다. Vulkan과 같은 명시적인 Subpass 선언은 없지만,  
Render Graph가 수행하는 최적화는 Metal에서도 동일하게 작동합니다.

## **Unity의 Native Render Pass**

그렇다면 Render Graph는 이 Vulkan의 RenderPass/Subpass를 어떻게 다룰까요?  
먼저 알아 두어야 할 것은 \`AddRasterRenderPass\`로 추가한 패스가 그대로 서브패스가 되는 것은 아니라는 점입니다. \`RecordRenderGraph\`로 추가되는 패스는 어디까지나 Render Graph의 논리적인 패스입니다. Render Graph는 프레임 내 모든 패스의 기록이 끝난 뒤 컴파일 단계에서 다음을 수행합니다.

1. 리소스 의존성으로 DAG를 구축하고 불필요한 패스를 제거(컬링)  
2. 인접한 패스 중 조건을 만족하는 것을 하나의 Native Render Pass(Vulkan의 RenderPass에 해당)로 머지하고, 머지된 각 패스를 서브패스로 만듦  
3. 각 어태치먼트의 로드/스토어 액션을 결정(앞 절의 "Render Graph가 액션을 결정하는 규칙" 참조)  
4. 가능한 리소스를 메모리리스화("메모리리스" 절)

즉 개발자 측의 일은 서브패스를 직접 구성하는 것이 아니라, 머지 가능한 형태로 패스를 설계하고 리소스 사용 방식을 올바르게 신고하는 것입니다. 머지의 성패는 Frame Debugger의 \`(RP n:m)\` 표기와 Render Graph Viewer로 확인합니다.

## **패스 머지 조건**

Render Graph가 패스를 머지하는 조건을 정리합니다.

1. 양쪽이 Raster Render Pass입니다. \`AddComputePass\`나 \`AddUnsafePass\`는 머지되지 않습니다. Raster 패스의 열 사이에 Compute 패스가 끼면 거기서 Native Render Pass가 끊깁니다.  
2. 패스가 연속해서 기록되어 있습니다. 머지는 그래프 상에서 인접한 패스끼리 이루어집니다.  
3. 출력 어태치먼트의 해상도가 일치합니다. 중간 버퍼의 해상도를 바꾸면(다운샘플링하면) 그 경계에서는 머지할 수 없습니다.  
4. MSAA 샘플 수가 일치합니다.  
5. 이전 패스의 출력을 이후 패스가 텍스처로 사용하지 않습니다. \`UseTexture\`로 사용하면 스토어→샘플링을 요구하므로 머지를 방해합니다. \`SetInputAttachment\`(입력 어태치먼트)로 사용하는 것은 머지를 방해하지 않습니다. 입력 어태치먼트와 텍스처는 다릅니다.  
6. 어태치먼트의 합계 수가 상한(8장)을 넘지 않습니다. 머지 후 Native Render Pass가 사용하는 어태치먼트의 총수에는 상한이 있습니다.  
7. 글로벌 텍스처화 등 Render Graph의 추적을 벗어나는 사용이 없습니다.

조건을 하나라도 만족하지 않으면 해당 패스는 단독 Native Render Pass가 됩니다. 머지되지 않은 이유는 Render Graph Viewer의 Pass Break Reasoning에 표시되므로, 머지되지 않을 때는 먼저 그곳을 확인하시기 바랍니다.  
![][image11]  
図: 머지 전후 Native Render Pass 구조 비교 

# **프레임 버퍼 페치**

조건을 만족하면 패스를 하나의 Native Render Pass로 머지할 수 있음을 알아보았습니다. 이 절에서는 머지된 서브패스 간에 이전 서브패스의 결과를 타일 메모리에서 직접 읽는 기술인 프레임 버퍼 페치(Frame Buffer Fetch, 이하 FBF)에 대해 설명합니다.

## **프레임 버퍼 페치란**

G-Buffer를 생성하고 라이팅을 수행하는 디퍼드 렌더링을 예로 들어 보겠습니다. 단순하게 구현하면 처리는 다음과 같습니다.

1. G-Buffer 생성(알베도, 노멀, 머티리얼 정보를 타일 메모리에 기록)  
2. 알베도, 노멀, 머티리얼 정보를 메인 메모리에 스토어  
3. 스토어된 G-Buffer를 텍스처로 샘플링하여 디퍼드 라이팅 수행

![][image12]  
図: 단순한 디퍼드 렌더링의 데이터 흐름   
여기서 한 가지 의문이 생깁니다. 라이팅 패스가 읽으려는 것은 "지금 처리 중인 픽셀의" G-Buffer 값입니다. 그 값은 직전 패스가 같은 타일의 타일 메모리에 방금 기록한 것입니다. 메인 메모리를 왕복하지 않고 타일 메모리에서 직접 읽을 수는 없을까요?  
이를 가능하게 하는 것이 FBF입니다. FBF를 사용하면 처리는 다음과 같습니다.

1. G-Buffer 생성(타일 메모리에 기록)  
2. 타일 메모리에서 G-Buffer를 직접 읽어 디퍼드 라이팅 수행

![][image13]  
図: 프레임 버퍼 페치를 사용한 디퍼드 렌더링의 데이터 흐름  
G-Buffer 3장분의 스토어와 이를 샘플링하기 위한 읽기가 사라집니다. 1080p RGBA8×3장이라면 스토어만으로 약 24MB/프레임, 60FPS에서 약 1.4GB/s의 대역폭 절감입니다.  
API 수준에서 FBF는 앞 절 "Vulkan의 RenderPass/Subpass"에서 언급한 Vulkan의 입력 어태치먼트(Metal에서는 컬러 어태치먼트 직접 읽기)로 구현됩니다. 따라서 FBF를 사용할 수 있는 것은 동일한 Render Pass 내의 서브패스 간뿐입니다. 패스가 머지되어 있는 것이 FBF의 전제 조건입니다.

## **제약 이해하기**

FBF는 텍스처 샘플링을 대체하는 것이 아닙니다. 다음과 같은 제약이 있습니다.

1. 처리 중인 프래그먼트와 같은 픽셀 위치만 읽을 수 있습니다. 타일 메모리에는 "지금 처리 중인 타일"의 내용만 있으므로 오프셋 샘플링(주변 픽셀 읽기)은 원리적으로 불가능합니다.  
2. 필터링이나 밉맵은 사용할 수 없습니다. 단일 값의 로드만 가능합니다.  
3. 입력 측과 출력 측의 해상도가 일치해야 합니다(머지 조건이기도 합니다).

제약 1이 작용하는 실제 예를 살펴보겠습니다. "모노크롬/세피아 합성 결과에 UV를 왜곡하는 노이즈 가공을 겹친다"는 처리를 생각해 봅니다. 노이즈 가공 셰이더는 다음과 같이 UV를 어긋나게 하여 샘플링합니다.  
CODE START csharp | 노이즈 가공 셰이더  
// 노이즈를 사용해 UV를 왜곡한다  
float2 distortedUV \= input.texcoord \+ noise \* 0.02;  
// 왜곡한 UV로 텍스처를 샘플링한다.  
// 로드하는 위치가 프래그먼트의 위치와 다르므로 프레임버퍼 페치는 사용할 수 없다.  
half4 color \= SAMPLE\_TEXTURE2D\_X(\_BlitTexture, sampler\_LinearClamp, distortedUV);  
CODE END  
읽으려는 위치가 처리 중인 프래그먼트의 위치와 다르므로 이 패스는 FBF를 사용할 수 없습니다. 합성 결과를 \`UseTexture\`로 신고해야 하며, 합성 패스와 노이즈 패스 사이에서 스토어→샘플링 왕복이 발생합니다.  
따라서 이펙트를 설계하는 단계에서 "이 패스는 같은 픽셀만 읽는가?"를 확인해 두면 FBF를 사용할 수 있는지 일찍 판단할 수 있습니다. 같은 픽셀로 충분한 처리(톤 매핑, 컬러 그레이딩, 합성, 디퍼드 라이팅)는 FBF의 후보가 되고, 주변을 읽는 처리(블러, 왜곡, Bloom)는 텍스처 사용이 됩니다.

## **셰이더 API 내부**

Unity 셰이더에서 FBF를 사용하려면 SRP Core가 제공하는 매크로를 이용합니다.  
CODE START csharp | 프레임 버퍼 페치 사용법  
// 입력 어태치먼트 0번을 선언한다  
FRAMEBUFFER\_INPUT\_X\_HALF(0);

half4 frag(Varyings input) : SV\_Target  
{  
    // 입력 어태치먼트 0번에서 처리 중인 프래그먼트 위치의 값을 로드한다  
    half4 color \= LOAD\_FRAMEBUFFER\_X\_INPUT(0, input.positionCS.xy);  
    ...  
}  
CODE END  
매크로에는 정밀도와 용도에 따른 변형이 있습니다.

| 선언 매크로 | 로드 매크로 | 용도 |
| :---- | :---- | :---- |
| FRAMEBUFFER\_INPUT\_HALF(n) | LOAD\_FRAMEBUFFER\_INPUT(n, pos) | half 정밀도 컬러 |
| FRAMEBUFFER\_INPUT\_FLOAT(n) | LOAD\_FRAMEBUFFER\_INPUT(n, pos) | float 정밀도 |
| FRAMEBUFFER\_INPUT\_X\_HALF(n) | LOAD\_FRAMEBUFFER\_X\_INPUT(n, pos) | XR(싱글 패스 인스턴싱) 대응 버전 |

\`\_X\`가 붙은 변형은 \`TEXTURE2D\_X\` 계열 매크로와 마찬가지로 XR 렌더 타깃 배열에 대응한 것입니다. XR 대응 가능성이 있는 프로젝트에서는 \`\_X\`가 붙은 것으로 통일해 두면 안전합니다.  
이 매크로가 플랫폼별로 무엇으로 전개되는지 알아 두면 동작과 폴백을 이해하기 쉽습니다.

| 플랫폼 | 전개 대상 | 실체 |
| :---- | :---- | :---- |
| Vulkan | subpassInput \+ subpassLoad | Render Pass의 입력 어태치먼트 |
| Metal | \[\[color(n)\]\] 속성에 의한 컬러 어태치먼트 읽기 | 타일 메모리 직접 읽기 |
| OpenGL ES | GL\_EXT\_shader\_framebuffer\_fetch 등의 확장(지원 GPU만) | 확장 미지원 시 아래의 폴백 |
| 미지원 환경 | 일반 텍스처(\_UnityFBInput0 등) + 로드 명령 | FBF가 되지 않고 일반 텍스처 읽기가 된다 |

주의할 점은 마지막 행입니다. 매크로는 미지원 환경에서도 동작하도록 폴백하지만, 그 경우에는 대역폭 절감 효과가 없습니다. 즉 동작한다고 해서 FBF가 효과를 내고 있다고는 할 수 없습니다. 효과가 나오고 있는지는 Render Graph Viewer의 액션 표시(입력 측 버퍼의 스토어가 DontCare로 되어 있는지)와 실기기의 대역폭 계측으로 확인하시기 바랍니다.

## **SetInputAttachment와 UseTexture의 차이**

C\# 측에서는 FBF로 읽으려는 리소스를 \`SetInputAttachment\`로 신고합니다.  
CODE START csharp | 입력 어태치먼트 신고 {{list:input_attachment_usage}}  
using (var builder \= renderGraph.AddRasterRenderPass\<PassData\>("Lighting", out var passData))  
{  
    builder.SetRenderAttachment(cameraColor, 0, AccessFlags.Write);  
    // 셰이더의 FRAMEBUFFER\_INPUT\_X\_HALF(0)에 대응한다  
    builder.SetInputAttachment(gBufferAlbedo, 0);  
    // FRAMEBUFFER\_INPUT\_X\_HALF(1)에 대응한다  
    builder.SetInputAttachment(gBufferNormal, 1);  
    ...  
}  
CODE END  
\`SetInputAttachment\`의 두 번째 인수 인덱스가 셰이더 측 \`FRAMEBUFFER\_INPUT\_X\_HALF(n)\`의 \`n\`에 대응합니다.  
여기서 "타일 메모리의 로드/스토어" 절부터 반복해서 등장한 \`UseTexture\`와의 차이를 표로 정리해 두겠습니다.

| 항목 | UseTexture | SetInputAttachment |
| :---- | :---- | :---- |
| 읽을 수 있는 위치 | 임의의 UV(오프셋, 필터링, 밉 가능) | 처리 중인 프래그먼트 위치만 |
| 이전 패스와의 머지 | 방해한다 | 방해하지 않는다 |
| 이전 패스 출력의 스토어 | Store가 강제된다 | 머지 성립 시 DontCare로 할 수 있다 |
| 대역폭 비용 | 스토어 + 샘플링 읽기 | 0(타일 메모리 내에서 완결) |
| 셰이더 측 읽기 방식 | SAMPLE\_TEXTURE2D 계열 | LOAD\_FRAMEBUFFER 계열 |

## **깊이 페치**

컬러뿐 아니라 깊이도 타일 메모리 상에 있습니다. 깊이를 페치할 수 있다면 소프트 파티클, 데칼, 포그처럼 "씬의 깊이를 참조하는 처리"에서도 \`\_CameraDepthTexture\`의 스토어→샘플링 왕복을 없앨 수 있을 가능성이 있습니다.  
SRP Core에는 깊이용 입력 어태치먼트를 다루기 위한 매크로 모음이 마련되어 있지만, 깊이 어태치먼트의 입력화는 플랫폼과 URP 버전에 따른 제약이 생기기 쉬운 영역입니다.  
검증 결과가 나올 때까지 이 장의 권장 사항은 확실하게 동작하는 다음 대안입니다.  
대안으로, 깊이를 컬러 어태치먼트에 기록하여 페치하는 방법이 있습니다. G-Buffer 구축 서브패스에서 선형화한 깊이 값을 컬러 어태치먼트의 1채널(R16\_Float 등)에 기록하고, 후속 서브패스에서는 그것을 FBF로 읽습니다. 어태치먼트가 한 장 늘지만 메모리리스화되면 실제 메모리 비용은 0이며, 깊이 텍스처의 스토어 왕복을 확실히 없앨 수 있습니다.

## **플랫폼 차이와 함정**

마지막으로 FBF를 사용할 때 확인해 두어야 할 플랫폼 차이를 정리합니다.

| 환경 | FBF 사용 | 비고 |
| :---- | :---- | :---- |
| Vulkan (Adreno) | ○ 입력 어태치먼트 | 폭넓게 지원. 드라이버 품질은 기기에 따라 다름 |
| Vulkan (Mali) | ○ 입력 어태치먼트 | 폭넓게 지원 |
| Vulkan (IMG / Pixel 10\) | ○ 입력 어태치먼트 | TBDR 계열. 드라이버 고유의 동작 차이에 주의(마지막 절의 "GPU 벤더 차이") |
| Metal (Apple) | ○ 컬러 어태치먼트 읽기 | 안정적으로 동작 |
| OpenGL ES | △ 확장 의존 | GL\_EXT\_shader\_framebuffer\_fetch 지원 GPU만. 미지원은 폴백 |
| 에디터 (D3D 등) | × 폴백 동작 | 일반 텍스처로 동작. 동작 확인은 가능하지만 효과 계측은 불가 |

특히 주의할 점은 다음 세 가지입니다.

1. 에디터에서는 효과가 보이지 않습니다. 폴백으로 올바르게 렌더링되어 버리므로, 실기기에서 계측해야 비로소 차이를 알 수 있다는 전제로 워크플로를 구성하시기 바랍니다.  
2. MSAA를 병용하면 머지 조건(샘플 수 일치)에 걸리기 쉬워집니다. MSAA가 켜진 카메라 컬러와 MSAA 없이 만든 중간 버퍼는 머지할 수 없습니다. 중간 버퍼의 TextureDesc는 \`GetTextureDesc(cameraColor)\`처럼 원래 Desc에서 복제하여 샘플 수를 이어받도록 하시기 바랍니다.  
3.  하나의 서브패스에서 읽을 수 있는 입력 어태치먼트 수에는 상한이 있습니다. 어태치먼트 합계 8장 제약(앞 절의 "패스 머지 조건")은 입력 측도 포함한 이야기입니다. G-Buffer를 설계할 때는 채널 패킹으로 장수를 절약하시기 바랍니다.

# **메모리리스**

지금까지 중간 버퍼의 로드도 스토어도 없앨 수 있게 되었습니다. 여기서 한 가지 생각해 볼 것이 있습니다. 로드도 스토어도 하지 않는 버퍼에 메인 메모리 상의 실체가 필요할까요?

## **메모리리스란**

사실 필요하지 않습니다. 타일 메모리 상에서만 사용되고 타일 메모리 상에서 파괴되는 리소스에는 메인 메모리 확보 자체가 필요 없습니다. 이 상태를 메모리리스라고 부릅니다.  
Vulkan/Metal에는 리소스를 "타일 메모리(온칩 메모리)에만 존재하고 메인 메모리의 백킹 스토어를 가지지 않는" 것으로 생성하는 모드가 있습니다(Vulkan에서는 \`LAZILY\_ALLOCATED\` 메모리, Metal에서는 \`MTLStorageMode.memoryless\`). 메모리리스로 지정된 리소스는 다음과 같은 성질을 가집니다.

* 메인 메모리를 소비하지 않는다(앱의 메모리 풋프린트가 줄어든다)  
* 로드 액션에 Load를, 스토어 액션에 Store를 지정할 수 없다(읽고 쓸 실체가 없기 때문)

즉 메모리리스는 지금까지의 절에서 로드/스토어를 없앤 결과로 성립하는 것입니다. 로드/스토어가 사라졌다면 메모리리스화할 수 있고, 메모리리스화할 수 없다면 어딘가에 아직 로드나 스토어가 남아 있는 것입니다.

## **성립 조건**

어떤 리소스를 메모리리스화할 수 있는 조건을 지금까지의 용어로 쓰면 다음과 같습니다.

1. 로드 액션이 Clear 또는 DontCare이다(이전 내용을 메인 메모리에서 읽지 않는다. "AccessFlags를 통한 리소스 사용 신고" 항에서 본 WriteAll/Discard 신고, 또는 최초 사용 시의 Clear)  
2. 스토어 액션이 DontCare이다(후속 패스에서 텍스처로 사용되지 않고 다음 프레임으로도 넘기지 않는다. "Render Graph가 액션을 결정하는 규칙" 항 참조)  
3. 사용이 하나의 Native Render Pass 안에서 완결된다(서브패스 간 전달, 즉 입력 어태치먼트/FBF는 사용해도 된다. "Render Pass / Subpass" 절의 머지, "프레임 버퍼 페치" 절의 FBF)

보시다시피 이 세 가지 조건은 지금까지의 절에서 설명해 온 내용 그 자체입니다.  
![][image14]  
図: 메모리리스화 전후의 메모리 맵 비교

### **자동 메모리리스화와 명시 지정**

Unity의 Render Graph에서 메모리리스는 두 가지 경로로 실현됩니다.

### **자동 메모리리스화**

Render Graph가 관리하는 텍스처(\`renderGraph.CreateTexture\`로 생성)가 "성립 조건" 항의 세 조건을 만족하면 Render Graph가 자동으로 메모리리스화합니다. 개발자는 아무것도 지정할 필요가 없습니다.  
메모리리스화되어 있는지는 Render Graph Viewer의 Resource List에서 확인할 수 있습니다.  
![][image15]  
図: Render Graph Viewer에서의 Memoryless 표시

### **명시 지정**

\`TextureDesc\`의 \`memoryless\` 필드에서 명시적으로 지정할 수도 있습니다.  
CODE START csharp | 메모리리스 명시 지정 {{list:memoryless_explicit}}  
var desc \= renderGraph.GetTextureDesc(universalResourceData.cameraColor);  
desc.memoryless \= RenderTextureMemoryless.Color; // 메모리리스를 지정  
desc.name \= "Intermediate Texture";  
var handle \= renderGraph.CreateTexture(desc);  
CODE END  
명시 지정에는 설계 의도를 코드에 남길 수 있다는 장점이 있습니다. 메모리리스로 지정한 리소스를 실수로 후속 패스에서 \`UseTexture\`하려 하면 그 시점에 구성의 모순이 드러납니다. 자동에 맡겨 두면 리팩터링으로 어딘가에 \`UseTexture\`가 섞여 들어갔을 때 메모리리스가 해제된 것(=메모리 사용량과 대역폭이 늘어난 것)을 알아차릴 수 없습니다. 이 리소스는 타일 메모리 안에서 완결시킨다는 설계 의도가 있다면 명시 지정으로 코드에 남겨 두는 것을 권장합니다.

## **절감량 정량화**

메모리리스화의 절감량은 계산으로 구할 수 있습니다. 1080p에서의 대표 예를 나타냅니다.

| 리소스 구성 | 계산 | 절감량 |
| :---- | :---- | :---- |
| RGBA8 중간 버퍼 1장 | 1920×1080×4B | 약 7.9MB |
| RGBA16F 중간 버퍼 1장 | 1920×1080×8B | 약 15.9MB |
| G-Buffer 3장(RGBA8) + 깊이 | 7.9MB×4 | 약 31.7MB |
| 4xMSAA RGBA8 컬러 | 1920×1080×4B×4 | 약 31.7MB |
| 4xMSAA 깊이(D32) | 1920×1080×4B×4 | 약 31.7MB |

표에서 보듯 절감량이 큰 것은 MSAA와 G-Buffer입니다. 특히 4xMSAA는 컬러와 깊이를 합치면 60MB를 넘으므로, 이것이 사라지는지 여부는 앱 전체의 메모리 버짓에 상당히 큰 영향을 줍니다.  
한편 메모리리스에 의한 절감은 "메모리 사용량"이며, 대역폭 절감은 지금까지의 절에서 수행한 로드/스토어 절감에 의한 것입니다. 두 가지는 별개의 지표라는 점에 주의하시기 바랍니다(메모리리스는 로드/스토어 절감의 결과로 성립하므로 실무에서는 함께 달성됩니다).

## **MSAA 렌더 타깃의 메모리리스화**

메모리리스의 효과가 가장 큰 것은 MSAA입니다. 구조부터 살펴보겠습니다.  
4xMSAA에서는 타일 메모리 상에 픽셀당 4샘플의 컬러와 깊이가 유지됩니다. 그러나 최종적으로 화면에 표시되는 것은 4샘플을 평균(리졸브)한 1샘플분뿐입니다. 여기서 스토어 액션의 선택지가 중요해집니다.

* Store를 선택하면 4샘플분의 데이터가 그대로 메인 메모리에 기록됩니다(대역폭 4배, 메모리 4배).  
* Resolve를 선택하면 타일 메모리 상에서 리졸브가 수행되고, 리졸브 후의 1샘플분만 기록됩니다.

TBR GPU에서는 리졸브를 타일 메모리 상에서 거의 비용 없이 수행할 수 있습니다. 그리고 Resolve를 선택한 경우 4샘플분의 MSAA 버퍼 자체는 스토어되지 않으므로 메모리리스 조건을 만족합니다. 즉 올바르게 구성된 MSAA는 다음과 같은

* MSAA 컬러 버퍼(4샘플) → 메모리리스(메인 메모리 확보 없음, 스토어 없음)  
* 리졸브 대상 버퍼(1샘플) → 일반 스토어  
* MSAA 깊이 버퍼(4샘플) → 메모리리스 + 스토어 없음(깊이를 후속에서 사용하지 않는 경우)

구성이 됩니다. TBR GPU에서 MSAA가 (IMR에 비해) 저비용이라고 하는 것은 이 때문입니다.  
URP에서는 Universal Render Pipeline 에셋의 MSAA 설정을 활성화하면 Render Graph가 이 구성을 자동으로 만듭니다. 확인해야 할 것은 그것이 깨져 있지 않은지입니다. Render Graph Viewer에서 MSAA 컬러 버퍼의 스토어 액션이 Resolve로 되어 있는지, Memoryless가 yes로 되어 있는지 확인하시기 바랍니다. 깨지는 전형적인 예는 MSAA 버퍼 자체를 후속 패스가 텍스처로 요구하는 경우(커스텀 패스에서의 잘못된 신고, Opaque Texture의 구성 실수 등)입니다.

## **GPU 벤더 차이**

이 장에서 다룬 기술은 모두 표준 Vulkan/Metal 기능 위에 성립하며 주요 벤더에서 동작합니다. 그 위에서 벤더별 특성 차이를 파악해 두면 계측 결과의 해석이나 트러블 대응이 쉬워집니다.

| 벤더 | 아키텍처 특성 | 주의점 |
| :---- | :---- | :---- |
| Qualcomm Adreno | GMEM 기반 비닝. 렌더링 내용에 따라 다이렉트 모드(IMR과 유사한 동작)로 전환되는 경우가 있다 | 다이렉트 모드로 전환되면 타일 메모리를 전제로 한 최적화의 효과가 달라진다. 프로파일러로 어느 모드인지 확인한다 |
| ARM Mali | 16×16 타일 기본의 TBR | 외부 대역폭 카운터가 충실하여 이 장의 대역폭 계측의 기준 벤더 |
| Imagination (IMG) | 하드웨어 은면 제거(HSR)를 가진 TBDR. Pixel 10의 IMG DXT가 해당 | HSR 덕분에 불투명 오버드로가 원래 적어, 최적화가 효과를 내는 지점이 다른 벤더와 다르다. 새로운 드라이버이므로 동작 차이 검증을 권장 |
| Apple | PowerVR 계보의 TBDR. Metal에서 타일 메모리를 명시적으로 다룰 수 있다 | FBF/메모리리스가 안정적으로 동작한다. iOS에서 효과를 확인한 뒤 Android 검증으로 넘어가는 것도 유효하다 |

## **마치며**

이 장에서 다룬 네 가지 주제를 마지막으로 다시 한번 정리합니다.  
Render Graph를 사용하는 경우 개발자가 할 일은 리소스 사용 방식을 올바르게 신고하는 것과, Frame Debugger나 Render Graph Viewer로 결과를 확인하는 것의 두 가지입니다. 눈에 띄지 않는 작업이지만, 그 축적이 기기의 발열과 배터리 소모의 차이로 나타납니다. 이 장이 여러분의 프로젝트 최적화에 도움이 되기를 바랍니다.

## **참고 문헌**

\- ARM, "Arm GPU Best Practices Developer Guide", https://developer.arm.com/documentation/101897/latest/  
\- Qualcomm, "Adreno GPU Best Practices", https://docs.qualcomm.com/doc/80-78185-2/topic/mobile\_best\_practices.html  
\- Apple, "Metal Best Practices Guide", https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/  
\- Unity, "Render graph system" (URP Documentation), https://docs.unity3d.com/Manual/urp/render-graph.html  
\- 『학원 아이돌마스터』 모바일에서도 "라이팅과 반사"를 구현하다! 【CEDEC2024】, https://gamemakers.jp/article/2025\_01\_29\_90729/  
\- 모바일에서 초고속 Deferred Rendering을 수행하는 방법, CEDiL, https://cedil.cesa.or.jp/cedil\_sessions/view/2457  
\- Unity6부터 RenderGraph를 제대로 활용하자 ― 기본 기능편, https://blog.sge-coretech.com/entry/2024/06/04/171757  
\- Unity6부터 RenderGraph를 제대로 활용하자! 응용 구현편 1 "RenderPass 구축", https://blog.sge-coretech.com/entry/2024/09/11/105032  
\- 셰이더 최적화 입문 제2회 "타일 기반 렌더링 GPU란?", https://blog.sge-coretech.com/entry/2025/01/27/145242


[image1]: assets/kiyohara-takayuki-1/media/image1.png

[image2]: assets/kiyohara-takayuki-1/media/image2.png

[image3]: assets/kiyohara-takayuki-1/media/image3.png

[image4]: assets/kiyohara-takayuki-1/media/image4.png

[image5]: assets/kiyohara-takayuki-1/media/image5.png

[image6]: assets/kiyohara-takayuki-1/media/image6.png

[image7]: assets/kiyohara-takayuki-1/media/image7.png

[image8]: assets/kiyohara-takayuki-1/media/image8.png

[image9]: assets/kiyohara-takayuki-1/media/image9.png

[image10]: assets/kiyohara-takayuki-1/media/image10.png

[image11]: assets/kiyohara-takayuki-1/media/image11.png

[image12]: assets/kiyohara-takayuki-1/media/image12.png

[image13]: assets/kiyohara-takayuki-1/media/image13.png

[image14]: assets/kiyohara-takayuki-1/media/image14.png

[image15]: assets/kiyohara-takayuki-1/media/image15.png
