//raw[|latex|\reviewcompactschapterhead]
//raw[|latex|\widowpenalty10000]
=[nonum] 과제별 역방향 찾기 가이드

증상별로 먼저 확인해야 할 값과 그것을 다루는 장을 나열했습니다. 먼저 계측 진행 방법을 확인한 뒤, 용량, 메모리, CPU, GPU 순으로 과제에서 찾을 수 있도록 했습니다.

//raw[|latex|\vspace{\baselineskip}]

//indepimage[reader_guide_flow_ko]

//raw[|latex|\clearpage]

==[nonum] 먼저 계측부터

어디가 느린지를 정한 뒤에 손을 댑니다. 추측이 아니라 계측으로 원인을 특정하고 수정 전후의 결과를 비교하는 진행 방식과, 메모리 초과·처리 지연의 분리 방법은 전작 『Unity Performance Tuning Bible』의 제1장 「퍼포먼스 튜닝을 시작하자」에서 해설했습니다. Unity Profiler, Frame Debugger, RenderDoc 등의 기본적인 사용법도 제3장 「프로파일링 도구」에 정리되어 있습니다.

여기서는 전작에서 다루지 않은 계측 수단과, 기존 도구에서 새로 확인할 수 있게 된 항목을 소개합니다.
iOS용이라면 @<b>{Metal Performance HUD}와 @<b>{Performance Trace}를 Development Build 플래그에 의존하지 않고 사용할 수 있으므로, 계측 처리를 아직 내장하지 못한 단계에서나 릴리스에 가까운 구성 그대로에서도 부하의 윤곽을 잡을 수 있습니다.@<br>{}(@<b>{제4부 1장 「Unity에서 활용하는 iOS 표준 퍼포먼스 계측 기능」})

렌더링 쪽은 계측에 앞서 파이프라인의 상태를 확인해 둡니다. @<b>{Frame Debugger}는 이벤트 이름의 @<b>{(RP n:m)} 표기로 패스가 병합되어 있는지를 보여 주고, @<b>{Render Graph Viewer}는 각 어태치먼트의 로드/스토어 액션과 Memoryless 상태에 더해 병합되지 않은 이유(Pass Break Reasoning)까지 표시합니다.
Unity의 표시와 드라이버에 전달된 값을 대조하고 싶을 때는 @<b>{RenderDoc}으로 Vulkan 캡처를 얻어 loadOp / storeOp를 API 수준에서 확인합니다.@<br>{}(@<b>{제1부 1장 「Unity 6 Render Graph 타일 메모리 최적화」})

계측 자체를 매일 돌리는 경우에는, 디바이스 팜에서 빌린 실기기를 앱 내 원격 조작으로 화면 전환시키고, 메모리 스냅샷과 필터링된 Profiler 캡처를 빌드마다 전송하는 구성이 있습니다. 기기 상에서 프레임을 좁히는 구조나 임계값을 정하는 방법, 긴 순회를 기기 용량에 맞추는 방법도 함께 다룹니다.@<br>{}(@<b>{제4부 2장 「데일리 빌드의 실기기 프로파일링을 자동화합니다」})

계측이 아니라 리뷰 단계에서 효과를 내고 싶다면, 이 책의 지식 자체를 AI 코딩 에이전트의 평가 규칙으로 변환하는 방법이 있습니다. 전작의 77개 규칙에 이 책의 86개 규칙을 통합하여 163개 규칙으로 스킬화한 사례를 소개합니다.@<br>{}(@<b>{제4부 3장 「튜닝 바이블을 AI 코딩 에이전트의 스킬로 만들기」})

//raw[|latex|\clearpage]

==[nonum] 용량

앱 용량은 개별 에셋을 줄이기 전에 늘어나는 원인을 분류하는 편이 빨리 멈춥니다. 압축 설정 오류, 과도한 해상도, 불필요한 부수 데이터, 형식 선택 실수의 4가지는 Import 시에 판정할 수 있지만, 묶는 방식의 불일치와 번들 간 중복은 다른 에셋과의 관계로 결정되므로 빌드 전 검사가 필요합니다. 특히 중복은 눈에 보이지 않습니다. Addressables에 등록하지 않은 공유 에셋은 참조하는 번들마다 복제되므로, 4MB 텍스처를 두 Prefab이 참조하는 것만으로 합계 10MB가 되며, 전용 그룹에 등록하면 6MB까지 내려갑니다.@<br>{}(@<b>{제3부 2장 「운영 단계의 에셋 비대화와 Project Auditor를 이용한 조사」})

코드 쪽에도 줄일 여지가 있습니다. Managed Stripping Level을 Minimal에서 High로 올리면, 종료 처리 스크립트를 하나 둔 것뿐인 단순한 씬에서도 매니지드 어셈블리가 약 6.7% 줄었습니다.@<br>{}(@<b>{제3부 4장 「Unity Managed Code Stripping 실전 가이드」})

그런 다음 어떤 에셋이 언제까지 필요한지를 설계로 남깁니다. 로드와 해제의 창구를 기반으로 집약하고, 라이프타임 분류를 AssetBundle의 단위와 맞춰 정해 두면, 수명이 다른 에셋이 같은 번들에 섞이는 상태를 피할 수 있습니다.@<br>{}(@<b>{제3부 1장 「AssetBundle의 라이프타임 관리」})

//raw[|latex|\clearpage]

==[nonum] 메모리

iOS에서는 Xcode 설정을 하나 활성화하는 것만으로 앱에 적용되는 메모리 상한이 올라가는 경우가 있습니다. 제공된 실기기 관측에서는 @<b>{Increased Memory Limit}로 4GB 기기에서 약 240MB, 6GB 기기에서 약 1GB의 여유가 늘었습니다. 다만 3GB 기기에서는 활성화와 비활성화 사이에 차이가 없었습니다. footprint에 여유가 있는데도 할당이 실패한다면 가상 주소 공간 쪽을 넓히는 @<b>{Extended Virtual Addressing}이 후보가 됩니다.@<br>{}(@<b>{제3부 6장 「iOS의 메모리 상한과 가상 주소 공간」})

상주량 자체를 줄이는 쪽에서는 로드한 에셋의 해제 누락을 가장 먼저 의심합니다.@<br>{}(@<b>{제3부 1장 「AssetBundle의 라이프타임 관리」})

비동기 처리에서는 CancellationTokenSource의 Dispose 누락이 링크 원본으로부터의 참조를 남겨, 불필요한 오브젝트가 해제되지 않는 형태로 나타납니다.@<br>{}(@<b>{제2부 5장 「UniTask와 취소 처리의 라이프타임 관리」})

렌더링 쪽 메모리는 구조로 결정됩니다. 애니메이션하는 오브젝트를 16개 표시했을 때 GPU 메모리 증가량은 FBX 방식이 +411.8MB, Skin 방식이 +38.2MB였고, VAT 방식은 거의 늘지 않았습니다.@<br>{}(@<b>{제2부 3장 「VAT를 활용한 본 애니메이션 부하 절감」})

머티리얼 복제도 메모리를 끌어올립니다. 10000개의 Renderer에 각각 머티리얼을 갖게 하면 15.79MB 늘었지만, RSUV로 값을 전달하면 증가량은 1.66KB로 끝났습니다.@<br>{}(@<b>{제1부 5장 「Renderer Shader User Value를 활용한 그래픽스 최적화」})

로드도 스토어도 하지 않는 어태치먼트를 메모리리스로 하면 메인 메모리 상에 실체를 갖지 않아도 됩니다.@<br>{}(@<b>{제1부 1장 「Unity 6 Render Graph 타일 메모리 최적화」})

빌드 설정에서는 Managed Code Stripping으로 미사용 코드를 애초에 메모리에 올리지 않아도 됩니다.@<br>{}(@<b>{제3부 4장 「Unity Managed Code Stripping 실전 가이드」})

//raw[|latex|\clearpage]

==[nonum] CPU

캐릭터를 다수 표시하는 장면에서는 Transform 계층의 취급이 영향을 줍니다. Optimize Game Objects로 계층을 평탄화하면, 300개체 구성에서 메인 스레드의 @<b>{MeshSkinning.Update}가 6.23ms에서 약 1.5ms로 줄었고, 스키닝 행렬 계산은 전체 스레드 합계 36.6ms가 완전히 사라졌습니다. Playable Graph의 수동 업데이트는 그래프 단위로만 병렬화되므로, 600개체를 1개 그래프로 묶으면 8.396ms가 4.500ms가 됩니다. IK는 메인 스레드에서 풀지 않고 Animation Job으로 옮기면, 300개체가 64회씩 푸는 구성에서 9.094ms가 2.795ms가 되었습니다. 효과를 낸 것은 Burst가 아니라 실행 스레드를 옮긴 것입니다.

원경 캐릭터는 N프레임에 1회만 업데이트합니다. @<b>{Animator.Update()}를 개별적으로 호출하는 것보다 @<b>{Animator.enabled} 전환이나 위상별 수동 그래프로 Unity의 일괄 업데이트로 되돌리는 편이 저렴하며, 600개체를 2프레임에 1회로 하면 5.037ms가 3.082ms가 되었습니다. 업데이트할 개체를 프레임마다 어긋나게 해 두면, 4프레임에 1회 업데이트에서도 p95가 9.470ms에서 2.707ms로 수렴하여 스파이크가 되지 않습니다.@<br>{}(@<b>{제2부 1장 「Unity 3D 캐릭터 애니메이션 튜닝」})

본 업데이트 자체를 사전 계산으로 대체하는 VAT에서는 16개 표시 시 CPU 메인 스레드가 154.3ms에서 12.6ms가 되었습니다. 인스턴싱으로 드로우 커맨드가 프레임당 약 21만 개에서 99개로 줄고 SRPBatcher.Flush의 76.17ms가 거의 사라진 것이 효과를 냈습니다.@<br>{}(@<b>{제2부 3장 「VAT를 활용한 본 애니메이션 부하 절감」})

풀에서 돌아온 Animator의 초기화도 이 설계에 포함하여 생각합니다.@<br>{}(@<b>{제2부 2장 「ResetControllerState로 Animator 풀링을 안전하게 수행하기」})

단발성의 큰 스파이크는 다른 방식으로 찾습니다. Noise 모듈을 활성화한 ParticleSystem에 @<b>{ParticleSystem.Simulate(20f, false, false)}처럼 큰 초 값을 전달하면 NoiseJob이 약 2000회 호출되어 약 50ms의 스파이크가 발생했습니다.@<br>{}(@<b>{제2부 4장 「ParticleSystem에서 긴 시간을 Simulate하면 거대한 스파이크가 발생한다」})

//raw[|latex|\clearpage]

렌더링의 등록 비용도 CPU 부하입니다. 병합할 수 없는 Pass 체인을 하나의 UnsafePass로 묶으면 RasterRenderPass 1개당 약 1.8µs의 고정 비용을 절감할 수 있습니다. 100패스 핑퐁 Blur에서는 1080.7µs가 898.5µs가 되었습니다.@<br>{}(@<b>{제1부 2장 「RenderGraph의 UnsafePass를 사용해야 하는 상황」})

대량 렌더링에서는 머티리얼을 복제하지 않고 오브젝트별 값을 전달할 수 있는 RSUV가 효과적입니다. iPhone 12에서 Sphere 10000개의 파라미터를 매 프레임 바꾸는 계측에서, Renderer별 머티리얼은 8.1FPS였던 반면 RSUV와 GPU Resident Drawer를 병용하면 60.0FPS를 유지했습니다.@<br>{}(@<b>{제1부 5장 「Renderer Shader User Value를 활용한 그래픽스 최적화」})

UI에서는 요소를 하나 바꾸면 Canvas 전체의 메시가 다시 만들어지는 uGUI에 비해, UI Toolkit은 변경된 부분만 업데이트합니다.@<br>{}(@<b>{제2부 6장 「UI 시스템의 구분 사용 — IMGUI / uGUI / UI Toolkit」})

C# 코드 전체에 효과를 내는 수단으로는 LTO가 있습니다. Android에서는 Release 빌드에 @<b>{-flto=thin}을 추가하는 것만으로 벤치마크의 직접 호출이 21.8% 단축되어, Master(-O2)로 전환했을 때의 5.9%를 크게 웃돌았습니다. 실제 타이틀의 정상 화면에서는 UnityMain이 3.21% 단축되었습니다.@<br>{}(@<b>{제3부 5장 「Unity의 LTO 설정과 Android·iOS에서의 성능 비교」})

//raw[|latex|\clearpage]

==[nonum] GPU

모바일에서는 GPU의 연산 성능보다 먼저 메모리 대역폭이 병목이 됩니다. 1080p의 RGBA8을 1회 스토어하고 다음 패스에서 로드하는 것만으로 왕복 약 16MB, 60FPS라면 약 950MB/s를 소비합니다. 이를 줄이는 수단이 로드/스토어 액션의 명시, 패스 병합, 프레임 버퍼 페치, 메모리리스의 4가지이며, 모두 리소스 사용 방식을 Render Graph에 올바르게 전달하는 것이 조건입니다.

예를 들어 모든 픽셀을 반드시 덮어쓰는 패스는 @<b>{AccessFlags.WriteAll}로 선언하는 것만으로 1080p에서 약 8MB/프레임의 로드가 사라집니다. 디퍼드의 G-Buffer를 프레임 버퍼 페치로 읽으면 3장분의 스토어만으로 약 24MB/프레임, 60FPS에서 약 1.4GB/s의 대역폭을 절감할 수 있습니다. MSAA는 Store가 아니라 Resolve로 하면 타일 메모리 상에서 해결되어 4샘플분의 버퍼를 메모리리스로 만들 수 있습니다.@<br>{}(@<b>{제1부 1장 「Unity 6 Render Graph 타일 메모리 최적화」})

마지막에 1회만 실행되는 풀스크린 Blit도 고해상도에서는 무시할 수 없는 비용입니다. AfterRenderingPostProcessing 이후에 커스텀 패스를 주입하면 URP의 FinalBlitPass가 거의 반드시 추가되지만, Unity 6.3부터는 마지막 카메라의 최종 패스에서 @<b>{SwitchActiveTexturesToBackbuffer()}를 호출하면 BackBuffer로 직접 출력하여 생략할 수 있습니다. 커스텀 FXAA 예에서는 Unity 6.0에서 2회 필요했던 Blit이 1회가 되었고 중간 텍스처도 불필요해졌습니다.@<br>{}(@<b>{제1부 3장 「URP의 FinalBlitPass를 생략하는 방법」})

셰이더 내부에서는 초월 함수가 눈에 띕니다. sin은 곱셈과 @<b>{abs()}만의 포물선 근사로 대체할 수 있으며, 10회 실행 시 iPhone X에서 약 2.5ms, Galaxy S9에서 약 1.3ms의 GPU 시간을 절감했습니다. tan의 근사는 10회 실행 시 GPU 시간이 원래의 80%(iPhone 15 Pro)나 50%(iPhone 14)가 되었습니다. 다만 실행 횟수가 적을 때는 레지스터 압박에 의한 오버헤드가 눈에 띕니다.@<br>{}(@<b>{제1부 4장 「모바일용 셰이더 최적화: 초월 함수의 계측과 경량화」})

//raw[|latex|\clearpage]

첫 렌더링 시의 스터터는 GPU 측 컴파일이 원인입니다. Metal이나 Vulkan에서는 1회의 PSO 생성에 수십 밀리초가 걸리는 경우가 있으며, GraphicsStateCollection으로 임의의 타이밍에 끝내 둡니다. @<b>{WarmUpProgressively()}로 프레임당 건수를 지정하면 웜업 자체의 스파이크도 피할 수 있습니다. 도입 후에도 스터터가 남는다면 Shader Stripping에 의한 베리언트 누락이나 메타데이터 불일치를 의심합니다. iOS에서는 셰이더를 하나의 번들로 묶어 이중으로 로드되는 것을 방지하고, SubPass를 사용하는 셰이더는 로딩 화면에서 한 번 렌더링하여 PSO를 만들어 둡니다.@<br>{}(@<b>{제1부 6장 「GraphicsStateCollection을 이용한 셰이더 워밍업」})

그래도 해결되지 않는 경우에는 기기에 따라 렌더링 설정 자체를 전환합니다. @<b>{deviceModel}, @<b>{graphicsDeviceName}, @<b>{systemMemorySize}로 기기를 판정하여 Quality Level을 전환하거나, RenderPipelineAsset의 renderScale이나 그림자 캐스케이드 수를 직접 조작합니다. 자체 제작한 RendererFeature에도 임시 텍스처의 포맷이나 해상도를 낮추기, Blit 수 줄이기, 간단한 셰이더로 교체하기와 같은 전환 수단을 마련해 두면 같은 구조로 부하를 조정할 수 있습니다.@<br>{}(@<b>{제3부 3장 「기기에 따른 렌더링 설정 전환」})

//raw[|latex|\widowpenalty4000]
