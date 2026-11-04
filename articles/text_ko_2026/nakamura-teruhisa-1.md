# **튜닝 바이블을 AI 코딩 에이전트의 스킬로 만들기**

이름 / 로마자: Teruhisa Nakamura  
소속: 주식회사 Applibot

# **들어가며**

이 책과 같은 튜닝 노하우에는 구조적인 약점이 하나 있습니다. 읽은 사람에게만 효과가 있다는 점입니다.  
각 장에는 실측으로 뒷받침된 지식이 가득하지만, 그 지식이 프로젝트의 코드 리뷰에서 실제로 지적되는지는 리뷰어가 해당 장을 읽었는지, 그리고 리뷰하는 순간에 떠올릴 수 있는지에 달려 있습니다. 팀원 전원이 모든 장을 암기하기를 기대할 수는 없으며, 운영 단계에서 인원이 교체되면 지식은 희미해집니다.  
한편, 코드를 작성하는 현장에는 AI 코딩 에이전트(Claude Code, OpenAI Codex, GitHub Copilot 등)가 상주하게 되었습니다. 리뷰 요청이나 CI에 통합하면 모든 diff에 같은 기준을 적용할 수 있습니다. 부족한 것은 Unity 퍼포먼스에 관한 체계적인 평가 기준뿐입니다.  
이에 이 장에서는 튜닝 바이블의 지식을 'AI 에이전트가 실행할 수 있는 평가 항목'으로 변환하고, 특정 도구에 의존하지 않는 범용 스킬로 패키징하는 방법을 소개합니다. 필자가 실제로 전작에서 공개한 지식에 기반한 77개 규칙에, 이 책(2026년판)의 원고에서 추출한 86개 규칙을 통합하여 총 163개의 평가 규칙으로 스킬화한 사례를 바탕으로, 변환의 설계 지침과 운용상의 주의점을 정리합니다.

# **Agent Skills라는 공통 포맷**

## **SKILL.md의 사양**

AI 에이전트에 전문 지식을 부여하는 방법으로, 2025년 말에 Agent Skills{{fn:agentskills}}라는 오픈 사양이 공개되었습니다. 스킬의 실체는 SKILL.md라는 Markdown 파일을 중심으로 한 디렉터리이며, 다음과 같은 구성을 취합니다.  
CODE START text | 스킬 디렉터리 구성 {{list:skill_layout}}  
unity-perf-review/  
├── SKILL.md          \# 메타데이터 \+ 실행 절차(필수)  
├── references/       \# 규칙 모음 등의 참조 문서  
├── scripts/          \# 실행 가능한 스크립트(선택)  
└── assets/           \# 템플릿 등(선택)  
CODE END  
SKILL.md의 맨 앞에는 프런트매터라 불리는 메타데이터를 작성합니다. 필수 필드는 name과 description 두 가지뿐입니다.  
CODE START yaml | SKILL.md의 프런트매터(description은 지면에 맞게 줄였고, metadata 등의 선택 필드는 생략) {{list:skill_frontmatter}}  
\---  
name: unity-perf-review  
description: \>  
  Unity 프로젝트의 코드, 에셋, 설정을 퍼포먼스 관점에서  
  리뷰한다. C\# 스크립트, 셰이더, .meta, ProjectSettings를  
  163 규칙과 대조하여 Critical / Warning / Info 지적을 출력한다.  
  Unity 퍼포먼스 리뷰나 최적화 조사를 요청받았을 때 사용한다.  
license: MIT  
\---  
CODE END  
중요한 것은 description을 쓰는 방식입니다. 에이전트는 사용자의 요청 내용과 description을 대조하여 스킬을 자동으로 발동하므로, '무엇을 하는지'뿐 아니라 '언제 사용하는지'까지 적어 두면 발동 정확도가 높아집니다.

## **주요 에이전트의 지원 현황**

이 사양의 가치는 여러 에이전트가 같은 포맷을 읽을 수 있다는 점에 있습니다. 2026년 8월 시점의 지원 현황을 {{table:agent_support}}에 나타냅니다.  
表: 주요 코딩 에이전트의 스킬 지원(2026년 8월 시점) {{table:agent_support}}

| 에이전트 | 지원 | 배치 위치 |
| :---- | :---- | :---- |
| Claude Code | SKILL.md 네이티브 지원{{fn:claudecode}} | .claude/skills/ 또는 \~/.claude/skills/ |
| OpenAI Codex | SKILL.md 지원 | .agents/skills/ 또는 \~/.agents/skills/ |
| GitHub Copilot | SKILL.md 지원{{fn:copilot}} | .github/skills/ |
| Gemini CLI | SKILL.md 지원{{fn:gemini}} | .gemini/skills/ 또는 \~/.gemini/skills/ |

Codex / Copilot / Gemini CLI는 공통 .agents/skills/ 배치도 지원합니다. 또한 Copilot은 프로젝트의 .claude/skills/도 읽으므로, .claude/skills/에 한 번 배치하면 Claude Code와 Copilot 양쪽에 적용됩니다(Copilot의 개인용 배치는 \~/.copilot/skills/ 또는 \~/.agents/skills/). 지원 현황과 배치 위치는 시점에 따라 달라지므로, 도입 시 각 에이전트의 공식 문서에서 1차 정보를 확인하시기 바랍니다.  
튜닝 노하우를 한 번 SKILL.md 형식으로 정리하면, 어떤 에이전트를 사용하는 멤버에게든 같은 리뷰 기준(규칙과 절차)을 배포할 수 있습니다. Agent Skills가 공통화하는 것은 패키지 형식과 절차이며, 모델이나 도구 권한의 차이로 인해 실행 결과까지 같아지지는 않습니다. 그럼에도 '리뷰 기준은 리포지토리에, 실행은 에이전트에'라는 분업이 성립합니다.

# **지식을 '평가 항목'으로 변환하기**

스킬화의 핵심은 SKILL.md를 쓰는 것이 아니라, 각 장의 지식을 에이전트가 기계적으로 적용할 수 있는 단위까지 분해하는 것입니다. 필자는 다음 포맷으로 통일했습니다.  
CODE START text | 규칙 포맷(ParticleSystem 장에서 추출한 예. grep\_pattern의 제외 조건과 대처의 상세 내용은 지면에 맞게 줄임) {{list:rule_format}}  
\#\#\# US11 ParticleSystem.Simulate에 큰 초 값 지정  
\- 레벨: Warning(Noise 모듈이 활성화된 경우 Critical)  
\- 대상: 코드 리뷰 / 에셋 설정  
\- 검출 패턴: Simulate의 첫 번째 인자에 큰 초 값을 전달하고 있다.  
  특히 NoiseModule이 활성화된 ParticleSystem과 함께 사용하는 경우  
\- grep\_pattern: \\.Simulate\\(  
\- 이유: Simulate는 지정한 시간까지 일괄로 시뮬레이션한다.  
  실측에서 20초를 지정했을 때 NoiseJob이 약 2000회 호출되어  
  약 50ms의 스파이크가 발생했다  
\- 대처: Noise가 활성화된 파티클에서는 Simulate 사용을 자제한다  
CODE END  
이 포맷에는 네 가지 설계 판단이 담겨 있습니다.

## **검출의 기계화 가능성을 명시한다**

각 규칙에는 grep\_pattern으로, C\# 코드와 Unity의 YAML(.meta, Prefab, ProjectSettings)에 대해 기계적으로 실행할 수 있는 검색 사양을 부여합니다. 대부분은 정규 표현식 하나이지만, 여러 줄에 걸친 대조나 '있어야 할 기술의 부재'를 검출하는 것도 포함합니다. 에이전트는 이 사양으로 1차 검출을 수행하고, 히트한 위치만 정독하여 판단합니다. 이를 통해 대규모 리포지토리에서도 현실적인 비용으로 리뷰할 수 있습니다.  
정규 표현식으로 쓸 수 없는 항목을 억지로 쓰지 않는 것도 같은 정도로 중요합니다. 예를 들어 'Addressables에 등록되지 않은 공유 에셋이 여러 번들에서 참조되고 있다'는 grep으로는 잡을 수 없습니다. 이런 규칙에는 (수동 확인)이라고 명기하고, Addressables Analyze 등의 확인 수단을 대처 항목에 적습니다. 검출 가능성을 속이면 에이전트가 오검출을 양산하거나, 검출할 수 없는 규칙을 말없이 건너뛰게 됩니다.

## **실측값을 이유 항목에 남긴다**

'이유: 퍼포먼스가 악화되므로'라는 규칙은 설득력이 없습니다. 원고에 있는 실측값을 그대로 이유 항목에 인용합니다.

* 비압축 텍스처: 2048x2048에서 Compressed 2.7MB / Uncompressed 16.0MB(약 6배 차이)  
* 1개체 1그래프의 수동 Playable: 600개체에서 8.396ms 대 전체 1그래프 4.500ms(1.87배 차이)  
* Simulate(20f)와 Noise의 병용: NoiseJob 약 2000콜, 약 50ms의 스파이크

수치가 있으면 에이전트의 지적이 그대로 '왜 고쳐야 하는가'에 대한 설명이 됩니다. 리뷰를 받은 개발자가 원고의 해당 장으로 되돌아가는 경로도 됩니다. 다만 모두 원고에 기재된 특정 환경에서의 실측값입니다. 규칙의 이유 항목에는 대표값을 인용하고, 계측 조건의 상세는 출처 장을 참조하도록 운용하고 있습니다.

## **레벨과 덮어쓰기로 운용을 견디게 한다**

규칙에는 Critical / Warning / Info의 3개 레벨(Critical=성능 저하의 확실한 원인으로 적용 조건 성립이 확인되면 수정 필수, Warning=조건에 따라 문제가 되는 잠재 리스크, Info=선택적인 개선 권장)을 부여하고, 프로젝트 측에서 덮어쓸 수 있는 구조(rules-overrides)를 마련합니다. 예를 들어 'LINQ 사용은 Critical'이더라도 초기화 시(init 컨텍스트)에는 Warning으로 완화하고, Editor 도구(tooling)에서는 Skip하는 식의 조정입니다.  
튜닝 규칙은 본질적으로 컨텍스트에 의존합니다. 덮어쓰기 구조가 없으면 '우리 프로젝트에는 맞지 않는 지적이 많다'며 스킬 전체가 무효화됩니다. 기준은 공통, 적용은 프로젝트 고유라는 2층 구조가 오래 유지되는 비결입니다.

## **정적으로 검출할 수 없는 지식은 진단 노하우로 분리한다**

원고의 지식 중에는 코드를 읽어도 알 수 없는 것이 많이 있습니다. 'CPU 처리를 줄였는데도 프레임 시간이 변하지 않고 Semaphore.WaitForSignal 등의 대기 마커로 시간이 옮겨 간 경우에는, 렌더 스레드 쪽이 크리티컬 패스에 있을 가능성을 의심하고 Timeline 뷰에서 대기 대상을 확인한다'와 같은, 프로파일러 읽는 법에 관한 지식입니다. 대기 마커만으로는 병목 요인을 단정할 수 없다는 한계도 지식의 일부로 남깁니다.  
이들은 평가 규칙과는 별도 파일에 '증상 → 원인 → 대처' 표로 분리했습니다({{table:diagnosis_example}}). 필자의 리포지토리에서는 프로파일 분석용 에이전트(Claude Code용)가 진단 시에만 읽어 들입니다. 이 장에서 배포하는 스킬에는 참조 자료로 동봉되어 있으며, 정적 리뷰로 판단할 수 없는 의심 사항을 '프로파일 계측 권장'으로 보고할 때의 근거가 됩니다.  
表: 진단 노하우의 예 {{table:diagnosis_example}}

| 증상 | 원인 | 대처 |
| :---- | :---- | :---- |
| CPU 차트가 골과 스파이크가 번갈아 나타나는 빗 모양 | 솎아내기(간격 실행)의 위상 어긋남 누락 | index % interval로 위상 분산 |
| MeshSkinning.CalcMatrices가 큼 | 본 Transform 계층의 실체화 | Optimize Game Objects 적용 |
| iOS에서만, 에러와 Shader.CreateGPUProgram이 없고 GpuProgramMetal.GetCachedPipeline만 발생 | 셰이더의 AssetBundle 중복 패킹을 의심 | 중복 인스턴스를 확인하고 전용 번들화 |
| iOS에서만, 웜업 완료 후에도 SubPass 내 셰이더에서 PSO 컴파일 | SubPass(Native Render Pass) 내 셰이더의 웜업 실패 | SubPass 재검토 또는 로딩 중 프리렌더링 |

# **스킬로 조립하기**

## **단계적 로딩과 체크리스트 방식**

163개 규칙을 모두 SKILL.md 본문에 쓰면 에이전트의 컨텍스트를 압박합니다. Agent Skills의 설계 사상은 단계적 로딩(progressive disclosure)입니다.

1. 에이전트는 항상 프런트매터의 name과 description만 보고 있다  
2. 스킬이 발동하면 SKILL.md 본문(실행 흐름)을 읽는다  
3. 필요해진 참조 문서만 references/에서 읽는다

초기에 무엇을 읽는지(경로 등을 포함하는지)는 에이전트 구현에 따라 다르지만, '본문과 참조 자료는 발동할 때까지 읽히지 않는다'는 원칙은 공통입니다.  
여기서 주의할 점이 망라성입니다. 단순하게는 '대상 파일 종류에 따라 카테고리별 규칙 파일을 통째로 읽어 들이는' 구성을 생각할 수 있지만, 이 방식에서는 에이전트가 코드보다 먼저 읽는 규칙이 최대 2만 토큰을 넘는 분량(문자 수로 계산한 개산이며, 토큰 수는 모델에 따라 다릅니다)에 달합니다. 수십 개의 규칙을 '전부 기억해서 적용'하도록 모델의 주의력에 의존하면, 일부 규칙이 말없이 누락됩니다.  
그래서 전체 규칙에서 'ID / 레벨 / 적용 조건 / grep\_pattern / 규칙명 / 상세 파일명'을 규칙당 1행으로 압축한 체크리스트를 자동 생성하고, 리뷰를 2단계로 나누었습니다.

1. **1차 스캔**: 동봉된 스크립트 scan.py가 체크리스트 전체 행의 grep\_pattern을 대상 파일에 기계적으로 실행하여, 히트 목록(파일:행)과 Coverage를 출력합니다(python3가 없는 환경에서는 에이전트가 전체 행의 grep을 직접 실행하는 폴백)  
2. **2차 정밀 검토**: 히트한 규칙에 대해서만 상세 파일에서 검출 조건, 이유, 대처를 읽어 들여 오검출을 제외합니다

grep으로 표현할 수 없는 일부 규칙은 체크리스트에 MANUAL이라고 명기하고, 대상 종류에 해당할 때만 정독합니다. '있어야 할 기술의 부재'가 지적 조건이 되는 규칙에는 ABSENT를 붙이며, grep이 히트하지 않았을 때 상세를 확인합니다(히트 여부와 지적 조건의 대응이 뒤바뀌지 않게 하기 위한 명시입니다). 1차 스캔을 스크립트로 만든 것은, LLM에 수십 개의 grep을 하나씩 실행시키면 누락이 생길 수 있고, 정규 표현식 방언 차이(PCRE의 lookahead는 macOS 표준 grep에서 동작하지 않는 등)로 결과가 환경에 의존하게 되기 때문입니다. 이로써 1차 스캔의 망라성은 모델의 기억력과 분리됩니다. 지적의 정확성(2차 정밀 검토에서의 오검출 제외)은 계속 에이전트의 판단입니다. 사전에 읽는 것은 체크리스트뿐이며(코드용 약 16KB, 에셋용 약 8KB. 토큰 수는 모델에 따라 다르지만 합계 1만 토큰 안팎으로 개산), 상세는 히트한 분량만 읽습니다. 체크리스트는 규칙 본체에서 자동 생성하므로, 규칙을 추가해도 수동 동기화 작업은 발생하지 않습니다.

## **환경에 따른 규칙 제외**

규칙 중에는 Unity 버전이나 구성에 의존하는 것이 있습니다. 이 스킬에 수록한 RenderGraph 관련 규칙(URP용)은 Unity 6 이후가 대상이고, LTO 계열은 IL2CPP 전용이며, iOS 고유의 PSO 문제는 Android 전용 타이틀과는 무관합니다. 해당하지 않는 규칙을 대조하는 것은 노이즈와 수고만 늘릴 뿐입니다.  
그래서 환경 의존 규칙에는 적용 조건 태그(unity\>=6000.3, urp, il2cpp, ios 등)를 부여하고, 프로젝트의 설정 파일에서 생성한 환경 프로파일과 대조하여 '명확하게 조건을 충족하지 않는 규칙만' 제외합니다. 태그 대조는 1차 스캔 스크립트가 결정론적으로 처리하며, 검출하지 못한 항목(unknown)은 안전한 쪽(대조 대상으로 유지)으로 처리하고, 건너뛴 규칙 ID는 리포트에 열거하여 감사할 수 있게 합니다. 생성 절차는 도입 절차 절에서 설명합니다.

## **출력에 규칙 ID를 강제한다**

출력 포맷에서는 각 지적에 규칙 ID 부여를 필수로 합니다.  
CODE START text | 리뷰 출력의 예 {{list:review_output}}  
\#\#\# Critical  
\- Assets/Scripts/EffectPlayer.cs:42 \-  
  Noise가 활성화된 파티클에 대한 Simulate(20f) 호출 \[US11\]  
  \- 근거: NoiseJob이 약 2000회 호출되어 약 50ms의 스파이크 발생(실측)  
  \- 권장: Noise를 제거하거나 초 값을 최소화한다  
CODE END  
ID가 있으면 지적의 근거를 규칙 파일까지 거슬러 올라가 검증할 수 있어, '에이전트가 그럴듯하게 말하고 있을 뿐'인 경우와 구별됩니다. 오검출이었을 때도 어느 규칙의 검출 패턴을 고치면 되는지가 명확합니다.

# **배포와 각 에이전트로의 배치**

스킬의 공개는 디렉터리째 공유하는 것만으로 성립합니다. GitHub 리포지토리든 zip이든 상관없습니다. 이용자가 {{table:agent_support}}의 배치 위치에 복사하면 각 에이전트가 스킬을 발견합니다.  
운용상의 포인트는 규칙의 단일 소스를 유지하는 것입니다. 필자의 리포지토리에서는 규칙 본체를 rules/ 디렉터리에만 두고, 배포용 패키지는 스크립트로 조립합니다(이용자 측의 도입 절차는 다음 절).  
CODE START bash | 배포 패키지 조립(메인테이너용) {{list:packaging}}  
./package-skill.sh  
cp \-r dist/unity-perf-review /path/to/repo/.claude/skills/   \# Claude Code  
cp \-r dist/unity-perf-review /path/to/repo/.agents/skills/   \# Codex / Copilot / Gemini CLI  
cp \-r dist/unity-perf-review /path/to/repo/.github/skills/   \# Copilot 전용 배치(위 두 줄 중 어느 쪽으로도 전달되므로 선택 사항)  
CODE END  
에이전트별로 규칙을 이중 관리하기 시작하면 업데이트할 때마다 기준이 어긋나게 됩니다. '소스는 한 곳, 배치는 스크립트'를 처음에 정해 두기를 권장합니다.

# **자신의 프로젝트에 도입하기**

지금까지의 구조를 독자의 프로젝트에서 동작시키는 절차입니다. python3가 설치되어 있다면 복사와 초기 생성에 10분 정도가 소요됩니다. 리포지토리에 추가되는 것은 스킬 디렉터리와 환경 프로파일(tuning-env.md)뿐이며, Unity의 Assets / ProjectSettings는 변경하지 않습니다.

## **스킬 가져오기와 배치**

스킬 일체(SKILL.md, 규칙 163건을 동봉한 references/, 환경 검출·1차 스캔·체크리스트 재생성의 3개 스크립트, 도입 절차 README)는 이 책 리포지토리의 skills/unity-perf-review/에서 공개하고 있습니다{{fn:repo}}. 가져와서 사용 중인 에이전트의 스킬 디렉터리에 복사하기만 하면 됩니다. 다만 스킬은 '에이전트에 대한 지시와 실행 가능한 스크립트'를 포함하는 배포물입니다. 신뢰할 수 없는 배포처의 스킬은 SKILL.md와 scripts/의 내용을 확인한 뒤 도입하고, 버전을 고정하기를 권장합니다.  
CODE START bash | 스킬 가져오기와 배치 {{list:install_steps}}  
git clone https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026.git  
cd UnityPerformanceTuningTips2026/skills

\# 사용 중인 에이전트에 따라 어느 한쪽으로 복사한다  
cp \-r unity-perf-review /path/to/your-repo/.claude/skills/   \# Claude Code  
cp \-r unity-perf-review /path/to/your-repo/.agents/skills/   \# Codex / Copilot / Gemini CLI(공통 배치)  
cp \-r unity-perf-review /path/to/your-repo/.github/skills/   \# Copilot 전용 배치(위 두 줄 중 어느 쪽으로도 전달되므로 선택 사항)  
CODE END  
리포지토리 최상위에 배치하면 팀원 전원에게 같은 리뷰 기준이 배포됩니다. 개인적으로 시험해 보기만 한다면 홈 디렉터리 쪽(\~/.claude/skills/ 등)에서도 동작합니다.

## **배치 후의 상태**

Claude Code의 경우, 배치와 다음 항목의 환경 프로파일 생성이 끝나면 리포지토리는 다음 상태가 됩니다({{list:installed_layout}}).  
CODE START text | 배치 후의 리포지토리 구성 {{list:installed_layout}}  
your-repo/  
|-- .claude/  
|   \`-- skills/  
|       \`-- unity-perf-review/  
|           |-- SKILL.md                      \# name/description \+ 실행 흐름  
|           |-- README.md                     \# 사람을 위한 도입 절차  
|           |-- references/  
|           |   |-- checklist-code.md         \# 1차 스캔용(코드 계열)  
|           |   |-- checklist-asset.md        \# 1차 스캔용(에셋/설정 계열)  
|           |   |-- checklist-profile.md      \# 프로파일 분석용(정적 리뷰에서는 사용하지 않는 부속 자료)  
|           |   |-- rules-csharp.md 등 총 13개 파일  \# 상세 규칙(총 163개)  
|           |   |-- diagnosis-knowledge.md    \# 증상-\>원인-\>대처 진단 모음  
|           |   \`-- rules-overrides.example.md  
|           \`-- scripts/  
|               |-- detect-env.py             \# 환경 프로파일 생성  
|               |-- scan.py                   \# 1차 스캔의 기계 실행  
|               \`-- gen-checklists.py         \# 규칙 수정 시 체크리스트 재생성  
|-- tuning-env.md                             \# 환경 프로파일(다음 항목에서 생성)  
\`-- (Unity 프로젝트 본체는 변경 없음)  
CODE END  
3장의 체크리스트는 대상별(코드 / 에셋 / 프로파일)이며, 하나의 규칙이 여러 장에 실리는 경우가 있으므로 행 수의 합계는 규칙 수 163과 일치하지 않습니다.  
배치한 시점에 에이전트에 올라가는 것은 원칙적으로 프런트매터의 name과 description뿐입니다(합계 수백 토큰. 앞서 설명했듯이 구현에 따라 경로 등이 추가됩니다). 리뷰를 요청하면 앞서 설명한 단계적 로딩에 따라 SKILL.md 본문 → 대상에 맞는 체크리스트 → 히트한 규칙의 상세 순으로 필요한 분량만 읽힙니다. 보통은 163개 규칙 전문(약 125KB, 개산으로 4만 토큰 남짓)을 한꺼번에 투입하지 않아도 됩니다.  
rules-overrides.example.md는 그대로는 무효입니다. 프로젝트 고유의 조정을 시작할 때 rules-overrides.md로 이름을 바꾸고 편집합니다(후술).

## **환경 프로파일 생성**

앞서 설명했듯이 163개 규칙에는 환경 의존적인 것이 포함되어 있습니다. 프로젝트에 해당하지 않는 규칙을 제외 판단하게 하기 위해 환경 프로파일을 생성합니다.  
CODE START bash | 환경 프로파일 생성(clone한 리포지토리의 skills/에서 실행) {{list:detect_env}}  
python3 unity-perf-review/scripts/detect-env.py \\  
    /path/to/your-repo /path/to/your-repo/tuning-env.md  
CODE END  
배치 후에 실행하는 경우에는 경로를 배치 위치(.claude/skills/나 .agents/skills/ 등, 사용 중인 에이전트의 스킬 디렉터리)로 바꿔 읽으시기 바랍니다. Unity 프로젝트가 리포지토리 최상위가 아니라 하위 디렉터리에 있는 모노레포 구성에서도, 리포지토리 루트를 전달하면 설정 파일을 재귀적으로 탐색하여 검출합니다.  
스크립트 실행에는 python3가 필요합니다(없는 환경에서는 tuning-env.md를 수동으로 작성하거나, 이 절차를 건너뛰고 전체 규칙 대조로 운용할 수 있습니다. 1차 스캔의 scan.py도 python3를 사용하지만, 없는 경우에는 에이전트가 grep을 직접 실행하는 폴백으로 동작합니다). 스크립트는 ProjectVersion.txt, Packages/manifest.json, GraphicsSettings.asset, ProjectSettings.asset에서 Unity 버전, 렌더 파이프라인, Scripting Backend, Addressables 유무를 추정합니다. 어디까지나 설정 파일로부터의 추정값이며, 확정 정보는 tuning-env.md에 수동으로 기입한 내용이 정본입니다. 검출하지 못한 항목(플랫폼별로 설정이 나뉘는 Scripting Backend가 혼재된 경우 등)은 unknown이 되어 안전한 쪽(대조 대상으로 유지)으로 처리됩니다. 출력된 tuning-env.md를 열어 내용을 확인하고, 특히 출시 플랫폼과 SideFX VAT의 사용 여부는 수동으로 보정하며, render\_pipeline도 의도한 값인지 확인하시기 바랍니다.

## **프로젝트 고유의 조정(선택)**

의도적으로 규칙에서 벗어난 부분이 있는 경우에는 rules-overrides.md에 레벨 덮어쓰기를 기록합니다. 초기화 시의 LINQ를 허용하거나, Editor 도구에서는 로그 규칙을 무효화하는 등의 조정입니다. 환경 프로파일이 '그 환경에서는 원리적으로 적용 불가능한' 규칙의 제외를 담당하고, overrides가 '프로젝트 사정에 따른 조정'을 담당하는 역할 분담입니다.  
규칙 자체를 추가·수정하고 싶은 경우에는 references/의 rules-\*.md를 편집하고, python3 scripts/gen-checklists.py references/로 체크리스트를 재생성합니다. 체크리스트는 자동 생성물이므로 직접 편집하지 않습니다. 배포물만으로 이 추가·재생성 사이클이 완결됩니다.

## **실행하기**

에이전트에 요청하기만 하면 됩니다. 보통은 요청문과 description의 일치로 자동 발동합니다(발동은 모델 판단이므로, 확실히 사용하게 하고 싶다면 요청문에 스킬 이름 unity-perf-review를 명기하거나 각 에이전트의 명시적 호출을 사용합니다).  
CODE START text | 리뷰 요청 예 {{list:invoke_example}}  
이 diff를 퍼포먼스 관점에서 리뷰해 줘  
Assets/Scripts/Battle/ 하위를 튜닝 바이블의 규칙으로 체크해 줘  
CODE END  
출력 마지막에 붙는 Coverage 섹션에서 대조한 규칙 수, 환경에 따라 건너뛴 규칙 수(ID 포함), grep 히트 수를 확인할 수 있습니다. 이 수치들은 1차 스캔 스크립트 출력을 옮겨 적은 것이며 에이전트의 기억에 의존하지 않습니다(2차 정밀 검토에서의 채택·오검출 제외 건수만 에이전트가 집계합니다). 지나치게 제외된다고 느껴지면 tuning-env.md를, 지적이 너무 많다고 느껴지면 rules-overrides.md를 조정하시기 바랍니다.

# **운용 시 유의할 점**

실제로 프로젝트에 도입하면서 알게 된 주의점을 소개합니다.  
**처음에는 리포지토리 전체가 아니라 diff나 특정 디렉터리부터 시작합니다.** 1차 스캔 자체는 프로젝트 전체에서도 짧은 시간에 끝나지만, 운영 이력이 있는 코드베이스에서는 히트하는 규칙이 수십 건 규모가 되어 2차 정밀 검토(오검출 제외)의 부하가 거기에 집중됩니다. 좁은 범위에서 정밀 검토하고, 상시 허용하는 것을 overrides에 반영한 뒤 대상을 넓히는 편이 지적이 노이즈에 묻히지 않고 정착합니다.  
**정적 분석은 의도한 설정도 지적합니다.** 예를 들어 의도적으로 비압축으로 둔 텍스처도 AS02(텍스처 압축 미설정)에서 히트합니다. 검출 결과를 그대로 믿고 일괄 수정하지 말고, 한 건씩 의도를 확인한 뒤 고쳐야 합니다. 상시 허용하는 것은 overrides에 이유와 함께 기록하면 다음 회부터 노이즈를 줄일 수 있습니다. overrides의 조정 단위는 규칙 전체 또는 컨텍스트(in-game=핫 패스 / init=초기화 / tooling=Editor)이며, 특정 에셋만의 예외는 비고란에 경로와 이유를 적어 에이전트의 판단 재료로 삼습니다(전용 적용 범위 지정은 현재 버전에서는 지원하지 않습니다).  
**실측값에는 계측 환경이 딸려 있습니다.** 규칙에 인용한 수치는 특정 Unity 버전과 기기에서의 실측입니다. 'Pixel 8a에서의 실측'을 최신 하이엔드 기기의 판단에 외삽할 수는 없습니다. 규칙 자체에 대상 버전을 적고, Unity나 렌더 파이프라인의 업데이트를 규칙 재검토의 트리거로 삼습니다.  
**최종 판단은 사람이 합니다.** 이 책의 다른 장의 표현을 빌리면 '규약을 정하고 재검토하는 것은 사람, 지키게 하는 것은 기계'입니다. AI 에이전트는 '지키게 하는 기계'의 새로운 구현일 뿐이며, 규약=평가 항목의 타당성에 책임을 지는 것은 여전히 사람입니다. 에이전트의 지적이 납득되지 않는다면, 그것은 규칙의 검출 패턴이나 이유 항목을 개선하라는 신호로 받아들이시기 바랍니다.

# **마치며**

튜닝 바이블의 지식을 규칙화해 보며 실감한 것은 '실측값과 대처가 함께 적힌 원고는 거의 그대로 기계가 읽을 수 있는 평가 항목이 된다'는 점입니다. 반대로 말하면, 근거가 되는 수치가 없는 경험칙은 규칙화 단계에서 약점이 드러납니다. 이는 집필 측에 대한 피드백이기도 합니다. 계측 환경과 수치를 명기한 지식은 책의 독자뿐 아니라 AI 에이전트를 통해 모든 코드 리뷰에 전달됩니다.  
이 장에서 소개한 스킬과 규칙집은 유일한 정답이라기보다 출발점입니다. 각 프로젝트의 overrides가 성장하고, 새로운 장의 지식이 규칙으로 추가되어 가는, 살아 있는 규약집이 되기를 기대합니다.  
脚注: {{fn:repo}} https://github.com/CyberAgentGameEntertainment/UnityPerformanceTuningTips2026/tree/main/skills/unity-perf-review  
脚注: {{fn:agentskills}} Agent Skills 사양 https://agentskills.io/specification  
脚注: {{fn:claudecode}} Claude Code의 Skills 문서 https://code.claude.com/docs/en/skills  
脚注: {{fn:copilot}} GitHub Docs "About agent skills" https://docs.github.com/en/copilot/concepts/agents/about-agent-skills  
脚注: {{fn:gemini}} Gemini CLI의 Agent Skills 문서 https://geminicli.com/docs/cli/using-agent-skills/