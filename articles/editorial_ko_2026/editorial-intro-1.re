=[nonum] 2026년의 Unity 그래픽스 동향
#@# 原稿は sources/editorial-intro-1.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

==[nonum] Render Graph의 정식 릴리스

Unity 6가 정식 출시된 지 이 글을 집필하는 시점에서 약 2년이 지났습니다. Unity 6 릴리스의 주요 특징 중 하나가 Render Graph의 등장이었습니다. Render Graph는 Frostbite(DICE)가 2017년에 발표한 Frame Graph나 Unreal Engine의 RDG(Render Dependency Graph)처럼, 현재의 모던 렌더링 엔진에 널리 도입되어 있는 방식, 즉 1프레임의 리소스 의존 관계를 선언적으로 기술하고 실행 전에 그래프 전체를 최적화하는 개념에 기반한 구조입니다.

리소스 의존성을 기술함으로써 렌더 텍스처의 재사용, 불필요한 렌더 텍스처의 파괴, 적절한 메모리 로드·스토어의 자동 설정 등 다양한 이점을 얻을 수 있습니다.

Unity에서는 HDRP에 먼저 도입되었지만 URP에는 도입되지 않았으며, 마침내 Unity 6와 함께 기다려 온 릴리스가 이루어졌습니다.

이 책의 제1부 1장 「Unity 6 Render Graph 타일 메모리 최적화」에서는 Render Graph의 성능을 최대한 끌어내는 데 필요한 TBDR 아키텍처 GPU와 Render Graph의 관련성을 풀어 갑니다.

이어지는 제1부 2장 「RenderGraph의 UnsafePass를 사용해야 하는 상황」에서는 Render Graph의 이점이 약해지는 Unsafe Pass를 해설합니다. Render Graph는 보통 Raster Render Pass나 Compute Pass를 사용함으로써 Render Graph에 의한 최적화의 이점을 얻을 수 있습니다. 많은 경우 Raster Render Pass나 Compute Pass를 선택하는 것이 최선입니다. 그중에서 굳이 Unsafe Pass를 선택해야 하는 상황을 소개합니다. 퍼포먼스 향상을 위해, 이식을 위해, Unsafe로만 구현할 수 있는 기능을 구현하기 위해 Unsafe Pass를 적절히 선택하는 경우를 소개합니다.

==[nonum] Render Graph의 등장으로 모든 것이 바뀐 것인가

이처럼 Render Graph의 등장으로 렌더링 시스템은 크게 바뀌었습니다. Render Graph는 자동으로 최적화해 주지만, 구조를 이해하지 않고 사용하면 최적화의 이점을 최대한 살릴 수 없습니다. 새로운 렌더링 시스템을 이해하고 다룰 책임은 우리 개발자에게 있습니다.

한편 Render Graph의 등장으로 지금까지의 최적화 개념이 모두 뒤집혔는가 하면 그렇지는 않습니다. Render Graph는 그래픽스 영역에서 높은 레이어의 이야기입니다. 하드웨어에 더 가까운 낮은 레이어로 눈을 돌리면 예전부터의 지식은 여전히 유효합니다. 예를 들어 셰이더에서는 sin, cos, tan, pow 등 초월 함수의 연산 처리는 비용이 높다는 이야기가 있습니다. 이런 이야기는 현대의 모바일 환경에서도 똑같이 적용됩니다. 제1부 4장 「모바일용 셰이더 최적화: 초월 함수의 계측과 경량화」에서는 현대의 모바일 환경에서 초월 함수를 사용했을 때의 계측 결과의 한 예를 제시합니다. 그 위에서 초월 함수의 경량화 방법으로 몇 가지 근사 함수를 소개합니다. 저 자신도 연산 성능이 향상된 현대의 모바일 환경에서도 근사 함수가 필요한가 하는 의문을 가진 적이 있습니다. 그런 의문에 대한 하나의 답을 보여 줍니다. 제1부 5장 「Renderer Shader User Value를 활용한 그래픽스 최적화」와 제1부 6장 「GraphicsStateCollection을 이용한 셰이더 워밍업」에서 소개하는 기능도 Unity 6 이전부터 남아 있던 문제를 해결하기 위해 추가된 기능입니다.

==[nonum] 대량 오브젝트 렌더링의 부하

Renderer Shader User Value(RSUV)는 Unity 6.3부터 추가된 새로운 기능입니다. 이 기능이 해결하려는 문제는 예전부터 있던 대량의 오브젝트를 렌더링할 때의 CPU 부하, 특히 Set Pass Call의 부하입니다. 이 문제를 해결하는 수단으로 SRP Batcher나 GPU 인스턴싱 등이 있습니다.

그러나 SRP Batcher에는 「오브젝트마다 외형을 조금만 바꾸고 싶다」는 상황에서 제약이 있었습니다. SRP Batcher는 MaterialPropertyBlock을 사용한 렌더러별 프로퍼티 변경을 지원하지 않기 때문에, SRP Batcher를 유지한 채 개별 파라미터를 전달하려면 커스터마이즈하려는 렌더러의 수만큼 Material을 복제해야 했습니다. 약간의 파라미터 차이 때문에 Material을 늘리는 것은 메모리 측면에서도 관리 측면에서도 바람직하지 않습니다.

RSUV는 이 문제에 대한 더 가벼운 선택지입니다. RSUV를 사용하면 렌더러마다 32비트 부호 없는 정수를 하나 갖게 하여 셰이더 쪽에서 참조할 수 있습니다. 이 값을 설정해도 배칭이 깨지지 않으므로 SRP Batcher를 유지한 채 파라미터를 변경할 수 있습니다. 전달할 수 있는 것이 32비트 값 하나라는 제한은 있지만, 색상이나 인덱스, 플래그 등을 비트 패킹하여 담으면 많은 용도를 커버할 수 있습니다.

==[nonum] 셰이더 컴파일 시의 스파이크

GraphicsStateCollection을 이용한 셰이더 웜업이 해결하는 것은 셰이더 컴파일 시의 스파이크 문제입니다. GraphicsStateCollection은 Unity 6에서 Experimental(실험적 기능)로 추가되어 Unity 6.5에서 정식 기능으로 릴리스되었습니다.

기존의 ShaderVariantCollection.WarmUp 등 웜업 API는 Vulkan/Metal 등에서는 기대대로 동작하지 않았습니다. 이는 Vulkan/Metal 등의 셰이더 웜업 구조를 알면 이해할 수 있는데, 기존 API로는 올바르게 웜업하는 데 필요한 파라미터를 전달할 수 없었습니다. 그래서 Vulkan/Metal에서는 웜업을 완전하게 수행할 수 없었습니다. 따라서 개발자는 게임플레이 중의 스파이크를 피하기 위해 화면이 암전된 동안 뒤에서 렌더링하는 등 임시방편적인 웜업을 해야 했습니다.

GraphicsStateCollection은 이 문제를 해결하는 새로운 API입니다. GraphicsStateCollection은 셰이더 웜업에 필요한 버텍스 레이아웃 등 다양한 정보를 앱 실행 시에 수집하기 위한 구조입니다. 이러한 정보를 수집함으로써 셰이더를 완벽하게 웜업할 수 있습니다. 그러나 GraphicsStateCollection에는 정보 수집 누락 등 몇 가지 새로운 문제가 있습니다. 이는 GraphicsStateCollection 자체의 문제라기보다 Unity의 Shader Variant Strip에서 기인하는 것이나 Vulkan/Metal 등 현재의 모던 그래픽스 API의 사양에서 기인하는 것 등 다양합니다.

==[nonum] 제1부를 앞두고

Render Graph의 등장으로 렌더링 파이프라인을 구축하는 시스템은 크게 바뀌었습니다. 리소스 의존성을 정의함으로써 불필요한 리소스를 파괴할 수 있고, 타일 기반 GPU와의 궁합도 좋은 새로운 렌더링 시스템의 등장입니다.

그러나 높은 레이어의 렌더링 시스템이 새로워졌다고 해서 하드웨어에 더 가까운 레이어의 구조가 바뀐 것은 아닙니다. Render Graph, GraphicsStateCollection, RSUV가 해결하려는 문제는 이전부터 안고 있던 문제입니다.

제1부에서는 Unity 6에서 도입된 새로운 기능, 보편적인 내용, 그리고 제1부 3장 「URP의 FinalBlitPass를 생략하는 방법」처럼 내일부터 바로 쓸 수 있는 팁 성격의 내용도 소개합니다.

이 내용이 여러분의 개발에 도움이 되기를 바랍니다.
