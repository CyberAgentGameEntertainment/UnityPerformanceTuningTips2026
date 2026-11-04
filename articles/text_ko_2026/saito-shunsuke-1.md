# **스타일 가이드**

# **Unity Performance Tuning Bible 2026**  **원고 작성 템플릿 안내**

이 문서는 Re:VIEW로 변환하기 위한 원고 수집용 템플릿입니다.

# **첫 페이지에 기재해야 할 내용**

* 제목  
* 이름 / 로마자 / 소속  
* X 계정/웹사이트/GitHub 계정 (선택)

# **적용되어 있는 기본 스타일**

* **본문:** Noto Sans JP, 12pt, 줄 간격 1.8, 문단 뒤 간격 12pt, 글자색 \#1f2937  
* **제목:** 제목 1/2/3 스타일을 사용하며, 글자색은 \#111827이고 제목 앞 간격은 넓게, 뒤 간격은 좁게 설정되어 있습니다.

# **집필자용 작성 규칙 (Re:VIEW 변환 필수 요소)**

**1\. 제목:** Google 문서의 "제목 1 / 제목 2 / 제목 3" 스타일을 사용하십시오.  
**2\. 본문:** 표준 텍스트 스타일로 작성하십시오.  
**3\. 코드 블록, 이미지, 표, 각주:** 자세한 내용은 "**리스트·그림·표·각주 작성법**" 섹션을 참조하고, Re:VIEW 변환용 전용 형식(CODE START/END, ID 참조)을 반드시 사용하십시오.  
**4\. 주의:** 색상, 글꼴, 들여쓰기 등을 변경하지 마십시오.

# **리스트·그림·표·각주 작성법**

Re:VIEW 변환 시 번호는 자동으로 부여되므로, 집필자는 **\`リスト8.1\`**, **\`図8.5\`**, **\`表8.1\`**와 같은 고정 번호를 본문에 직접 쓰지 마십시오.  
본문에서 참조하려면 고정 번호 대신 전용 ID를 사용합니다. ID는 영숫자와 밑줄만 사용하며, 내용을 알 수 있는 이름으로 지정하십시오.  
예:

* \`settext\_usage\`  
* \`canvas\_disable\`  
* \`ui\_toggle\_cost\`  
* \`zstring\`

# **코드 리스트**

코드는 다음 형식으로 작성하십시오.  
CODE START csharp | SetText 사용 예 {{list:settext_usage}}  
label.SetText("{0}", number);  
CODE END  
캡션이 필요 없는 경우:  
CODE START csharp  
label.text \= number.ToString();  
CODE END  
규칙:

* \`CODE START 言語名 | キャプション {{list:id}}\` 형식으로 작성합니다.  
* C\#의 경우 \`csharp\`을 사용합니다.  
* 본문에서 참조할 가능성이 있는 코드에는 \`{{list:id}}\`을 붙입니다.  
* 코드 안에서는 스마트 따옴표 \`“ ”\`를 사용하지 말고 일반 \`"\`을 사용하십시오.  
* 코드에는 색상이나 굵게 등의 장식을 적용하지 마십시오.

본문에서 참조하는 경우:  
\`{{list:settext_usage}} のように SetText を利用すると、第2引数の値を表示できます。\`  
변환 후에는 Re:VIEW 쪽에서 리스트8.1과 같은 번호가 붙은 참조로 치환합니다.

# **그림**

이미지는 Google Docs 본문에 삽입하고, 이미지 바로 아래에 캡션을 작성하십시오.  
예:  
![][image1]  
図: 튜닝 바이블 로고 {{fig:bible_logo}}  
규칙:

* 캡션은 반드시 이미지 바로 아래에 작성합니다.  
* 본문에서 참조할 가능성이 있는 그림에는 {{fig:id}}을 붙입니다.

본문에서 참조하는 경우:  
이것은 튜닝 바이블의 로고입니다({{fig:bible_logo}}).

# **표**

표는 Google Docs의 표 기능으로 작성해도 됩니다. 표 바로 앞에 캡션을 작성하십시오.  
예:  
表: 표시 전환 처리 시간 {{table:ui_toggle_cost}}

| 방법 | 처리 시간(표시) | 처리 시간(비표시) |
| :---- | :---- | :---- |
| SetActive | 323.79ms | 209.93ms |
| Canvas의 enabled | 61.25ms | 61.23ms |
| CanvasGroup의 alpha | 3.64ms | 3.40ms |

규칙:

* 캡션은 표 바로 앞에 작성합니다.  
* 본문에서 참조할 가능성이 있는 표에는 {{table:id}}을 붙입니다.  
* 셀 병합은 가능한 한 피하십시오.  
* 복잡한 장식이나 색상 지정은 하지 마십시오.  
* 단위는 열 이름에 포함하십시오.

본문에서 참조하는 경우:  
{{table:ui_toggle_cost}}의 결과를 보면 CanvasGroup을 사용한 방법이 처리 시간이 가장 짧다는 것을 알 수 있습니다.

# **각주**

각주를 넣고 싶은 위치에는 본문에 {{fn:id}}을 작성하십시오. 각주 내용은 장 끝 또는 원고 끝에 모아서 작성하십시오.  
본문 예:  
이 TextMeshPro의 기능은 ZString{{fn:zstring}}과 조합하면 매우 강력합니다.  
각주 정의 예:  
脚注: {{fn:zstring}} [\[https://github.com/Cysharp/ZString\](https://github.com/Cysharp/ZString)](https://github.com/Cysharp/ZString)  
규칙:

* 각주 ID는 영숫자와 밑줄만 사용하여 작성합니다.  
* URL만 있는 각주여도 문제없습니다.

# **본문과의 연결 방법**

리스트·그림·표를 본문에서 설명할 때는 고정 번호를 직접 쓰지 말고 ID 참조를 사용하십시오.  
피해야 할 예:  
리스트8.1과 같이 사용하면 두 번째 인수의 값을 표시할 수 있습니다. 그림8.5는 Canvas를 비활성화하는 예입니다. 표8.1의 결과에서 알 수 있듯이...  
권장 예:  
{{list:settext_usage}}과 같이 사용하면 두 번째 인수의 값을 표시할 수 있습니다. {{fig:canvas_disable}}는 Canvas를 비활성화하는 예입니다. {{table:ui_toggle_cost}}의 결과에서 알 수 있듯이...  
최종 Re:VIEW 변환 시 이러한 참조를 실제 리스트8.1, 그림8.5, 표8.1과 같은 번호가 붙은 참조로 치환합니다.

# **본문**

# **Unity 3D 캐릭터 애니메이션 튜닝**

* 이름: Shunsuke Saito

* 소속: 주식회사 Colorful Palette

# **들어가며**

이 장에서는 "Unity에서의 3D 캐릭터 애니메이션 기능 최적화 방법"을 에셋 설정과 구현의 여러 관점에서 소개합니다.

## **계측 환경**

이 장의 구현과 계측은 다음 환경에서 수행했습니다.

表: 계측 환경 {{table:measurement_env}}

| 항목 | 값 |
| :---- | :---- |
| Unity | 6000.5.7f1 |
| 플랫폼 | macOS Standalone (StandaloneOSX) |
| Scripting Backend | IL2CPP / IL2CPP Compiler Configuration \= Release / Code Generation \= OptimizeSpeed |
| Graphics API | Metal / GPU Skinning \= 활성화 |
| Quality Level | PC / Skin Weights \= Four Bones |

## **사용 에셋과 그 구조**

계측에 사용하는 캐릭터는 unity-chan을 기반으로 **Generic 리그**로 다시 임포트한 것입니다. 후술하는 Optimize Game Objects를 적용하기 전후의 사양은 다음과 같습니다.

表: 계측에 사용하는 프리팹의 Transform 수와 버텍스 수 {{table:character_prefab_spec}}

| 프리팹 | Transform 수 | 버텍스 수 |
| :---- | :---- | :---- |
| Base | 166 | 13,391 |
| Optimized（Optimize Game Objects） | 24 | 13,391 |

# **Unity Animation의 기본 제어에 대하여**

먼저 Unity에서 애니메이션을 제어하는 세 가지 방법을 정리합니다.

## **1\. Animator 컴포넌트를 사용한 제어**

\`Animator\`에 \`RuntimeAnimatorController\`를 할당하는 가장 일반적인 방법입니다. 스테이트 머신, 트랜지션, 블렌드 트리, 파라미터 등의 구조가 갖추어져 있어 Mecanim 워크플로를 그대로 사용할 수 있습니다.

CODE START csharp | Animator 컴포넌트를 이용한 제어 {{list:animator_control}}  
var animator \= GetComponent\<Animator\>();  
animator.runtimeAnimatorController \= controller;   // 스테이트 머신을 할당한다  
animator.SetFloat("Blend", 0.5f);                  // 파라미터로 제어한다  
CODE END

중요한 점은 Animator가 내부적으로 **Playable Graph를 구성하여 실행한다**는 것입니다. 스테이트 머신의 평가도 클립 샘플링도 내부에서는 그래프의 노드로 처리됩니다. 다음 두 방법은 이 내부 그래프를 직접 구성하는 방법에 해당합니다.

## **2\. Playable API로 Playable Graph를 구성하고 자동 업데이트로 제어**

\`PlayableGraph\`를 직접 구성하는 방법입니다. 스테이트 머신을 거치지 않고 블렌드 구조를 완전히 제어할 수 있으며, 후술하는 Animation Job(\`AnimationScriptPlayable\`)을 그래프의 임의 위치에 삽입할 수 있습니다.

CODE START csharp | Playable API를 이용한 그래프 구성(자동 업데이트) {{list:playable_auto}}  
var graph \= PlayableGraph.Create("MyGraph");  
var output \= AnimationPlayableOutput.Create(graph, "Animation", animator);

var clipA \= AnimationClipPlayable.Create(graph, waitClip);  
var clipB \= AnimationClipPlayable.Create(graph, walkClip);  
var mixer \= AnimationMixerPlayable.Create(graph, 2);  
graph.Connect(clipA, 0, mixer, 0);  
graph.Connect(clipB, 0, mixer, 1);  
mixer.SetInputWeight(0, 0.5f);  
mixer.SetInputWeight(1, 0.5f);

output.SetSourcePlayable(mixer);  
graph.Play();  
CODE END

업데이트 모드의 기본값은 \`DirectorUpdateMode.GameTime\`입니다. 이 모드에서는 Unity의 라이프사이클(Update와 LateUpdate 사이에 있는 Animation 단계)에서 Animator 컴포넌트와 같은 타이밍에 **자동으로 평가**됩니다. 호출 측에서 매 프레임 별도로 처리할 필요는 없습니다.

## **3\. Playable API로 Playable Graph를 구성하고 수동 업데이트로 제어**

같은 그래프에 \`DirectorUpdateMode.Manual\`를 설정하면 Unity는 자동 평가를 전혀 하지 않게 되고, \`Evaluate()\`를 호출했을 때만 그래프가 평가됩니다.

CODE START csharp | 수동 업데이트에서의 그래프 평가 {{list:playable_manual}}  
graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);  
graph.Play();

void Update()  
{  
    graph.Evaluate(Time.deltaTime);   // 호출했을 때만 평가된다  
}  
CODE END

평가의 타이밍과 빈도를 완전히 제어할 수 있으므로 "특정 캐릭터(그래프)만 업데이트를 멈춘다", "몇 프레임에 한 번만 업데이트한다"와 같은 제어를 그대로 구현할 수 있습니다.

세 방법의 관계를 정리하면 다음과 같습니다.

表: 애니메이션 제어 3가지 방법 비교 {{table:animation_control_methods}}

| 방법 | 그래프 구성 | 평가 타이밍 |
| :---- | :---- | :---- |
| Animator 컴포넌트 | Unity(내부) | Unity의 Animation 단계 |
| Playable API \+ 자동 업데이트 | 직접 | Unity의 Animation 단계 |
| Playable API \+ 수동 업데이트 | 직접 | \`Evaluate()\`를 호출했을 때 |

# **에셋 설정**

코드 측 최적화를 소개하기 전에, 에셋 측 설정으로 적용할 수 있는 최적화부터 살펴보겠습니다. 이 절의 주제는 **본의 Transform 계층을 없애는 것**입니다. 같은 결과를 얻는 방법은 두 가지입니다.

## **Optimize Game Objects(임포트 시)**

FBX 임포트 설정(Rig 탭)의 \`Optimize Game Objects\`를 활성화하면 실행 시 본의 GameObject / Transform이 통째로 생성되지 않으며, Animator가 내부 스켈레톤만 가지게 됩니다. 이번 캐릭터에서는 **Transform이 166개에서 24개**(루트 \+ Renderer 23개)로 줄었습니다.

※본이 GameObject로 존재하지 않으므로 계층을 따라가 본을 다루는 코드는 동작하지 않게 됩니다. 무기의 어태치 포인트처럼 특정 본만 필요한 경우에는 \`Extra Transforms to Expose\`로 개별적으로 남깁니다.

## **Optimize Hierarchy(런타임)**

같은 작업을 런타임에 수행하는 API가 \`AnimatorUtility.OptimizeTransformHierarchy\`입니다.

CODE START csharp | 런타임에서의 계층 평탄화 {{list:optimize_hierarchy_api}}  
// go는 Animator(와 유효한 Avatar)를 가진 루트이다. 두 번째 인자는 노출할 본 이름의 배열이다  
AnimatorUtility.OptimizeTransformHierarchy(go, null);

// 역변환도 있다(에디터 확장이나 디버그에서 계층을 일시적으로 되돌리고 싶을 때)  
AnimatorUtility.DeoptimizeTransformHierarchy(go);  
CODE END

임포트 설정을 변경할 수 없는 에셋이라도 생성 시점에 실행하면 같은 구조가 됩니다. 실제로 확인한 결과, Base 프리팹(Transform 166개)에 적용하면 **Transform 24개·\`hasTransformHierarchy \== false\`**가 되었으며, 임포트 시 Optimize Game Objects를 적용한 프리팹과 완전히 같은 구조가 되었습니다.

![][image2]

図: 최적화 전의 Hierarchy. \`Character1\_Hips\` 아래에 본의 Transform 계층이 166개 이어진다. {{fig:hierarchy_base}}

![][image3]

図: \`AnimatorUtility.OptimizeTransformHierarchy\` 적용 후. 본 계층이 사라지고 루트 바로 아래에 SkinnedMeshRenderer 23개만 나열된다(Transform 24개). {{fig:hierarchy_optimized}}

## **실측: 계층 평탄화의 효과**

렌더링 부하를 두 단계로 바꾸어 계측했습니다. 첫 번째는 Renderer를 모두 활성화한 구성(300체 × 23 Renderer \= 6,900 Renderer), 두 번째는 각 캐릭터에서 버텍스 수가 가장 많은 SkinnedMeshRenderer 1개만 활성화하여 렌더링 부하를 낮춘 구성입니다. 프레임 시간은 중앙값입니다.

表: 계층 평탄화의 효과(300체·전체 Renderer와 600체·Renderer 1개) {{table:optimize_hierarchy_result}}

| 구성 | 베리언트 | 프레임 시간 | \`MeshSkinning.Update\` (main) | \`MeshSkinning.CalcMatrices\` (all) | \`Semaphore.WaitForSignal\` |
| :---- | :---- | :---- | :---- | :---- | :---- |
| 300체·전체 Renderer | Base | 18.59 ms | 6.23 ms | 36.633 ms | 5.33 ms |
| 300체·전체 Renderer | Optimize Game Objects | 18.497 ms (±0) | **1.537 ms** (−75.7 %) | **0 ms** | **10.255 ms** |
| 300체·전체 Renderer | Optimize Hierarchy | 18.307 ms (±0) | **1.504 ms** (−76.2 %) | **0 ms** | **9.977 ms** |
| 600체·1 Renderer | Base | 6.797 ms | 0.735 ms | 4.385 ms | 5.741 ms |
| 600체·1 Renderer | Optimize Game Objects | **5.991 ms (−11.9 %)** | 0.207 ms | **0 ms** | 4.901 ms |
| 600체·1 Renderer | Optimize Hierarchy | **6.009 ms (−11.6 %)** | 0.208 ms | **0 ms** | 5.010 ms |

CPU 측 처리는 어느 구성에서나 같은 비율로 줄었습니다. 메인 스레드의 스키닝 처리가 6.321 → 약 1.5 ms(전체 Renderer), 0.735 → 약 0.21 ms(Renderer 1개)로, 모두 70% 넘게 절감되었습니다. 스키닝 행렬 계산(\`MeshSkinning.CalcMatrices\`)은 전체 스레드 합계 36.6 ms / 4.4 ms가 **완전히 사라졌습니다**.

다만 "전체 Renderer 구성"에서는 렌더 스레드의 처리가 프레임 시간의 크리티컬 패스이므로 프레임 시간의 감소는 나타나지 않습니다.

렌더링 부하를 낮춘 Renderer 1개 구성에서는 같은 절감이 그대로 **12 % 전후의 프레임 시간 단축**으로 나타납니다. 300체에서도 같은 경향으로, 3.312 → 2.899 / 2.901 ms(−12.5 % / −12.4 %)였습니다. **임포트 시 Optimize Game Objects와 런타임 Optimize Hierarchy의 차이는 0.1 % 미만**으로, 구조가 같아지는 이상 효과도 같다는 것을 알 수 있습니다.

Profiler의 Timeline에서도 같은 현상이 나타납니다. 둘 다 300체·전체 Renderer이며, 프레임 시간의 중앙값 부근 프레임입니다.

![][image4]

図: Base(Transform 166개). 프레임 시간 18.59 ms 중 메인 스레드에서 \`MeshSkinning.Update\`가 6.23 ms를 차지하고, \`Gfx.WaitForPresentOnGfxThread\` / \`Semaphore.WaitForSignal\`가 5.33 ms. {{fig:profiler_ch2_base}}

![][image5]

図: \`AnimatorUtility.OptimizeTransformHierarchy\` 적용 후. \`MeshSkinning.Update\` 블록은 라벨을 읽을 수 없을 정도로 가늘어지는 반면, \`Semaphore.WaitForSignal\`가 5.33 → 10.44 ms로 늘어났고 \`PlayerLoop\`는 18.58 → 18.91 ms로 거의 변하지 않았다. 줄어든 CPU 시간이 대기 시간으로 바뀌었다. {{fig:profiler_ch2_optimized}}

## **실측: Renderer 전체 비활성화(애니메이션 평가만)**

렌더링과 스키닝을 완전히 제외하고 애니메이션 평가 자체에 대한 기여를 확인합니다. 300체입니다.

表: Renderer를 전체 비활성화했을 때의 프레임 시간 {{table:optimize_hierarchy_norender}}

| 베리언트 | 프레임 시간 |
| :---- | :---- |
| Base | 2.702 ms |
| Optimize Game Objects | 2.687 ms (−0.6 %) |
| Optimize Hierarchy | 2.496 ms (−7.6 %) |

클립 샘플링이나 블렌딩 같은 평가 처리는 Transform 계층의 유무와 관계없이 실행되므로, 이쪽에 대한 기여는 거의 없다고 할 수 있습니다. 이로써 이 최적화의 목적이 주로 **스키닝 행렬 계산과 Transform으로의 쓰기 반영**에 대한 것이며 애니메이션 평가 자체가 아니라는 점을 알 수 있습니다.

## **이 장의 결론**

* 계층 평탄화는 **스키닝 행렬 계산(\`MeshSkinning.CalcMatrices\`)** 절감으로 이어진다

* 임포트 시(Optimize Game Objects)와 런타임(\`AnimatorUtility.OptimizeTransformHierarchy\`)의 효과는 동등하다

* 어느 쪽을 택할지는 운용의 문제: 자체 에셋이라면 임포트 설정으로 처리하고, 임포트 설정을 변경할 수 없는 사정이 있다면 런타임 API를 사용한다

* **모델 자체의 버텍스 수·본 수를 제작 단계에서 적정화해 두는 것**이 최우선이다

# **Job Thread 활용**

## **Unity의 애니메이션 병렬화**

Animator 컴포넌트와 자동 업데이트 Playable Graph는 Unity가 **모든 그래프를 한꺼번에 스케줄하여 Animation Job Thread에 분산**합니다.

문제는 수동 업데이트입니다. \`PlayableGraph.Evaluate()\`는 **해당 그래프 내부의 평가 처리를 병렬화**하는 것이며 그래프 간에는 병렬화하지 않습니다. 따라서 "1캐릭터 \= 1 Playable Graph"라는 직관적인 구성으로 N체를 제어하면 \`Evaluate()\`를 N번 호출하게 되어 **N개의 그래프가 직렬로 처리됩니다**.

이는 Unity Playable Graph의 피할 수 없는 사양이지만, 대책은 단순하며 **N체를 하나의 그래프로 모으는 것**입니다.

CODE START csharp | 모든 캐릭터를 1개의 그래프로 집약 {{list:shared_graph}}  
// 하나의 그래프에 캐릭터별 출력과 서브트리를 연결한다  
\_sharedGraph \= PlayableGraph.Create("Shared");  
\_sharedGraph.SetTimeUpdateMode(DirectorUpdateMode.Manual);

foreach (var character in characters)  
{  
    var output \= AnimationPlayableOutput.Create(\_sharedGraph, "Animation", character.Animator);  
    output.SetSourcePlayable(BuildClipSource(\_sharedGraph));   // 각 캐릭터의 믹서  
}

\_sharedGraph.Play();

// 업데이트는 한 번만 수행한다  
void Update() \=\> \_sharedGraph.Evaluate(Time.deltaTime);  
CODE END

## **실측 1: 제어 방법 × 캐릭터 수**

Renderer는 전체 비활성화하여 애니메이션 평가 비용만 추출했습니다. 프레임 시간의 중앙값입니다.

表: 제어 방법과 캐릭터 수별 프레임 시간 {{table:driver_frame_time}}

| 제어 방법 | 100체 | 300체 | 600체 |
| :---- | :---- | :---- | :---- |
| Animator 컴포넌트 | 0.899 ms | 2.413 ms | **5.004 ms** |
| Playable API \+ 자동 업데이트 | 0.899 ms | 2.498 ms | **5.093 ms** |
| Playable API \+ 수동 업데이트(1체 1그래프) | 1.395 ms | 3.925 ms | **8.396 ms** |
| Playable API \+ 수동 업데이트(전체 1그래프) | 0.800 ms | 2.115 ms | **4.500 ms** |

**Animator와 Playable 자동 업데이트는 거의 같은 비용입니다.** "Playable API가 더 가볍다"라는 주장은 적어도 "단순한 애니메이션 제어" 조건에서는 성립하지 않습니다. Animator의 스테이트 머신 평가(\`AnimatorControllerPlayable.PrepareFrame\`)는 600체에서도 전체 스레드 합계 2.624 ms이고 그중 메인 스레드는 0.161 ms에 불과해, 이 처리가 병목이 될 가능성은 매우 낮다고 할 수 있습니다. (다만 2 ms를 크다고 볼지 작다고 볼지는 게임에 따라 다르다고 생각합니다.)

**수동 업데이트는 그래프 구조에 따라 1.87배 차이가 납니다.** 같은 600체라도 그래프를 묶는 방식에 따라 자동 업데이트보다 빠르기도 하고 느리기도 합니다.

## **실측 2: 스레드 사용 방식**

600체, Renderer 비활성화. Unity Profiler 캡처에서 스레드별로 집계한 결과입니다.

表: 그래프 단위에 따른 스레드 사용 방식의 차이(600체) {{table:graph_granularity_threads}}

|  | 1체 1그래프(600그래프) | 600체 1그래프 |
| :---- | :---- | :---- |
| 프레임 시간 | **9.490 ms** | **4.928 ms** |
| 메인 스레드의 \`Evaluate\` | 9.028 ms | 4.556 ms |
| **애니메이션 처리를 담당한 워커 스레드 수** | **5** | **15** |
| \`Animator.ProcessGraph\` 전체 스레드 합계 | 0.920 ms | **19.034 ms** |
| 워커 1개당 \`Animator.ProcessGraph\` | 0.32 / 0.15 / 0.03 / 0.02 / 0.002 ms | 각 **약 1.19 ms**(15개 거의 균등) |

**1체 1그래프일 때** 애니메이션 처리는 거의 전부 메인 스레드 위에 있습니다. 600번의 \`Evaluate()\`가 순서대로 실행될 뿐이며, 워커 스레드는 거의 일하지 않습니다.

**600체 1그래프일 때** 15개 워커 스레드 모두가 거의 균등하게 \`Animator.ProcessGraph\`를 실행합니다.

![][image6]

図: 1체 1그래프(600그래프)의 Timeline. 메인 스레드의 \`ATB.ManualEvaluate\`가 약 8.6 ms를 차지하고, Job 워커 대부분이 "Idle" 상태로 남아 있다. {{fig:profiler_timeline_per_character}}

![][image7]

図: 600체 1그래프의 Timeline. 메인 스레드의 \`ATB.ManualEvaluate\`는 약 4.3 ms로 절반이 되었고, 모든 워커가 약 1.2 ms씩의 \`EngineJob\`를 동시에 실행하고 있다. {{fig:profiler_timeline_single_graph}}

**수치를 읽을 때 한 가지 보충할 점이 있습니다.**

이 표의 수치는 Unity Profiler를 연결한 상태의 캡처에서 가져왔기 때문에, Profiler 없이 실행한 계측(4.500 ms / 8.396 ms)보다 약간 무겁게 나옵니다. 장 끝의 정리 표는 Profiler 비연결 수치를 사용합니다.

## **이 장의 결론**

表: 600체에서의 프레임 시간 정리 {{table:graph_summary}}

| 구성 | 600체의 프레임 시간 |
| :---- | :---- |
| 1체 1그래프 \+ 수동 업데이트 | 8.396 ms |
| Animator(자동) | 5.004 ms |
| 600체 1그래프 \+ 수동 업데이트 | **4.500 ms** |

* Animator와 자동 업데이트 Playable Graph는 Unity가 모든 그래프를 한꺼번에 Animation Job Thread에 분산해 준다

* 수동 업데이트는 그래프 단위로만 병렬화된다

* 수동 업데이트를 선택하는 경우 그래프의 단위를 반드시 설계 항목에 고려한다

* Playable API로 교체하는 것 자체에는 최적화 효과가 거의 없다. Playable API를 사용하는 진정한 가치는 "Animation Job을 삽입할 수 있다", "그래프 구조와 업데이트 타이밍을 제어할 수 있다"와 같은 **애니메이션 제어의 유연성**에 있다

# **Animation C\# Jobs 사용과 SIMD 대응에 대하여**

'프로시저럴하게 애니메이션을 제어하고 싶다'는 요구는 게임 개발 중에 반드시 발생합니다. 예를 들어 '캐릭터의 발을 지면 형상에 맞게 접지하도록 움직이고 싶다'와 같은 경우입니다.

이 경우 취할 수 있는 수단으로 Two-Bone IK라는 기법이 있습니다.

실제로 Two-Bone IK와 같은 프로시저럴 애니메이션 제어를 구현할 때 선택지는 세 가지입니다.

1. \`LateUpdate\`에서 Transform을 직접 덮어쓴다

2. \`AnimationScriptPlayable\`에 \`IAnimationJob\`를 삽입한다

3. 2에 더해 Burst로 컴파일한다

이번에는 이 방법들의 퍼포먼스에 주목합니다.

세 가지 모두 **완전히 동일한 연산**을 수행하도록 구현했습니다. 해법은 하나의 static 메서드로 분리했고 LateUpdate 버전·Job 버전·Burst 버전이 이를 공유하므로, 다른 점은 실행되는 위치뿐입니다.

## **Animation C\# Jobs란**

Animation C\# Jobs는 \`AnimationScriptPlayable\`와 \`IAnimationJob\`를 통해 **애니메이션 평가 파이프라인 안에 자체 처리를 삽입**하는 구조입니다. 잡은 클립 평가나 블렌딩과 같은 흐름 안에서 호출되며, 본에는 \`AnimationStream\`와 \`TransformStreamHandle\`를 통해 접근합니다.

\`LateUpdate\`와의 차이는 코드가 동작하는 위치입니다. \`LateUpdate\`는 메인 스레드 위에서 애니메이션 평가가 끝난 뒤의 Transform을 직접 덮어씁니다. 처리는 캐릭터 수만큼 메인 스레드에 직렬로 쌓입니다. Animation Job은 평가 파이프라인 내부(Animation Job Thread)에서 호출되므로, Unity는 다른 애니메이션 평가와 마찬가지로 워커 스레드에 분산할 수 있습니다. 같은 연산이라도 메인 스레드의 부담이 되는지 여부가 달라집니다.

\`AnimationStream\`를 경유해야 한다는 접근 제약은 그 대가입니다. 잡에서 보이는 것은 사전에 묶어 둔 핸들뿐이며, 임의의 \`Transform\`이나 \`GameObject\`는 다룰 수 없습니다.

## **구현**

잡 본체는 \`ProcessAnimation\`에서 해법을 호출할 뿐입니다.

CODE START csharp | Two-Bone IK의 Animation Job(Burst 대응) {{list:two_bone_ik_job}}  
\[BurstCompile\]  
public struct TwoBoneIkJobBurst : IAnimationJob  
{  
    public TransformStreamHandle Root;  
    public TransformStreamHandle Mid;  
    public TransformStreamHandle Tip;  
    public NativeArray\<float3\> Target;   // 길이 1. 매 프레임 메인 스레드에서 기록한다  
    public float Weight;  
    public int Iterations;

    public void ProcessRootMotion(AnimationStream stream) { }

    \[BurstCompile\]  
    public void ProcessAnimation(AnimationStream stream)  
    {  
        TwoBoneIkCore.SolveStreamIterations(  
            ref stream, Root, Mid, Tip, Target\[0\], Weight, Iterations);  
    }  
}  
CODE END

본의 읽기·쓰기는 \`TransformStreamHandle\`와 \`AnimationStream\`의 조합으로 수행합니다.

CODE START csharp | AnimationStream에서 본을 읽고 쓰기 {{list:ik_solve_stream}}  
static void SolveStream(  
    ref AnimationStream stream,  
    TransformStreamHandle root, TransformStreamHandle mid, TransformStreamHandle tip,  
    float3 target, float weight)  
{  
    var rootPosition \= (float3)root.GetPosition(stream);  
    var midPosition \= (float3)mid.GetPosition(stream);  
    var tipPosition \= (float3)tip.GetPosition(stream);  
    var rootRotation \= (quaternion)root.GetRotation(stream);  
    var midRotation \= (quaternion)mid.GetRotation(stream);

    Solve(rootPosition, midPosition, tipPosition, target, rootRotation, midRotation, weight,  
        out var newRootRotation, out var newMidRotation);

    mid.SetRotation(stream, newMidRotation);  
    root.SetRotation(stream, newRootRotation);  
}  
CODE END

핸들은 \`Animator.BindStreamTransform\`로 만들고, 잡은 클립의 후단으로 그래프에 삽입합니다.

CODE START csharp | Animation Job을 그래프에 삽입하기 {{list:ik_playable_connect}}  
var job \= new TwoBoneIkJobBurst  
{  
    Root \= animator.BindStreamTransform(rootTransform),  
    Mid \= animator.BindStreamTransform(midTransform),  
    Tip \= animator.BindStreamTransform(tipTransform),  
    Target \= ikTarget,   // NativeArray\<float3\>  
    Weight \= 1f,  
    Iterations \= iterations,  
};

var ikPlayable \= AnimationScriptPlayable.Create(graph, job, 1);  
graph.Connect(clipSource, 0, ikPlayable, 0);   // clipSource = 클립 쪽 믹서  
ikPlayable.SetInputWeight(0, 1f);  
output.SetSourcePlayable(ikPlayable);  
CODE END

IK 타깃처럼 매 프레임 바뀌는 입력은 \`NativeArray\`에 두고 메인 스레드에서 씁니다.

Burst는 클래스에 \`BurstCompile\` 속성을 부여하면 적용됩니다.

Burst Compiler에 의한 최적화가 실제로 적용되고 있는지는 Burst Inspector 등의 도구로 확인할 수 있습니다.

## **실측**

300체, Renderer 비활성화. 1체당 IK 해법 호출 횟수를 1 / 16 / 64로 바꾸었습니다.

表: IK 해법 호출 횟수별 프레임 시간(300체·Renderer 비활성화) {{table:ik_frame_time}}

| 구현 | 1회 | 16회 | 64회 |
| :---- | :---- | :---- | :---- |
| IK 없음(기준) | 2.501 ms | 2.601 ms | 2.596 ms |
| LateUpdate | 2.699 ms | 4.400 ms | **9.094 ms** |
| Animation Job | 2.411 ms | 2.496 ms | **2.795 ms** |
| Animation Job \+ Burst | 2.494 ms | 2.496 ms | **2.792 ms** |

CPU 시간 내역(64회일 때):

表: IK 구현별 CPU 시간 내역(해법 64회) {{table:ik_cpu_breakdown}}

| 구현 | IK의 CPU 시간(전체 스레드) | 그중 메인 스레드 | 프레임 시간에 대한 가산분 |
| :---- | :---- | :---- | :---- |
| LateUpdate | 6.910 ms | **6.910 ms（100 %）** | \+6.498 ms |
| Animation Job | 7.790 ms | 0.487 ms（6.3 %） | \+0.199 ms |
| Animation Job \+ Burst | 6.932 ms | 0.436 ms（6.3 %） | \+0.196 ms |

## **Profiler Timeline으로 보는 실행 스레드의 차이**

같은 연산이 어느 스레드에서 실행되는지는 Unity Editor의 Profiler에서 확인할 수 있습니다. 아래는 해법 64회·300체의 캡처를 Profiler 창에서 열고 프레임 시간의 중앙값 부근 프레임을 선택한 것입니다. Profiler 연결 상태의 캡처이므로 위 표의 수치와는 완전히 일치하지 않습니다.

![][image8]

図: LateUpdate 구현. 프레임 시간 8.698 ms 중 \`LateBehaviourUpdate\`가 6.63 ms를 차지한다. 워커 스레드 15개는 프레임의 대부분(약 6.8~7.7 ms)이 \`Idle\` 상태이다. {{fig:profiler_ik_lateupdate}}

![][image9]

図: Animation Job 구현. 같은 연산이 \`AnimationScriptPlayable.ProcessAnimation\`로서 워커 스레드 15개에 분산되어 1개당 약 0.5 ms씩 실행되고 있다. 프레임 시간은 2.796 ms. {{fig:profiler_ik_animationjob}}

LateUpdate 구현에서는 300체 분량의 \`ATB.TwoBoneIk.LateUpdate\`(합계 6.577 ms)가 모두 메인 스레드의 \`LateBehaviourUpdate\` 안에 있습니다. 이때 워커 스레드가 담당하는 것은 기본 애니메이션 평가(\`Animator.ProcessGraph\`가 1개당 0.12~0.29 ms)뿐이며, IK 계산은 하나도 실려 있지 않습니다.

Animation Job 구현에서는 \`AnimationScriptPlayable.ProcessAnimation\`의 전체 스레드 합계가 7.891 ms이고, 그중 메인 스레드는 0.498 ms입니다. 나머지 7.393 ms는 워커 스레드 15개에 거의 균등하게(1개당 0.47~0.51 ms) 분산되어 있습니다. 총 CPU 시간은 LateUpdate 구현보다 많지만 이것이 16개 스레드로 나뉘므로 프레임 시간은 8.698 → 2.796 ms로 줄어듭니다.

## **읽어낼 수 있는 점**

**이 사례의 최적화에서 가장 효과적인 것은 Burst가 아니라 실행 스레드를 옮기는 것입니다.** LateUpdate 구현은 IK의 CPU 시간을 통째로 메인 스레드에서 부담하므로, 64회에서는 프레임 시간이 2.596 → 9.094 ms로 늘어납니다. 같은 연산을 Animation Job으로 옮기면 메인 스레드의 부담이 거의 사라지며, 프레임 시간에 대한 가산분에서 **33배의 차이**가 납니다.

한편 **총 CPU 시간에서는 LateUpdate 버전이 더 적습니다**. Animation Job에는 \`AnimationScriptPlayable\` 노드당 고정 비용이 있으며, 300체 기준 약 2.16 ms입니다. 그래도 병렬화의 이득이 훨씬 큽니다.

**실제 운용에 가까운 "1체 1회" 조건에서는 세 구현의 차이가 거의 계측 노이즈 범위**입니다. 300체 정도의 Two-Bone IK라면 어느 구현을 선택해도 체감 차이는 없습니다.

## **Burst는 속성을 붙이는 것만으로는 효과가 없다**

\`\[BurstCompile\]\`를 붙이면 SIMD가 적용되어 몇 배 빨라진다는 이해는 잘못된 것입니다. 이번 실측에서 Burst에 의한 총 CPU 시간 절감은 11.0 %입니다. 노드 고정 비용을 뺀 "해법 1회당 한계 비용"으로 보면 다음과 같습니다.

* Burst 없음: (7.790 − 2.157) / 63 \= **0.0894 ms**

* Burst 있음: (6.932 − 2.167) / 63 \= **0.0756 ms**(−15.4 %)

Burst가 수행하는 것은 LLVM에 의한 네이티브 코드 생성이며, SIMD 명령이 생성되는지는 작성된 코드의 형태로 결정됩니다. **적절하게 SIMD화되는 코드를 구현하지 않으면 큰 이점을 얻을 수 없습니다.**

Animation C\# Jobs에는 SIMD화를 어렵게 하는 성질이 두 가지 있습니다.

**\`AnimationStream\`의 읽기·쓰기가 처리 안에 빈번하게 나타납니다.** 해법 1회마다 \`GetPosition\`·\`GetRotation\`로 본을 읽고 \`SetRotation\`로 다시 씁니다. 반복할 때마다 스트림의 상태가 바뀌므로 반복 간에 의존 관계가 남아, 여러 회분을 묶어 하나의 벡터 명령으로 만들 수 없습니다. 게다가 \`IAnimationJob\`가 받는 \`AnimationStream\`는 1체 분량이므로, "4체 분량을 동시에 계산"하기 위한 데이터가 애초에 잡 안에 없습니다.

**초월 함수 사용이 빈번합니다.** 코사인 법칙으로 굽힘 각도를 구하는 \`acos\`, \`quaternion.AxisAngle\`가 내부에서 호출하는 삼각함수, \`math.length\`의 제곱근. 이들은 레인을 가로지르는 계산이나 스칼라 라이브러리 호출이 되어 벡터화를 막습니다.

Burst의 LLVM도 해법을 반복 호출하는 루프를 **벡터화할 수 없다고 명시적으로 보고합니다**(최적화 리마크 \`loop not vectorized: instruction return type cannot be vectorized\`).

## **Burst의 이점을 최대한 활용하기 위한 방안**

Burst의 이점을 최대한 활용하려면 \`C\# Job System\`를 사용하여 **\`AnimationStream\`를 거치지 않고 Transform을 읽고 쓰는** 구성이 후보가 됩니다. Transform의 읽기·쓰기는 \`TransformAccessArray\`와 \`IJobParallelForTransform\`로 잡 측에 모을 수 있고, 본 수집과 쓰기 반영만 Animation Job에 맡기고 무거운 계산은 N체 분량을 모은 배치 잡으로 분리하는 형태도 가능합니다. 어느 쪽이든 캐릭터 단위가 아니라 **N체 분량의 데이터가 하나의 잡 안에 갖춰지므로** SIMD화의 전제가 성립합니다.

그 위에서 배치 잡의 내용은 다음과 같이 정리해야 합니다.

* 1성분 1배열의 SoA(Structure of Arrays)로 만들어 캐릭터를 메모리 상에서 연속시킨다

* 분기를 \`math.select\`나 \`math.max\`로 치환하여 루프 본체를 한 줄기의 직선으로 만든다

* 초월 함수를 루프에서 제거한다(항등식으로 다시 쓰기, 테이블화, 근사)

* \`float3\`가 아니라 \`float4\`로 "4체 분량을 1레인씩" 작성하여 자동 벡터화의 판단에 의존하지 않는다

대가는 설계의 복잡함입니다. \`IAnimationJob\`는 그래프에 노드를 하나 삽입하는 것만으로 평가 파이프라인의 올바른 타이밍에 본의 최신 상태를 다룰 수 있다는 간편함이 가장 큰 장점인데, 이 방안은 그것을 포기하게 됩니다. 이 글에서는 이 구성의 실측을 수행하지 않았으므로 얼마나 빨라지는지는 여기서 제시하지 않습니다. Burst에 몇 배의 효과를 기대한다면 이 방향의 설계 변경이 전제가 된다는 제안으로 받아들여 주십시오.

## **이 장의 결론**

* Animation Job의 가치는 **Animation Job Thread를 활용할 수 있다**는 점이다

* 연산량이 적을 때는 \`AnimationScriptPlayable\`의 노드 고정 비용이 더 크다. 단순하게 LateUpdate에서 작성하는 편이 총 CPU 시간이 적다

* \`\[BurstCompile\]\`는 붙이는 것만으로는 SIMD가 되지 않는다. SIMD가 적용될 것을 염두에 둔 데이터·처리 설계가 필요하다

# **애니메이션 LOD 구현**

## **구조와 효과적인 상황**

게임 사양에 따라 다르지만 기본적으로 먼 캐릭터까지 매 프레임 업데이트할 필요는 없습니다. N프레임에 한 번만 업데이트하면 그 캐릭터의 평균 부하는 1/N이 됩니다. 이것이 "애니메이션 LOD"입니다. 메시 LOD가 렌더링 비용을 거리에 따라 단계적으로 제어하는 것과 같은 발상을 **애니메이션의 업데이트 빈도**에 적용합니다.

효과적인 상황은 다음과 같습니다.

* 화면 안에 수백 체 규모의 캐릭터가 있고 대부분이 원경이나 모브이다

* 먼 캐릭터의 30 fps / 15 fps 상당의 움직임을 체감상 허용할 수 있다(카메라가 멀수록 솎아내기가 눈에 띄지 않는다)

* 애니메이션 평가가 프레임 시간에서 무시할 수 없는 비율을 차지하고 있다

반대로 근거리에서 카메라에 크게 잡히는 캐릭터에 적용하면 끊김이 바로 눈에 띕니다. 거리나 화면 점유율에 따라 솎아내기 단계를 전환하는 것이 상정되는 실제 운용일 것입니다.

## **단순한 구현: \`Animator.Update()\`를 직접 호출한다**

\`animator.enabled \= false\`로 설정하여 Unity의 자동 업데이트를 멈추고, 필요한 프레임에서만 \`Animator.Update()\`를 명시적으로 호출합니다. 건너뛴 프레임 분량의 시간은 누적하여 전달합니다.

**이 구현에는 지금까지 설명한 Animation Job Thread에서의 병렬성을 잃는다는 부작용이 있습니다**.

CODE START csharp | Animator.Update()를 직접 호출하는 솎아내기 구현 {{list:animlod_manual_update}}  
animator.enabled \= false;   // 초기화 시. Unity의 자동 업데이트를 중지한다

void Update()  
{  
    foreach (var character in \_characters)  
    {  
        character.PendingDeltaTime \+= Time.deltaTime;  
        if (\_frameIndex % interval \!= character.Slot) continue;   // Slot \= index % interval

        character.Animator.Update(character.PendingDeltaTime);    // 누적한 시간으로 명시적으로 업데이트한다  
        character.PendingDeltaTime \= 0f;  
    }  
    \_frameIndex++;  
}  
CODE END

## **Playable API를 직접 사용한 구현 예**

수동 업데이트(\`DirectorUpdateMode.Manual\`)의 Playable Graph라면 \`Evaluate()\` 호출 자체를 솎아내기만 하면 됩니다.

CODE START csharp | 수동 업데이트 그래프의 Evaluate()를 솎아내는 구현 {{list:animlod_manual_evaluate}}  
graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);  
graph.Play();

void Update()  
{  
    foreach (var character in \_characters)  
    {  
        character.PendingDeltaTime \+= Time.deltaTime;  
        if (\_frameIndex % interval \!= character.Slot) continue;

        character.Graph.Evaluate(character.PendingDeltaTime);  
        character.PendingDeltaTime \= 0f;  
    }  
    \_frameIndex++;  
}  
CODE END

한 가지 주의할 점이 있습니다. 앞 장에서 권장한 "모든 캐릭터를 1개의 그래프로 집약한다"는 구성에서는 그래프 전체를 한 번의 \`Evaluate()\`로 평가하므로 **캐릭터 단위의 솎아내기를 표현할 수 없습니다**. 위 코드처럼 1체 1그래프로 하면 솎아낼 수 있지만, 그만큼 집약에 의한 병렬화를 잃게 됩니다.

어느 구현에서든 중요한 것이 \`Slot\`(위상)입니다. 모든 캐릭터를 같은 프레임에 업데이트하면 그 프레임에만 부하가 집중됩니다. \`index % interval\`로 위상을 분산하면 매 프레임 1/N체씩 업데이트되는 형태가 됩니다.

## **실측**

600체, Renderer 비활성화. 먼저 Animator에서의 구현입니다.

**평균값**은 12초 동안의 총 부하를, **중앙값과 p95의 차이**는 프레임 시간의 변동 정도를 나타냅니다.

表: 솎아내기와 위상 분산의 프레임 시간 통계(Animator·600체) {{table:animlod_animator_stats}}

| 방식 | 평균 | 중앙값 | p95 | p99 | 최대 | 표준편차 |
| :---- | :---- | :---- | :---- | :---- | :---- | :---- |
| 매 프레임 업데이트(Animator 자동) | 5.037 ms | 5.000 ms | 5.303 ms | 5.707 ms | 16.901 ms | 0.433 |
| 2프레임에 1회·위상 없음 | 4.781 ms | 0.648 ms | 9.607 ms | 9.816 ms | 12.194 ms | 4.544 |
| 2프레임에 1회·위상 분산 | 4.831 ms | 4.803 ms | 5.099 ms | 5.212 ms | 8.804 ms | 0.171 |
| 4프레임에 1회·위상 없음 | 2.613 ms | **0.382 ms** | **9.470 ms** | 9.658 ms | 11.171 ms | 3.863 |
| 4프레임에 1회·위상 분산 | 2.578 ms | **2.593 ms** | **2.707 ms** | 2.908 ms | 11.722 ms | 0.185 |

Playable API(1체 1그래프·수동 업데이트)에서도 같은 경향입니다.

表: 솎아내기와 위상 분산의 프레임 시간 통계(Playable API·1체 1그래프) {{table:animlod_playable_stats}}

| 방식 | 평균 | 중앙값 | p95 | 표준편차 |
| :---- | :---- | :---- | :---- | :---- |
| 매 프레임 업데이트 | 8.496 ms | 8.493 ms | 9.001 ms | 0.337 |
| 4프레임에 1회·위상 없음 | 2.438 ms | 0.369 ms | 8.855 ms | 3.570 |
| 4프레임에 1회·위상 분산 | 2.486 ms | **2.495 ms** | **2.613 ms** | 0.173 |

## **위상 분산은 "부하를 줄이는" 것이 아니라 "평탄하게 만드는" 것이다**

4분의 1로 솎아냈을 때의 평균값은 위상 유무에 따라 거의 변하지 않으며, **총 부하는 거의 같습니다**. 위상 분산은 작업을 줄이는 것이 아닙니다.

한편 프레임 시간의 분포에는 다음과 같은 차이가 나타납니다.

* 위상 없음: 4프레임 중 3프레임은 아무것도 하지 않고, 1프레임에서 600체 분량을 한꺼번에 처리한다. 중앙값은 내려가지만 p95와 표준편차가 급등한다

* 위상 분산: 매 프레임 150체씩 처리한다. 중앙값과 p95가 거의 일치하고 편차가 작다

중앙값 0.382 ms라는 수치만 보고 "1/13이 되었다"고 기뻐하면, 실제로는 9.5 ms의 스파이크가 4프레임에 한 번 발생하는 게임이 만들어집니다. **위상 분산은 체감 품질을 위한 대책**이며, 표준편차 0.185는 매 프레임 업데이트의 0.433보다 안정적입니다.

이 분포의 차이는 Profiler의 CPU 차트에 그대로 나타납니다(모두 Profiler 연결 상태의 캡처이므로 위 표의 비연결 수치보다 무겁게 나옵니다. 또한 차트의 세로축 스케일은 자동 조정이라 두 장이 서로 다릅니다).

![][image10]

図: 4프레임에 1회·위상 없음. CPU 차트가 "거의 0 ms인 골짜기 3프레임 \+ 약 10 ms 스파이크 1프레임"의 빗살 모양이 된다. 아래 Timeline에서는 선택 중인 빈 프레임(0.47 ms)의 양옆에 600체 분량을 한꺼번에 처리하는 \`ATB.ManualAnimatorUpdate\`(10.37 ms) 프레임이 보인다. {{fig:profiler_lod_decimated}}

![][image11]

図: 4프레임에 1회·위상 분산. 매 프레임 150체씩 처리하므로 파형은 평탄하고(약 3.5 ms) 스파이크가 사라졌다. {{fig:profiler_lod_staggered}}

## **단순한 구현은 병렬화를 잃는다**

**2프레임에 1회 솎아내기는 거의 아무것도 줄이지 못했습니다.** 업데이트 횟수를 절반으로 줄였는데도 평균 프레임 시간 절감은 겨우 **4.1 %**입니다.

이유는 Profiler 마커의 내역을 보면 알 수 있습니다.

表: 단순한 솎아내기 구현에서 잃게 되는 병렬성 {{table:animlod_parallel_loss}}

| 방식 | 1프레임에 업데이트하는 체 수 | 메인 스레드의 애니메이션 업데이트 시간 | \`Animator.ProcessGraph\` 전체 스레드 합계 | 그중 메인 스레드 |
| :---- | :---- | :---- | :---- | :---- |
| 매 프레임 업데이트(Unity의 배치 업데이트) | 600 | \`Animators.Update\` 4.046 ms | 20.104 ms | 1.257 ms |
| 2프레임에 1회·위상 분산 | 300 | \`ATB.ManualAnimatorUpdate\` 4.532 ms | 0.549 ms | 0.317 ms |
| 4프레임에 1회·위상 분산 | 150 | \`ATB.ManualAnimatorUpdate\` 2.316 ms | 0.278 ms | 0.180 ms |

매 프레임 업데이트에서는 작업 중 메인 스레드에 남는 것이 6 %뿐이고 나머지는 워커 스레드에 분산되어 있습니다. 반면 \`Animator.Update()\`를 명시적으로 호출하면 **그 자리에서 동기적으로 평가**되므로 워커 스레드가 거의 사용되지 않습니다. Unity의 배치 업데이트가 가지고 있던 병렬화를 솎아내기와 맞바꾸어 포기한 것입니다.

1체당 메인 스레드 비용으로 환산하면 다음과 같습니다.

* 매 프레임 업데이트: 4.046 ms / 600체 \= **6.7 µs/체**(병렬)

* 2프레임에 1회: 4.532 ms / 300체 \= **15.1 µs/체**(직렬)

* 4프레임에 1회: 2.316 ms / 150체 \= **15.4 µs/체**(직렬)

직렬화된 순간 1체당 비용이 약 2.3배가 되고, 그 후에는 솎아내기 배율과 관계없이 일정합니다. 즉 **2배 솎아내기로 얻는 2배 절감이 직렬화로 인한 2.3배 악화로 거의 상쇄됩니다**. 실측 4.1 %는 그 잔차에 불과합니다.

단순한 구현 그대로는 4배 솎아내기(**−48.8 %**)에서야 비로소 실질적인 효과가 나타납니다. 다만 이것은 피할 수 있는 손실입니다.

## **병렬성을 유지하면서 솎아내기**

\`Animator.Update()\`가 동기적인 것은 피할 수 없습니다. 하지만 **"Unity의 배치 업데이트에 참여시킬 체를 매 프레임 선택한다"**는 형태로 하면 솎아내면서도 병렬성을 유지할 수 있습니다. 두 가지 구현이 있으며, 둘 다 Animator(와 그 스테이트 머신)를 그대로 사용합니다.

### **방법 1: \`Animator.enabled\`를 전환한다**

Unity의 애니메이션 업데이트는 그 시점에 활성화된 Animator를 모아 하나의 배치로 만들고 Animation Job Thread에 분산합니다. 따라서 \`Animator.Update()\`를 호출하는 대신 **해당 프레임에 업데이트하고 싶은 체만 \`enabled \= true\`로 해 두면 됩니다**. 평가는 Unity가 수행하므로 병렬화가 그대로 유지됩니다.

CODE START csharp | Animator.enabled를 전환하는 솎아내기 구현 {{list:animlod_enable_toggle}}  
// 초기화 시. 건너뛴 프레임만큼은 speed로 만회한다  
foreach (var character in \_characters)  
{  
    var animator \= character.Animator;  
    animator.keepAnimatorStateOnDisable \= true;   // 비활성화로 스테이트를 되감지 않는다(필수)  
    animator.speed \= interval;                    // interval 프레임에 한 번만 업데이트되는 만큼을 보정한다  
}

void Update()  
{  
    foreach (var character in \_characters)  
    {  
        var scheduled \= \_frameIndex % interval \== character.Slot;  
        if (character.Animator.enabled \!= scheduled)  
        {  
            character.Animator.enabled \= scheduled;   // Unity의 배치 업데이트에 참여하는 대상을 선택할 뿐이다  
        }  
    }  
    \_frameIndex++;  
}  
CODE END

\`keepAnimatorStateOnDisable \= true\`를 잊어서는 안 됩니다. 기본값인 \`false\`에서는 Animator를 비활성화할 때마다 컨트롤러의 스테이트가 되감겨 애니메이션이 진행되지 않습니다.

제약은 \`Animator.speed\`가 **레이트**라는 점입니다. \`Animator.Update(pendingDeltaTime)\`처럼 "누적한 Δt를 정확히 전달"할 수 없고, 업데이트 프레임의 \`Time.deltaTime × interval\`만큼만 진행됩니다. 프레임 시간이 흔들리는 환경에서는 진행 방식도 흔들리지만 평균 재생 속도는 유지됩니다.

### **방법 2: 위상별로 그래프를 나눈다**

앞 장의 결론은 "N체를 하나의 수동 그래프로 모으면 병렬화된다"였습니다. 이를 솎아내기와 조합합니다. **위상 수만큼 그래프를 만들고 1프레임에 1개만 \`Evaluate()\`합니다**. 1개의 그래프에는 N/interval체가 들어 있으므로 그 1회의 \`Evaluate()\` 안에서 병렬화가 작동합니다.

스테이트 머신을 \`AnimatorControllerPlayable\`로서 그래프에 가지게 하면 Animator 컴포넌트에 할당했을 때와 같은 평가가 실행됩니다.

CODE START csharp | 위상별로 그래프를 나누는 솎아내기 구현 {{list:animlod_bucketed_graphs}}  
// 초기화 시: 위상 버킷마다 수동 그래프를 하나씩 만든다  
for (var bucket \= 0; bucket \< interval; bucket++)  
{  
    var graph \= PlayableGraph.Create($"Bucket\_{bucket}");  
    graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);  
    \_bucketGraphs\[bucket\] \= graph;  
}

foreach (var character in \_characters)  
{  
    var graph \= \_bucketGraphs\[character.Slot\];  
    character.Animator.runtimeAnimatorController \= null;   // 평가는 그래프 쪽에 맡긴다  
    var output \= AnimationPlayableOutput.Create(graph, "Animation", character.Animator);  
    output.SetSourcePlayable(AnimatorControllerPlayable.Create(graph, controller));  
}

foreach (var graph in \_bucketGraphs) graph.Play();

void Update()  
{  
    for (var bucket \= 0; bucket \< interval; bucket++) \_pendingDeltaTime\[bucket\] \+= Time.deltaTime;

    var scheduled \= \_frameIndex % interval;  
    \_bucketGraphs\[scheduled\].Evaluate(\_pendingDeltaTime\[scheduled\]);   // 한 프레임에 한 버킷만  
    \_pendingDeltaTime\[scheduled\] \= 0f;  
    \_frameIndex++;  
}  
CODE END

이쪽은 \`Evaluate()\`에 **누적한 Δt를 그대로 전달할 수 있으므로** 진행이 정확합니다. 대신 그래프를 솎아내기 단계 수만큼 가지게 되며, LOD 단계를 동적으로 바꾸려면 캐릭터를 그래프 사이에서 옮겨야 합니다(출력을 다시 만듦).

### **실측: 세 가지 구현의 비교**

600체, Renderer 비활성화. 평균 프레임 시간과 매 프레임 업데이트(5.037 ms) 대비 절감률입니다.

表: 세 가지 솎아내기 구현의 프레임 시간 비교(600체) {{table:animlod_impl_compare}}

| 구현 | 1/2 솎아내기 | 1/4 솎아내기 |
| :---- | :---- | :---- |
| \`Animator.Update()\`를 명시적으로 호출(위상 분산) | 4.831 ms(−4.1 %) | 2.578 ms(−48.8 %) |
| \`Animator.enabled\` 전환 | **3.350 ms(−33.5 %)** | **1.835 ms(−63.6 %)** |
| 위상별 수동 그래프(\`AnimatorControllerPlayable\`) | **3.082 ms(−38.8 %)** | **1.541 ms(−69.4 %)** |

**단순한 구현이 가장 수지가 맞지 않습니다.** 특히 1/2 솎아내기에서의 차이가 커서, −4.1 %가 −33.5 % / −38.8 %가 되었습니다. "2배 솎아내기는 병렬화 손실에 잠식된다"는 앞 절의 결론은 **구현을 바꾸면 성립하지 않습니다**.

병렬성이 유지되고 있다는 것은 Profiler 마커로 확인할 수 있습니다.

表: 세 가지 솎아내기 구현의 스레드별 내역 {{table:animlod_impl_threads}}

| 방식 | \`Animator.ProcessGraph\` 전체 스레드 합계 | 그중 메인 스레드 | 메인 스레드 측 업데이트 시간 |
| :---- | :---- | :---- | :---- |
| 매 프레임 업데이트(600체/프레임) | 20.104 ms | 1.257 ms | \`Animators.Update\` 4.046 ms |
| \`Animator.Update()\`·1/4(150체/프레임) | 0.278 ms | 0.180 ms | \`ATB.ManualAnimatorUpdate\` 2.316 ms |
| \`Animator.enabled\`·1/4(150체/프레임) | **4.424 ms** | 0.288 ms | \`Animators.Update\` 1.104 ms ＋ 전환 0.125 ms |
| 위상별 수동 그래프·1/4(150체/프레임) | **4.253 ms** | 0.279 ms | \`ATB.BucketEvaluate\` 1.278 ms |

새로운 두 구현에서는 **\`Animator.ProcessGraph\`의 작업량이 매 프레임 업데이트의 정확히 1/4**이 되며(1프레임에 150체만 평가하므로 당연합니다), 그 작업은 여전히 워커 스레드에 분산되어 있습니다. 반면 \`Animator.Update()\` 버전은 이 마커에 거의 계상되지 않고, 작업의 본체는 메인 스레드의 \`ATB.ManualAnimatorUpdate\` 안에 있습니다.

1체당 메인 스레드 비용으로 나열하면 차이가 분명해집니다.

* 매 프레임 업데이트: 4.046 ms / 600체 \= **6.7 µs/체**

* \`Animator.enabled\` 전환·1/4: 1.229 ms / 150체 \= **8.2 µs/체**

* 위상별 수동 그래프·1/4: 1.278 ms / 150체 \= **8.5 µs/체**

* \`Animator.Update()\`·1/4: 2.316 ms / 150체 \= **15.4 µs/체**

새로운 두 구현은 1체당 비용이 거의 매 프레임 업데이트 그대로입니다(차이는 전환이나 \`Evaluate\` 호출 오버헤드). 그래서 솎아내기 배율이 그대로 절감률에 가까워집니다.

재생 속도가 유지되고 있다는 점도 확인했습니다. 마지막에 스폰한 캐릭터(위상 슬롯이 가장 큰 체)의 손가락 본이 1초당 움직이는 거리를 계측하면, 매 프레임 업데이트 0.509 m/s에 대해 \`Animator.enabled\` 전환 0.496 m/s, 위상별 수동 그래프 0.517 m/s입니다. 솎아내기로 움직임이 직선으로 근사되는 만큼의 오차 범위 내에서, 어느 쪽도 속도가 변하지 않았습니다.

분포에 대해서는 한 가지 주의할 점이 있습니다. 1/2 솎아내기의 수동 그래프 버전은 p99가 9.102 ms, 표준편차가 1.086으로, \`enabled\` 전환 버전(p99 4.100 ms, 표준편차 0.562)보다 꼬리가 길게 나왔습니다. 1/4에서는 반대로 수동 그래프 버전이 더 안정적입니다(표준편차 0.306 대 0.340). 그래프 개수와 1개당 체 수의 균형에 따라 동작이 달라지므로, 채택한다면 평균뿐 아니라 p99도 확인하십시오.

어느 쪽을 택할지는 요구 사항에 따라 다릅니다. Animator의 스테이트 머신과 인스펙터 워크플로를 그대로 계속 사용하고 싶다면 \`Animator.enabled\` 전환이 압도적으로 간편합니다(몇 줄이면 되고 기존 코드에 대한 영향도 거의 없습니다). 진행의 정확성이나 Animation Job 삽입과 같은 그래프 측 제어가 필요하다면 위상별 수동 그래프입니다. **어느 쪽이든 \`Animator.Update()\`를 직접 호출하는 구현을 선택할 이유는 없습니다.**

## **이 장의 결론**

* 애니메이션 LOD는 "업데이트 빈도 솎아내기 \+ 위상 분산"의 세트로 설계한다

* 위상 분산은 총 부하를 줄이지 않는다(평균 2.613 대 2.578 ms). 줄이는 것은 **프레임 시간의 편차**(표준편차 3.863 → 0.185)이며, 이는 체감 품질을 위한 대책이다

* **\`Animator.Update()\`를 직접 호출하는 구현은 선택하지 않는다.** 그 자리에서 동기 평가되므로 잡 병렬화를 잃고, 1체당 메인 스레드 비용이 6.7 → 15.4 µs로 약 2.3배가 된다

* 대신 **Unity의 배치 업데이트에 참여하는 체를 매 프레임 선택한다**. \`Animator.enabled\` 전환(\`keepAnimatorStateOnDisable \= true\`와 \`Animator.speed\`에서의 보정이 필요)이나, 위상별로 수동 그래프를 나누어 \`AnimatorControllerPlayable\`를 올리는 형태이다. 1체당 8.2~8.5 µs로 수렴하며 병렬화도 남는다

* 구현을 바꾸면 **2배 솎아내기에서도 효과가 있다**: −4.1 %가 −33.5 % / −38.8 %가 되었다. 4배라면 −63.6 % / −69.4 %

* 캐릭터 단위의 솎아내기는 1개의 집약 그래프로는 표현할 수 없다. 다만 "위상 수만큼 그래프를 나눈다"면 병렬화와 업데이트 빈도 제어를 양립할 수 있다

* 중앙값만으로 판단하지 않는다. 총 부하는 평균으로, 체감은 p95·p99와 표준편차로 본다

{{pagebreak}}

# **정리**

각 장의 실측 결과입니다.

表: 각 장의 실측 결과 정리 {{table:overall_summary}}

| 장 | 시책 | 조건 | 결과 |
| :---- | :---- | :---- | :---- |
| 에셋 설정 | Optimize Game Objects | 600체·Renderer 1개 | 6.797 → 5.991 ms(**−11.9 %**) |
| 에셋 설정 | Optimize Hierarchy(런타임) | 600체·Renderer 1개 | 6.797 → 6.009 ms(**−11.6 %**) |
| 에셋 설정 | 위 두 방법 | 300체·전체 Renderer | 18.205 → 18.3~18.5 ms(개선 없음·렌더링 병목) |
| Job Thread | 수동 업데이트 그래프를 1개로 집약 | 600체 | 8.396 → 4.500 ms(**−46.4 %**) |
| Animation C\# Jobs | Two-Bone IK를 Animation Job화 | 300체·64회 해법 | 9.094 → 2.795 ms(**−69.3 %**) |
| Animation C\# Jobs | 추가로 Burst로 컴파일 | 300체·64회 해법 | 2.795 → 2.792 ms(프레임 시간은 차이 없음) |
| 애니메이션 LOD | 1/4 솎아내기·\`Animator.Update()\`를 명시적으로 호출 | 600체 | 평균 5.037 → 2.578 ms(**−48.8 %**) |
| 애니메이션 LOD | 1/4 솎아내기·위상별 수동 그래프 | 600체 | 평균 5.037 → 1.541 ms(**−69.4 %**) |
| 애니메이션 LOD | 1/2 솎아내기·\`Animator.Update()\`를 명시적으로 호출 | 600체 | 평균 5.037 → 4.831 ms(−4.1 %) |
| 애니메이션 LOD | 1/2 솎아내기·위상별 수동 그래프 | 600체 | 평균 5.037 → 3.082 ms(**−38.8 %**) |

## **최종 최적화 접근 방식**

이 글의 실측을 바탕으로 한, 대량 캐릭터 애니메이션 최적화의 진행 방법입니다.

**1\. 에셋 설정을 최적화한다**

우선 모델 자체의 버텍스 수와 본 수 사양을 적절한 값으로 정합시다. 기준이 되는 지원 기기에서 벤치마크를 계측하고 적절한 모델 레귤레이션을 정하는 것이 우선 이상적입니다. 그 위에서 Unity 측에서 Optimize Game Objects 적용을 검토합시다. 3D 타이틀에서 상류에 위치하는 모델 제작을 양산이 진행된 프로젝트 후반에 크게 수정하기는 어려우므로, 프로젝트 초반에 수행하는 것이 가장 좋습니다.

**2\. Animation Job Thread에서의 처리 병렬성을 높인다**

 Animator와 자동 업데이트 Playable Graph는 Unity가 자동으로 병렬화하지만, 수동 업데이트는 그래프 단위로만 병렬화됩니다. 캐릭터 제어 설계를 하는 시점에서 "Animation Job Thread를 효과적으로 활용할 수 있는가"를 염두에 둡시다.

**3\. Animation Job의 Burst 적용을 포함한 최적화를 수행한다**

 IK 등의 후처리는 우선 메인 스레드에서 Animation Job으로 옮깁니다.

더 많은 퍼포먼스가 필요한 경우에는 SIMD화를 적용할 수 있는 데이터·처리 설계를 적용합시다.

**4\. 애니메이션 LOD 등 애플리케이션 차원의 궁리를 한다**

에셋·엔진 설정·코드 최적화를 다 했다면, 마지막은 "애초에 애니메이션을 처리하지 않는다"는 궁리입니다.

프로젝트 요구 사항과 상의해야 하지만, 게임 경험상 허용할 수 있는 선을 탐색하면서 처리를 억제하는 구조를 검토합시다.

{{pagebreak}}

# **각주**

## **Unity 공식 문서**

* \[Playable API\](https://docs.unity3d.com/Manual/Playables.html) — Playable Graph 구성과 \`DirectorUpdateMode\`

* \[\`AnimatorUtility.OptimizeTransformHierarchy\`\](https://docs.unity3d.com/ScriptReference/AnimatorUtility.OptimizeTransformHierarchy.html) — 런타임에서의 계층 평탄화

* \[Model Import Settings: Rig（Optimize Game Objects）\](https://docs.unity3d.com/Manual/FBXImporter-Rig.html)

* \[\`IAnimationJob\`\](https://docs.unity3d.com/ScriptReference/Animations.IAnimationJob.html) — \`AnimationScriptPlayable\`에 삽입하는 잡

* \[\`AnimationScriptPlayable\`\](https://docs.unity3d.com/ScriptReference/Animations.AnimationScriptPlayable.html)

* \[C\# Job System\](https://docs.unity3d.com/Manual/JobSystem.html) — \`AnimationStream\`를 거치지 않고 병렬 처리를 작성하기 위한 구조

## **사용 에셋**

* \[유니티짱 3D 모델 데이터\](https://unity-chan.com/) — © Unity Technologies Japan/UCL

[image1]: assets/saito-shunsuke-1/media/image1.png

[image2]: assets/saito-shunsuke-1/media/image2.png

[image3]: assets/saito-shunsuke-1/media/image3.png

[image4]: assets/saito-shunsuke-1/media/image4.png

[image5]: assets/saito-shunsuke-1/media/image5.png

[image6]: assets/saito-shunsuke-1/media/image6.png

[image7]: assets/saito-shunsuke-1/media/image7.png

[image8]: assets/saito-shunsuke-1/media/image8.png

[image9]: assets/saito-shunsuke-1/media/image9.png

[image10]: assets/saito-shunsuke-1/media/image10.png

[image11]: assets/saito-shunsuke-1/media/image11.png