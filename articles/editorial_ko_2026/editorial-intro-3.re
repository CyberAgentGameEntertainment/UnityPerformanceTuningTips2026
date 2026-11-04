=[nonum] 2026년의 에셋·설정·빌드 동향
#@# 原稿は sources/editorial-intro-3.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

퍼포먼스 튜닝은 자신들의 코드나 에셋을 고치는 것만이 아닙니다. 같은 코드와 에셋이라도 엔진 버전이나 설정 하나에 따라 빌드 시간, 앱 용량, 메모리 사용량, 실행 속도가 달라집니다. 엔진 쪽에서 이미 해결된 문제를 자체 구조로 계속 안고 있지는 않은지, 새로 추가된 설정을 모른 채 기본값으로 사용하고 있지는 않은지. 이를 확인하기 위해서도 엔진 자체의 변화를 따라가는 것은 튜닝의 전제가 됩니다.

Unity에게도 퍼포먼스는 엔진으로서 우선순위가 높은 영역입니다. Unity 6 세대 각 릴리스의 What's New에는 신기능과 함께 빌드 시간 단축, 플레이어 용량 절감, 메모리 관리 개선이 매번 올라와 있습니다. 제3부에서 다루는 에셋·설정·빌드는 그 변화가 특히 큰 영역입니다.

집필 시점인 2026년 9월 기준으로 장기 지원 버전의 최신은 Unity 6.3 LTS, 정식 버전의 최신은 Unity 6.6입니다. Unity 6.7은 LTS가 될 예정인 버전으로 베타 버전이 공개되어 있으며@<fn>{fn1}, Unity 7은 2026년 12월에 초기 베타 버전, 2027년 1분기에 정식 버전 릴리스가 예정되어 있습니다@<fn>{fn2}. 이후에는 Unity 6.0부터 6.7까지의 변화를 릴리스 순서대로 따라가며, 같은 버전 안에서는 효과가 큰 것부터 다룹니다. 제3부의 각 장에서 자세히 다루는 변화는 요점만 언급하고 해당 장을 안내합니다. 또한 본문에서 언급하는 Unity의 버전이나 기본값은 집필 시점의 것이며 이후 릴리스에서 바뀔 수 있음을 양해해 주시기 바랍니다.

==[nonum] Unity 6.0

=== iOS 메모리 상한을 API로 설정하기

Increased Memory Limit과 Extended Virtual Addressing은 앱이 사용할 수 있는 메모리 상한 자체와 관련된 iOS의 엔타이틀먼트입니다. 이전 Unity에서도 내보낸 Xcode 프로젝트의 Entitlements 파일을 수동으로 편집하거나 빌드 후처리에서 수정하면 설정할 수 있었습니다. Unity 6000.0.11f1 이후에서는 @<tt>{ProjectCapabilityManager}에 전용 API가 추가되어, Entitlements에 대한 추가와 Xcode 프로젝트로의 반영을 한꺼번에 수행할 수 있습니다. 각각의 역할과 실기기에서 상한을 확인하는 방법은 제3부 6장 「iOS의 메모리 상한과 가상 주소 공간」에서 다룹니다.

=== Build Profiles로 구성별 설정 갖기

Unity 6.0에서 도입된 Build Profiles는 하나의 플랫폼에 대해 여러 빌드 구성을 가질 수 있는 구조입니다. 도입 시점부터 프로파일마다 씬 목록, Player Settings 덮어쓰기, 스크립트 Define Symbols 추가를 가질 수 있습니다. Unity 6.1부터는 Graphics 설정과 Quality 설정을 프로파일별로 덮어쓸 수 있게 되었고@<fn>{fn3}, 6.3에서는 덮어쓸 항목을 선택하여 추가하는 형태가 되었습니다. 6.5에서는 @<tt>{BuildProfile.CreateBuildProfile}로 스크립트에서 프로파일을 생성할 수도 있습니다.

퍼포먼스 측면의 이점은 플랫폼이나 배포처별 설정 차이를, 빌드 전에 설정을 수정하는 스크립트가 아니라 프로파일로 고정해 둘 수 있다는 점입니다. 예를 들어 Define Symbols를 나누면 디버그용 계측이나 로그 코드를 릴리스용 프로파일에서만 제외할 수 있고, Quality 설정을 덮어쓰면 로우엔드용 구성에서는 빌드에 포함하는 품질 레벨 자체를 좁힐 수 있습니다. 제3부 3장 「기기에 따른 렌더링 설정 전환」은 실행 시 기기 성능에 맞춰 렌더링 부하를 분배하는 설계를, Quality Level을 전환하는 방식과 Render Pipeline Asset의 값을 직접 수정하는 방식으로 비교합니다. Build Profiles는 이를 빌드 시점에 전환하는 수단으로 조합할 수 있습니다.

=== Managed Stripping Level의 Low가 지원 중단 예정으로

Unity 6.0 매뉴얼에서는 Managed Stripping Level의 Low가 향후 폐지 대상으로 되어 있으며 Minimal이나 Medium을 사용하도록 안내하고 있습니다@<fn>{fn4}. Low를 사용하던 프로젝트에서는 Medium으로 올려 바이너리를 작게 할지, Minimal로 내릴지를 선택하게 됩니다. Medium으로 올리면 리플렉션으로 참조되는 코드가 제거되기 쉬워지며, 그 대처는 제3부 4장 「Unity Managed Code Stripping 실전 가이드」에서 다룹니다.

==[nonum] Unity 6.1

=== 기기별로 Graphics Jobs 모드 선택하기

Unity 6.1에서 추가된 Vulkan Device Filter Asset에서는 Android 기기별로 Graphics Jobs 모드를 지정할 수 있습니다. 특정 GPU나 드라이버에서만 느려지거나 불안정해지는 설정을, 다른 기기의 설정을 바꾸지 않고 피할 수 있습니다.

==[nonum] Unity 6.2

=== 임포트 시 자동 생성되는 Mesh LOD

Unity 6.2에서 추가된 Mesh LOD는 모델의 임포트 설정에서 Generate Mesh LODs를 활성화하면 임포트 시에 LOD를 자동 생성하는 기능입니다@<fn>{fn5}. LOD를 준비하지 않았던 모델도 설정 하나로 원경의 폴리곤 수를 줄여 GPU 부하를 낮출 수 있습니다. 이미 LOD Group을 사용하고 있다면 폴리곤을 줄이는 효과는 같고, 차이가 나는 것은 메모리입니다. Mesh LOD는 버텍스를 추가하지 않고 모든 LOD가 원본 메시의 버텍스 버퍼를 공유하며 인덱스 버퍼만 늘어납니다. LOD마다 메시 전체와 GameObject를 갖는 LOD Group보다 메모리 증가를 작게 억제할 수 있습니다.

다만 자동 생성되는 LOD의 품질은 원본 메시의 형상에 좌우되며, 수작업으로 만든 LOD와 같은 품질은 기대할 수 없습니다. 공식 문서에서도 생물이나 바위 같은 유기적인 형상에는 적합한 한편, 곡률이 높은 형상이나 떨어진 파트가 많은 메시에서는 텍스처 어긋남이나 나무 잎이 사라지는 등의 깨짐이 발생하는 예가 제시되어 있습니다. 생성된 LOD는 Inspector에서 단계별로 확인하고, 깨지는 단계부터는 Limit LODs로 생성을 멈추거나 해당 메시만 LOD Group으로 되돌리는 등의 판단이 필요합니다.

또한 줄일 수 있는 것은 폴리곤뿐이며 머티리얼은 바뀌지 않습니다. Particle System, VFX Graph, Static Batching에서는 항상 LOD0이 사용되므로 효과가 없고, Skinned Mesh Renderer에서는 어떤 LOD가 선택되어 있더라도 변형 계산은 LOD0에 대해 수행됩니다. LOD의 크로스페이드에는 GPU Resident Drawer가 필요하며, LOD Group과의 병용은 권장되지 않습니다.

==[nonum] Unity 6.3

=== 셰이더 베리언트를 설정으로 줄이기

셰이더 베리언트는 키워드 조합마다 컴파일되므로 키워드가 늘어날수록 곱셈으로 늘어납니다. 베리언트를 줄이면 빌드 시간, 빌드 용량, 셰이더 로드 시간, 실행 시 메모리가 함께 줄어듭니다.

Unity 6.3에서 추가된 Shader Build Settings에서는 셰이더 소스를 수정하지 않고 키워드의 종류를 @<tt>{shader_feature}나 @<tt>{dynamic_branch}로 덮어쓰거나, 특정 키워드의 코드를 빌드에서 제외할 수 있습니다@<fn>{fn6}. 이 설정은 Build Profile별로 덮어쓸 수 있으므로 플랫폼별로 불필요한 베리언트만 제거할 수 있습니다. 빌트인 키워드를 제외하면 필요한 베리언트까지 사라질 수 있으므로, 누락을 감지할 수 있는 Strict Shader Variant Matching과의 병용도 필수입니다. 빌드에 남은 베리언트의 실행 시 메모리는 2021 LTS부터 사용할 수 있는 Shader Variant Loading 설정으로, 로드하는 청크의 크기와 수를 제한하여 억제할 수 있습니다@<fn>{fn7}.

==[nonum] Unity 6.4

=== 에디터에 포함된 Project Auditor

Unity 6.4에서는 Project Auditor가 에디터에 포함되었습니다. Addressables의 번들 간 중복과 같은 비대화를 기계적으로 찾아낼 수 있습니다. 6.4 이후에는 진단 규칙이 별도 패키지로 분리되어 있어 도입 절차가 버전에 따라 다릅니다. 조사 절차는 제3부 2장 「운영 단계의 에셋 비대화와 Project Auditor를 이용한 조사」에서 다룹니다.

=== Physics 모듈 비활성화하기

Unity 6.4에서는 내장 Physics 모듈을 Package Manager에서 비활성화할 수 있게 되었습니다@<fn>{fn8}. 3D 피직스를 사용하지 않는 타이틀에서는 그만큼 빌드 용량을 줄일 수 있습니다. 다만 Local Volume을 사용하는 경우에는 활성화한 채로 두어야 합니다.

==[nonum] Unity 6.5

=== Read/Write의 암묵적 활성화가 사라졌다

텍스처나 메시의 Read/Write를 활성화하면 GPU에 전송한 데이터와 별도로 CPU 측에도 사본이 남아 그만큼 메모리를 소비합니다. 지금까지의 Unity는 Particle System의 Shape 모듈이나 Terrain의 디테일, Mesh Collider가 CPU에서의 읽기를 필요로 하는 경우에 빌드 시 이 설정을 암묵적으로 활성화했습니다. Unity 6.5에서는 텍스처, Unity 6.6에서는 메시에 대해 이 암묵적 활성화가 사라져, 필요한 에셋에서 명시적으로 활성화하지 않으면 Inspector에 경고가 표시되고 빌드가 실패하게 되었습니다.

공식 문서는 이 변경의 이유로 빌드 시간 단축을 들고 있지만 메모리 측면에서도 의미가 있습니다. 어떤 에셋이 CPU 측에 사본을 갖는지가 에셋 설정으로 보이게 되므로, 정말로 CPU에서의 읽기가 필요한지를 하나씩 판단할 수 있고 불필요한 사본을 줄이는 단서가 됩니다.

=== Android의 LTO

Unity 6.5에서는 Android에 링크 타임 최적화(LTO) 설정이 추가되었습니다. 이 토글이 효과를 내는 것은 엔진 측 라이브러리뿐이며 Strip Engine Code의 비활성화를 전제로 하므로, 실행 속도 개선과 용량 증가를 비교하여 채택을 판단하게 됩니다. 실측 결과는 제3부 5장 「Unity의 LTO 설정과 Android·iOS에서의 성능 비교」에서 다룹니다.

=== mimalloc의 추가

Unity 6.5에서는 네이티브 메모리 얼로케이터로 mimalloc이 추가되었습니다. mimalloc은 여러 스레드가 동시에 메모리를 할당·해제할 수 있는 얼로케이터로, 6.5에서는 선택제였지만 Unity 6.6에서는 기본으로 활성화되어 있습니다@<fn>{fn9}. 잡이나 워커 스레드에서 네이티브 메모리 할당이 겹치는 상황에서는 스레드 간 대기 감소를 기대할 수 있습니다. 한편 얼로케이터가 바뀌면 메모리 사용량이나 단편화 경향도 바뀌며, 공식적으로는 효과의 수치도 제시되어 있지 않습니다. 6.6으로 업데이트할 때는 네이티브 메모리의 추이를 업데이트 전과 비교해 두는 것이 안전합니다. 문제가 있으면 Memory Settings에서 비활성화할 수 있습니다.

=== iOS의 열 상태에 따른 목표 프레임 레이트

Unity 6.5에서는 iOS의 열 상태가 Serious나 Critical이 되었을 때의 목표 프레임 레이트를 Player Settings에서 지정할 수 있게 되었습니다. 발열 시 프레임 레이트를 낮추는 처리를 직접 작성하지 않고도 서멀 스로틀링으로 인한 급격한 성능 저하를 완화할 수 있습니다.

=== AssetBundle의 TypeTree 분리하기

AssetBundle에는 직렬화된 데이터의 타입 정보인 TypeTree가 포함되어 있습니다. TypeTree 덕분에 번들을 빌드했을 때와 플레이어의 Unity 버전이 달라도 로드할 수 있지만, 같은 타입의 정보가 번들마다 중복해서 포함되어 용량을 키웁니다. Unity 6.5와 Addressables 2.9.1 이후에서는 Extract TypeTree Data를 활성화하면 TypeTree를 별도 파일로 분리하여 AssetBundle의 용량을 줄일 수 있습니다@<fn>{fn10}. 배포하는 번들이 많은 타이틀일수록 다운로드량과 기기 스토리지 양쪽에 효과가 있습니다. 다만 이 설정은 이미 배포 중인 모든 AssetBundle을 변경하므로, 공식적으로는 신규 프로젝트에서의 사용이 권장됩니다.

==[nonum] Unity 6.6

=== AssetBundle의 후속인 Content Directories

Unity 6.6에서는 AssetBundle의 후속이 되는 새로운 콘텐츠 빌드 구조로 Content Directories가 추가되었습니다@<fn>{fn11}. 공식 문서가 드는 이점은 암묵적 중복 제거, 로드 시간 단축, 메모리 오버헤드 절감입니다. AssetBundle에서는 여러 번들에서 참조되는 에셋이 각 번들에 복제되어 용량과 메모리를 이중으로 소비하는 경우가 있었습니다. 제3부 2장 「운영 단계의 에셋 비대화와 Project Auditor를 이용한 조사」에서 다루는 번들 간 중복은 Content Directories에서는 자동으로 제거됩니다. 에셋의 로드와 해제는 @<tt>{Loadable<T>} API로 수행하며 async/await도 지원합니다. Addressables 4.0에서는 Content Directory용 스키마가 추가되어 기존 그룹을 변환할 수 있습니다@<fn>{fn12}.

다만 Content Directories가 현 시점에서 지원하는 것은 로컬 콘텐츠 빌드뿐이며 원격 배포는 지원하지 않습니다. 베타 버전인 Unity 6.7에서도 마찬가지이며, 원격 콘텐츠 지원은 Unity 7 세대에서 예정되어 있습니다@<fn>{fn13}. Addressables 4.0에서도 원격 로드 경로를 가진 그룹을 변환하면 스키마가 비활성 상태로 추가됩니다. 다운로드로 콘텐츠를 추가해 가는 많은 모바일 타이틀에서는 배포하는 콘텐츠는 계속 AssetBundle로 다루고, 앱에 포함하는 콘텐츠부터 이행을 검토하게 될 것입니다. 참조 순환에 포함된 에셋의 합계가 64MB를 넘으면 빌드가 실패한다는 제약도 있습니다.

어느 구조를 사용하더라도 에셋을 언제 로드하고 언제 해제할 것인가 하는 설계는 그대로 적용됩니다. 제3부 1장 「AssetBundle의 라이프타임 관리」는 로드와 해제를 기반 계층에 집약하고, 로드한 시점에 에셋의 수명을 정함으로써 해제 누락이나 불필요한 에셋의 상주를 구조로 방지합니다.

=== 셰이더 상수를 Build Profile별로 지정하기

Unity 6.6에서는 셰이더 상수의 값을 프로젝트 전체 또는 Build Profile별로 지정할 수 있게 되었습니다@<fn>{fn14}. 플랫폼마다 값만 다른 처리를 키워드로 베리언트를 늘리지 않고 구분하여 작성할 수 있습니다.

=== Build History와 Build Analysis

Unity 6.6에서는 Build History와 Build Analysis 윈도우가 추가되어 플레이어의 빌드 리포트가 자동으로 축적됩니다. 빌드별 용량 추이를 추적할 수 있으므로 비대화의 원인이 된 변경을 특정하기 쉬워집니다. 다만 AssetBundle 빌드에 대한 지원은 제한적입니다.

==[nonum] Unity 6.7(발표 단계)

다음 기능은 2026년 7월 Unite Seoul에서 발표된 것으로, 집필 시점에는 문서에서 자세한 내용을 확인할 수 없는 발표 단계의 정보입니다@<fn>{fn15}.

=== Neural Texture Compression

Neural Texture Compression(NTC)은 학습된 모델을 사용하여 텍스처를 거의 열화 없이 압축하는 기술입니다. 발표에서는 외형 열화 없이 텍스처의 실행 시 메모리를 50% 이상, 디스크 용량을 약 70% 절감한 예가 제시되었습니다. NTC는 Unity 6.7부터 순차적으로 제공이 시작되는 Unity Neural의 일부로 소개되고 있습니다.

=== Fast Build Profile

Fast Build Profile은 Unity 6.7에서 추가될 예정인 새로운 빌드 프로파일로, 셰이더 빌드 시간을 최대 90% 단축할 수 있다고 합니다. 아울러 PSO의 트레이스와 웜업으로 실행 시 스터터를 줄이는 구조도 소개되었습니다.

==[nonum] 제3부를 앞두고

Unity 6.0부터 6.7까지를 따라가 보면, 엔진 쪽의 변화는 지금까지 자체 스크립트나 운영으로 보완해 온 것을 설정으로 가질 수 있게 하는 방향으로 나아가고 있습니다. 한편 이러한 변화는 기본값 그대로 기다린다고 효과가 나오는 것이 아닙니다. mimalloc처럼 업데이트만으로 동작이 바뀌는 것은 업데이트 전후의 계측으로 확인해야 합니다. Content Directories처럼 배포 요건에 따라서는 아직 이행할 수 없는 것도 있습니다. 에셋을 언제 로드하고 해제할지, 어디까지 코드를 제거할지, 기기별로 무엇을 전환할지와 같은 설계 판단은 엔진 버전이 바뀌어도 남습니다.

제3부의 각 장은 그 판단에 필요한 설계와 계측을 실제 프로젝트에서의 사례와 함께 다룹니다. 새로운 버전으로의 업데이트를 검토할 때의 판단 재료로 활용해 주시면 기쁘겠습니다.

//footnote[fn1][Unity Discussions, Unity 6.7 Beta is now available @<href>{https://discussions.unity.com/t/unity-6-7-beta-is-now-available/1736830}]
//footnote[fn2][Unity, Unity 7 roadmap revealed at Unite Seoul @<href>{https://unity.com/news/unity-7-roadmap-revealed-at-unite-seoul}]
//footnote[fn3][Unity Manual 6000.1, What's new in Unity 6.1 @<href>{https://docs.unity3d.com/6000.1/Documentation/Manual/WhatsNewUnity61.html}]
//footnote[fn4][Unity Manual 6000.0, Configure managed code stripping @<href>{https://docs.unity3d.com/6000.0/Documentation/Manual/managed-code-stripping-configure.html}]
//footnote[fn5][Unity Manual 6000.6, Introduction to Mesh LOD @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/lod/mesh-lod-introduction.html}]
//footnote[fn6][Unity Manual 6000.3, What's new in Unity 6.3 @<href>{https://docs.unity3d.com/6000.3/Documentation/Manual/WhatsNewUnity63.html}]
//footnote[fn7][Unity Blog, Improvements to shader build times and memory usage in 2021 LTS @<href>{https://unity.com/blog/engine-platform/2021-lts-improvements-to-shader-build-times-and-memory-usage}]
//footnote[fn8][Unity Manual 6000.4, What's new in Unity 6.4 @<href>{https://docs.unity3d.com/6000.4/Documentation/Manual/WhatsNewUnity64.html}]
//footnote[fn9][Unity Manual 6000.6, mimalloc @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/performance-mimalloc.html}]
//footnote[fn10][Unity Manual 6000.5, What's new in Unity 6.5 @<href>{https://docs.unity3d.com/6000.5/Documentation/Manual/WhatsNewUnity65.html}]
//footnote[fn11][Unity Manual 6000.6, Introduction to content directories @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/content-directories-introduction.html}]
//footnote[fn12][Addressables 4.0 Manual, Content build systems @<href>{https://docs.unity3d.com/Packages/com.unity.addressables@4.0/manual/content-build-systems.html}]
//footnote[fn13][Unity Blog, Content directories: Beyond the AssetBundle @<href>{https://unity.com/blog/content-directories-beyond-the-assetbundle}]
//footnote[fn14][Unity Manual 6000.6, What's new in Unity 6.6 @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/WhatsNewUnity66.html}]
//footnote[fn15][Unity Blog, Unite Seoul 2026 keynote recap @<href>{https://unity.com/blog/unite-seoul-keynote-2026-recap}]
