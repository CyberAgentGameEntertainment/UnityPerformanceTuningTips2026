# **UI 시스템의 구분 사용 — IMGUI / uGUI / UI Toolkit**

이름 / 로마자：Zaitsu Hiromu  
소속：주식회사 Sumzap  
X 계정 / 웹사이트 / GitHub 계정(선택): @RomGameclient

전작 『Unity 퍼포먼스 튜닝 바이블』{{fn:tuning_bible}}에서는 Canvas 분할이나 Raycast Target 조정 등 uGUI의 부하에 대처하는 방법을 다뤘습니다. 이 장에서는 한 걸음 거슬러 올라가 UI 시스템의 선택 자체를 다룹니다. 어느 것을 선택하느냐가 UI의 부하 구조와 취할 수 있는 대처의 종류를 결정하기 때문입니다.  
Unity에는 IMGUI, uGUI, UI Toolkit의 3가지 UI 시스템이 있습니다. 이 3가지를 차례로 살펴보고, 각각이 런타임과 에디터 확장에서 현재 어떤 위치에 있는지 정리합니다. 이 장의 내용은 Unity 6.5 시점의 정보를 기반으로 합니다.

# **IMGUI**

런타임의 IMGUI는 게임 내 UI의 선택지에서 제외되었습니다. 공식 매뉴얼은 플레이어가 조작하는 일반적인 게임 내 UI에 사용하는 것을 의도하지 않는다고 명시하며, 상정 용도로 게임 내 디버그 표시와 툴, 커스텀 인스펙터, 에디터 확장을 듭니다. 디버그 오버레이나 런타임 로그 뷰어 용도로는 현재도 사용됩니다.  
한편 에디터 확장에서 IMGUI는 공식적으로 현역입니다. 공식 UI 시스템 비교 페이지{{fn:ui_system_compare}}는 IMGUI를 대안으로 들며, 장점으로 에디터의 확장 기능에 제한 없이 접근할 수 있다는 점과 화면에 UI를 빠르게 렌더링할 수 있는 경량 API라는 점의 2가지를 제시합니다.  
공식 IMGUI 마이그레이션 가이드{{fn:imgui_migration}}에는 UI Toolkit에 대응하는 기능이 없는 API가 명시되어 있습니다. 실무에서 영향이 큰 것은 InspectorTitlebar(), Toolbar(), IntPopup(), LinkButton() 정도입니다. 모두 에디터 윈도우의 외관을 Unity 본체에 맞추기 위한 API로, 필요한 경우에는 현재 IMGUI를 사용하게 됩니다.  
다만 에디터 확장으로 사용할 때 알아 두어야 할 부하 특성이 있습니다. IMGUI는 OnGUI() 안에 UI를 매번 기술하는 즉시 모드 시스템으로, UI 트리 정보를 유지하지 않고 이벤트마다 생성과 파괴를 반복합니다. 보존 모드인 UI Toolkit과의 차이는 {{fig:render_cycle}}와 같습니다. OnGUI()는 1프레임에 여러 번 실행되며, 그때마다 힙 할당이 발생하기 쉽습니다. 상시 다시 그려지는 에디터 윈도우에서는 이것이 GC 부하가 됩니다.  
![][image1]  
図: 즉시 모드와 보존 모드의 렌더링 사이클 {{fig:render_cycle}}

# **uGUI**

Canvas와 GameObject를 기반으로 한 시스템입니다. 요소 하나하나가 GameObject이며 Transform 계층을 가집니다. 런타임 전용이며 에디터 확장에는 사용할 수 없습니다.  
런타임에서는 사실상 표준입니다. 장점은 3가지입니다.  
・Scene 뷰에서 직접 배치하고 그 자리에서 미리 볼 수 있다  
・Animator와 Timeline으로 UI를 움직일 수 있다  
・Asset Store에 uGUI를 전제로 한 패키지가 대량으로 있다  
부하 측면에서는 요소가 변경된 Canvas에서 메시 재구축이 실행되고, 요소 수에 비례하여 Transform 계층 갱신과 Raycast Target 탐색의 비용이 더해집니다. 전작에서 다룬 Canvas 분할과 Raycast Target 조정은 모두 이 구조에서 비롯되는 부하에 대한 대표적인 대처입니다.

# **UI Toolkit**

VisualElement 트리를 한 번 구축한 뒤 이후에는 변화한 부분만 갱신하는 보존 모드 시스템입니다. 로직은 C\#, UI 구조는 UXML, 스타일은 USS로 나누어 기술합니다. 런타임과 에디터 확장 모두에서 사용할 수 있으며, 에디터 확장에서는 공식 제1 권장입니다. 다만 공식은 「복잡한 에디터 툴을 만든다면」이라는 표현을 사용합니다. 필드 몇 개를 나열하는 정도의 툴이라면 UXML과 USS를 도입하지 않고 C\#만으로 구성해도 문제없습니다.  
부하 측면에서는 uGUI와의 차이가 곧 대처의 차이가 됩니다. 요소를 하나만 바꿔도 Canvas 전체의 메시가 다시 만들어지는 uGUI와 달리, UI Toolkit은 변화한 부분만 갱신하므로 Canvas 분할에 해당하는 대처는 필요하지 않습니다. 다만 요소 수가 늘면 레이아웃 계산 비용은 마찬가지로 늘어나므로, 요소를 줄이는 노력은 계속 필요합니다. Unity 6 계열에서는 이벤트 디스패치 고속화, 지오메트리 생성의 잡화, 레이아웃 계산 캐시 개선 등 성능 개선도 이어지고 있습니다.

## **uGUI와 비교해 부족한 점**

Unity 6.5 시점에서 할 수 없는 것으로는 다음과 같은 항목을 들 수 있습니다. 모두 Unity가 개발 중이라고 밝힌 항목이지만{{fn:uitk_status}}, 지금 도입을 판단한다면 할 수 없다는 전제로 생각해야 합니다.  
・VisualElement의 키프레임 애니메이션을 Animation 윈도우에서 다루는 기능  
・Scene 뷰에서의 UI 편집  
・UXML로 정의한 VisualElement를 MonoBehaviour 스크립트에서 참조하는 구조  
・커스텀 셰이더용 버텍스 데이터 지원  
에코시스템도 얇은 상태 그대로입니다. Asset Store의 에셋은 GameObject 계층을 전제로 한 것이 많아 그대로는 동작하지 않습니다. TextMeshPro도 UI Toolkit에서는 사용할 수 없으며, UI Toolkit은 전용 텍스트 시스템(6.5에서 기본값이 된 Advanced Text Generator)을 사용합니다. DOTween도 VisualElement용 단축 메서드를 추가했지만, 대상은 Unity 6.3 이후입니다{{fn:dotween}}.  
uGUI에서 마이그레이션하는 경우 앵커와 피벗에 직접 대응하는 기능은 없습니다. UI Toolkit의 레이아웃은 부모 요소 안에 자식 요소를 순서대로 흘려 넣어 배치하는 방식입니다. 요소마다 위치와 크기를 지정하던 uGUI와는 구성 방식 자체가 달라집니다.

# **공식 권장의 변천**

공식 UI 시스템 비교 페이지는 매뉴얼 버전별로 공개되어 있으며, 비교해서 읽어 보면 런타임 UI의 권장 이유가 바뀌었습니다{{fn:ui_system_compare_60}}. 그 내역을 {{table:recommendation_shift}}에 나타냅니다.  
表: 런타임 UI 권장 이유의 변화 {{table:recommendation_shift}}

| 요건・용도 | Unity 2022.3〜6.1 | Unity 6.5 |
| :---- | :---- | :---- |
| 대량의 UI를 다룬다 | UI Toolkit | UI Toolkit |
| 아티스트와 디자이너를 위한 제작 워크플로가 필요하다 | UI Toolkit | UI Toolkit |
| 텍스처리스 UI 렌더링 | UI Toolkit | UI Toolkit |
| 3D 공간에 배치・라이팅되는 UI | uGUI | UI Toolkit |
| 커스텀 셰이더와 머티리얼을 통한 표현 | uGUI | UI Toolkit |
| MonoBehaviour에서 참조하기 쉽다 | uGUI | uGUI |

2022.3부터 6.1까지 일관되게 uGUI 쪽에 있던 2가지{{fn:ui_system_compare_2022}}는 월드 스페이스 UI가 6.2에서, 커스텀 셰이더가 6.3에서 URP용으로 구현되면서 UI Toolkit 쪽으로 옮겨 갔습니다. 남은 「MonoBehaviour에서 참조하기 쉽다」도 UXML로 정의한 요소를 참조하는 구조가 개발 중인 항목에 들어 있습니다. 다만 비교 페이지의 구성 자체는 Unity 6.5에서도 바뀌지 않았습니다. 런타임 UI는 uGUI를 기본값으로 소개하고 UI Toolkit을 그 대안으로 나열하는 형태 그대로입니다. 바뀐 것은 권장 이유뿐이며, 어느 쪽을 기본값에 두는지는 달라지지 않았습니다. 같은 페이지에 실린 용도별 목록도 제목이 2022.3부터 6.2까지는 「Recommendation(권장)」, 6.3 이후는 「Often used for(자주 사용됨)」로 바뀌었습니다. Unity 스스로도 이것을 권장이라고 부르지 않게 되었습니다.

# **UI 시스템을 병용할 때의 제약**

하나의 프로젝트에서 여러 UI 시스템을 사용하는 것 자체는 가능하지만, 조합하면 몇 가지 문제가 생깁니다. 런타임과 에디터 확장으로 나누어 살펴봅니다.

## **런타임**

uGUI와 UI Toolkit은 같은 프로젝트에서 사용할 수 있지만, 한쪽을 다른 쪽 안에 포함시킬 수는 없습니다. 내비게이션과 스타일의 불일치도 공식이 제약으로 들고 있습니다.  
렌더링 순서는 각 시스템 내부에서만 제어할 수 있습니다({{fig:sort_order}}). UI Toolkit으로 UI를 화면에 표시하려면 GameObject에 Panel Renderer 컴포넌트를 붙입니다. 이 역할은 Unity 6.4 이전에서는 UIDocument가 담당했으며, 6.5에서 대체되었습니다{{fn:ui_document_component}}. LTS인 6.3을 사용하고 있다면 UIDocument가 됩니다. 이 Panel Renderer가 정렬의 단위이며, 패널끼리는 Panel Settings의 Sort Order로, 같은 패널 안의 Panel Renderer끼리는 PanelRenderer.sortingOrder로 순서가 결정됩니다. uGUI는 루트 Canvas의 Sort Order로 정렬합니다. 이 둘을 가로지르는 순서를 지정하는 프로퍼티는 없습니다.  
uGUI에서의 마이그레이션 가이드{{fn:ugui_migration}}는 렌더 텍스처로 렌더링을 합성하는 방법도 들고 있지만, 입력 이벤트는 전달되지 않는다고 명시합니다. 조작하는 UI에는 사용할 수 없으므로 uGUI와 UI Toolkit을 겹치는 설계는 피하시기 바랍니다.  
![][image2]  
図: UI Toolkit과 uGUI의 렌더링 순서 결정 방식 {{fig:sort_order}}

## **에디터 확장**

에디터 확장에서는 두 시스템을 중첩할 수 있는 방향이 한쪽으로만 제한됩니다({{fig:propertydrawer_nesting}}). UI Toolkit 트리 안에 IMGUI 렌더링을 두는 것은 IMGUIContainer로 가능하지만, 그 반대인 IMGUI 렌더링 안에 UI Toolkit 트리를 두는 것은 불가능합니다.  
이것이 문제가 되는 때는 인스펙터 표시를 직접 만들 때입니다. 인스펙터 커스터마이즈는 컴포넌트 전체를 담당하는 커스텀 인스펙터와 필드 단위로 외관을 교체하는 PropertyDrawer의 2단계로 되어 있으며, 안쪽의 PropertyDrawer는 자신이 어느 시스템의 인스펙터에서 호출되는지 선택할 수 없습니다. UI Toolkit만으로 구현한 PropertyDrawer가 IMGUI로 작성된 커스텀 인스펙터나 IMGUI PropertyDrawer 안에서 호출되면, 해당 필드는 값 입력란이 「No GUI Implemented」(GUI가 구현되지 않음)라는 표시로 대체됩니다. Unity 2022.2 이후에는 인스펙터의 기본값이 UI Toolkit이므로 대부분의 경우 발생하지 않지만, IMGUI 커스텀 인스펙터나 PropertyDrawer가 남아 있는 프로젝트에서는 PropertyDrawer 쪽에 IMGUI 버전의 렌더링도 준비해 두어야 합니다.  
![][image3]  
図: PropertyDrawer 중첩 가능 여부 {{fig:propertydrawer_nesting}}

# **마치며**

조사해 보고 인상적이었던 점은, 공식 권장 이유가 Unity 6.5에서 대부분 UI Toolkit 쪽으로 옮겨져 있었다는 것입니다. 런타임은 uGUI가 여전히 표준이며 동작 중인 것을 이전할 이유는 없지만, 신규 개발에서 UI Toolkit을 피할 이유는 확실히 줄었습니다. 고민될 때는 Animator와 Timeline 연출, Asset Store 에셋에 얼마나 의존하는지로 판단하는 것이 좋을 것 같습니다. 에디터 확장은 복잡한 도구는 UI Toolkit, Unity 본체의 외관에 맞추는 경우는 IMGUI로 정리하면 당분간 문제가 없다고 느꼈습니다. UI 시스템의 선택은 전작에서 다룬 대처가 필요해지는지를 결정하는 가장 상류의 튜닝입니다.

脚注: {{fn:tuning_bible}} https://cyberagentgameentertainment.github.io/UnityPerformanceTuningBible/ja/preface.html  
脚注: {{fn:ui_system_compare}} https://docs.unity3d.com/6000.5/Documentation/Manual/UI-system-compare.html (2026년 8월 6일 참조)  
脚注: {{fn:imgui_migration}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-IMGUI-migration.html  
脚注: {{fn:uitk_status}} https://discussions.unity.com/t/ui-toolkit-development-status-and-next-milestones-november-2025/1698009  
脚注: {{fn:dotween}} https://dotween.demigiant.com/download.php  
脚注: {{fn:ui_system_compare_60}} https://docs.unity3d.com/6000.0/Documentation/Manual/UI-system-compare.html (2026년 8월 6일 참조)  
脚注: {{fn:ui_system_compare_2022}} https://docs.unity3d.com/2022.3/Documentation/Manual/UI-system-compare.html (2026년 8월 6일 참조)  
脚注: {{fn:ui_document_component}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-create-ui-document-component.html  
脚注: {{fn:ugui_migration}} https://docs.unity3d.com/6000.5/Documentation/Manual/UIE-Transitioning-From-UGUI.html

[image1]: assets/zaitsu-hiromu-2/media/image1.png

[image2]: assets/zaitsu-hiromu-2/media/image2.png

[image3]: assets/zaitsu-hiromu-2/media/image3.png