# **UniTask와 취소 처리의 라이프타임 관리**

이름 / 로마자：Inada Taketo

소속：주식회사 사이버에이전트 SGE 코어 기술본부

X 계정 / 웹사이트 / GitHub 계정(선택):

UniTask를 사용한 비동기 처리에서 종료 조건이나 취소를 적절히 관리하지 않으면 완료되지 않는 처리나 불필요한 참조가 계속 남을 수 있습니다.

전작 『Unity 퍼포먼스 튜닝 바이블』에서는 MonoBehaviour가 파괴된 후에도 UniTask가 계속 대기하는 사례를 소개했습니다. 이 절에서는 CancellationTokenSource의 해제 누락으로 인해 불필요한 오브젝트가 계속 참조되는 사례를 소개합니다.

참고로 CancellationTokenSource와 CreateLinkedTokenSource는 UniTask 고유의 기능이 아니라 .NET의 취소 처리를 구성하는 API입니다.

이 절에서 다루는 코드 중 using 선언(using var)은 C\# 8 구문으로, Unity 2020.2 이상에서 사용할 수 있습니다. 또한 뒤에서 설명하는 CreateLinkedTokenSource의 내부 구현은 Unity Technologies가 공개한 Unity 2022.3용 Mono 구현(unity-2022.3-mbe 브랜치)을 참조한 것이며, Unity 버전에 따라 달라질 수 있습니다.

### **CancellationTokenSource의 Dispose**

CancellationToken은 처리에 취소 요청을 전달하기 위한 값 타입입니다. CancellationToken 자체는 IDisposable을 구현하지 않으므로 Dispose를 호출할 필요가 없습니다.

다만 CancellationToken.Register로 콜백을 등록한 경우 그 반환값인 CancellationTokenRegistration은 IDisposable입니다{{fn:cancellation_token_registration_dispose}}. 취소 전에 등록된 콜백은 발행처인 CancellationTokenSource가 보유하므로, 등록이 불필요해진 시점에 Dispose하여 해제하지 않으면 콜백이 참조하는 오브젝트가 해제되지 않을 수 있습니다.

반면 CancellationTokenSource는 IDisposable을 구현합니다. 사용을 마친 CancellationTokenSource는 소유자가 Dispose를 호출합니다.

CODE START csharp | CancellationTokenSource를 Dispose하는 예 {{list:cts_dispose}}

private async UniTask LoadCancelableAsync()

{

	using var cts \= new CancellationTokenSource();

	

	// 취소를 요청하는 처리는 생략

	await LoadAsync(cts.Token);

}

CODE END

{{list:cts_dispose}}에서는 using 선언으로 CancellationTokenSource를 관리합니다. 비동기 처리가 정상 종료된 경우뿐 아니라 취소나 예외로 종료된 경우에도 메서드를 빠져나올 때 cts.Dispose()가 자동으로 호출됩니다.

Cancel과 Dispose는 역할이 다릅니다. Cancel은 처리에 취소를 요청하고, Dispose는 CancellationTokenSource가 보유한 타이머나 콜백 등록 등을 해제합니다. Dispose를 호출하는 것만으로는 처리에 취소 요청이 통지되지 않습니다.

참고로 Dispose를 호출하지 않더라도 CancellationTokenSource와 관련 오브젝트가 모두 외부에서 참조되지 않게 되면 GC의 회수 대상이 됩니다{{fn:managed_memory}}. 다만 수명이 긴 CancellationTokenSource에 콜백을 등록하는 등의 이유로 참조가 남을 수 있으므로, 사용을 마친 시점에 Dispose하는 것을 원칙으로 하시기 바랍니다.

### **CancellationTokenSource.CreateLinkedTokenSource**

CancellationTokenSource.CreateLinkedTokenSource는 여러 원본 토큰을 감시하는 새로운 CancellationTokenSource를 생성합니다. 원본 토큰 중 하나라도 취소되면 생성된 CancellationTokenSource의 Token이 취소 상태가 됩니다.

원본 토큰끼리 서로 취소되는 것은 아니며, 취소는 원본 토큰에서 링크된 쪽으로 전파됩니다.

이하에서는 tokenA와 tokenB의 발행처인 CancellationTokenSource를 각각 A, B라고 합니다.

Unity Technologies가 GitHub에 공개한 Unity 2022.3용 Mono 구현{{fn:mono_cancellation_token_source}}에서 CreateLinkedTokenSource는 원본 토큰에 취소 콜백을 등록합니다. 참조 관계와 관련된 부분을 간략화하면 {{list:linked_source_internal}}과 같습니다.

CODE START csharp | CreateLinkedTokenSource 내부 구현의 핵심 {{list:linked_source_internal}}

linkedCts.m\_linkingRegistrations\[0\] \=

	tokenA.InternalRegisterWithoutEC(

    	s\_LinkedTokenCancelDelegate,

    	linkedCts);

 

linkedCts.m\_linkingRegistrations\[1\] \=

	tokenB.InternalRegisterWithoutEC(

    	s\_LinkedTokenCancelDelegate,

    	linkedCts);

CODE END

원본 토큰에 등록된 콜백이 linkedCts를 참조하고, linkedCts도 A와 B에 대한 등록 정보를 보유합니다.

따라서 B가 애플리케이션 전체에서 사용되는 수명이 긴 CancellationTokenSource이고 오랫동안 취소되지 않는 경우, linkedCts를 Dispose하지 않으면 다음 참조가 남을 수 있습니다.

수명이 긴 B → 콜백 등록 → linkedCts → A에 대한 등록 정보 → 수명이 짧은 A

이 참조 관계만으로 반드시 메모리 누수가 발생하는 것은 아닙니다. 모든 오브젝트가 외부에서 참조되지 않게 되면 한꺼번에 GC의 회수 대상이 됩니다. 그러나 B가 살아 있고 링크용 콜백이 해제되지 않은 동안에는 linkedCts가 B에서 계속 참조되며, 거기에서 수명이 짧은 A도 도달 가능한 상태로 남습니다.

또한 linkedCts.Token에 등록된 콜백이 다른 오브젝트를 참조하고 있는 경우 그 오브젝트들도 유지될 수 있습니다.

linkedCts를 Dispose하면{{fn:cancellation_token_source_dispose}} 내부에서 A와 B에 대한 CancellationTokenRegistration이 Dispose되어 원본 토큰에 등록된 콜백이 해제됩니다.

CreateLinkedTokenSource의 반환값은 링크한 토큰을 사용하는 처리의 스코프 내에서 using 선언 등으로 관리하는 것이 좋습니다.

CODE START csharp | 링크한 CancellationTokenSource를 Dispose하는 예 {{list:linked_source_using}}

private async UniTask LoadAsync(

	CancellationToken tokenA,

	CancellationToken tokenB)

{

	using var linkedCts \=

        CancellationTokenSource.CreateLinkedTokenSource(

        	tokenA,

        	tokenB);

 

	await LoadAssetAsync(linkedCts.Token);

}

CODE END

{{list:linked_source_using}}에서는 LoadAssetAsync가 정상 종료, 취소 또는 예외로 종료된 뒤에 linkedCts.Dispose()가 호출되어 원본 토큰에 대한 등록이 해제됩니다.

링크된 CancellationTokenSource가 이미 취소 상태이더라도 다른 원본 토큰에 대한 등록이 남아 있을 수 있습니다. CreateLinkedTokenSource의 반환값은 불필요해진 시점에 반드시 Dispose하시기 바랍니다.

### **IDisposable 오브젝트의 Dispose 누락을 팀에서 방지하기**

지금까지 CancellationTokenSource를 예로 IDisposable의 Dispose 누락을 살펴보았습니다. 이 대책은 오브젝트의 소유자가 적절한 시점에 Dispose를 호출한다는 것을 전제로 합니다. 그러나 프로젝트 규모가 커지면 모든 생성 위치를 코드 리뷰만으로 계속 확인하기는 어렵습니다. 특히 CreateLinkedTokenSource는 짧은 코드로 생성할 수 있어서 반환값이 IDisposable이라는 점을 놓치기 쉽습니다.

팀에서 재발을 방지하려면 개인의 주의에 더해, Dispose가 필요한 오브젝트를 다루기 쉬운 API로 모으는 것, Analyzer로 문제를 조기에 검출하는 것, 필요에 따라 CI에서 새로운 위반을 검출하는 것을 조합하는 것이 좋습니다. Analyzer에만 책임을 집중시키지 않고 설계, 정적 분석, 운영의 각 측면에서 같은 방침을 뒷받침하면 효과적입니다.

여기서 소개하는 using에 의한 Dispose와 Analyzer에 의한 검출은 CancellationTokenSource에 국한되지 않고 IDisposable을 구현하는 다른 리소스에도 응용할 수 있습니다.

이 절에서는 이러한 체계화 방법 중 C\# 코드를 자동으로 검사할 수 있는 Roslyn Analyzer를 사용한 방법을 소개합니다.

### **Roslyn Analyzer란**

Roslyn Analyzer는 C\# 코드를 컴파일 시점이나 IDE에서 정적으로 분석하여 문제 가능성이 있는 위치에 경고나 제안을 표시하는 구조입니다. 구문, 타입 정보, 처리 경로 등을 바탕으로 버그의 징후나 코딩 규약 위반을 검출할 수 있으며, Code Fix를 준비하면 수정 후보도 제시할 수 있습니다.

Analyzer가 무엇을 문제로 검출하는지는 진단 규칙으로 정의되며, 각 규칙에는 CA2000과 같은 식별용 진단 ID가 부여됩니다. CA2000은 .NET이 제공하는 기존 규칙이고, 뒤에서 설명하는 UTK001과 UTK002는 프로젝트 독자적으로 추가하는 규칙안입니다.

Unity 프로젝트에서는 Analyzer의 DLL을 Assets 아래에 배치하고 RoslynAnalyzer 레이블을 부여하여 도입합니다{{fn:roslyn_analyzers}}. asmdef를 사용하여 적용 대상 어셈블리를 한정할 수도 있습니다.

### **기존 Analyzer를 첫 번째 방어선으로 삼기**

먼저 검토할 것은 .NET 코드 품질 규칙에 포함된 CA2000입니다{{fn:ca2000}}. CA2000은 로컬에서 생성한 IDisposable 오브젝트가 Dispose되지 않은 채 스코프를 벗어나는 경로를 검출합니다. CancellationTokenSource도 IDisposable이므로 new CancellationTokenSource()의 Dispose 누락을 검출하는 첫 번째 방어선으로 활용할 수 있습니다.

다만 사용 중인 Analyzer의 버전이나 분석 설정에 따라, CreateLinkedTokenSource처럼 오브젝트를 생성해 반환하는 메서드의 반환값이나 메서드 간 소유권 이전을 어디까지 추적할 수 있는지는 다릅니다. 도입 시에는 new CancellationTokenSource()와 CreateLinkedTokenSource 모두에 대해 검출되는 예와 검출되지 않는 예를 작은 검증 프로젝트에서 확인하는 것이 좋습니다. 기존 규칙으로 충분히 검출하지 못하는 패턴은 필요에 따라 뒤에서 설명하는 독자 규칙으로 보완할 수 있습니다.

### **비동기 처리용 독자 규칙 설계 예**

기존 Analyzer로 CreateLinkedTokenSource의 반환값을 충분히 추적하지 못하는 경우, 프로젝트 고유의 Roslyn Analyzer로 보완하는 방법이 있습니다. 다음은 독자 Analyzer를 구현하는 경우의 규칙 설계 예입니다. 설명을 위해 가상의 진단 ID로 UTK001과 UTK002를 사용합니다.

UTK001에서는 new CancellationTokenSource() 또는 CancellationTokenSource.CreateLinkedTokenSource()로 생성된 로컬 변수가 using 문, using 선언 또는 finally 절의 Dispose로 보호되고 있는지 검사하는 것을 상정합니다. 모든 정상 종료 경로뿐 아니라 return이나 예외로 메서드를 빠져나오는 경로도 분석 대상으로 합니다.

CODE START csharp | UTK001이 검출하는 예 {{list:analyzer_detects_linked_source}}

private async UniTask LoadAsync(

	CancellationToken tokenA,

	CancellationToken tokenB)

{

	var linkedCts \=

        CancellationTokenSource.CreateLinkedTokenSource(

        	tokenA,

        	tokenB);

 

	await LoadAssetAsync(linkedCts.Token);

}

CODE END

{{list:analyzer_detects_linked_source}}에서는 linkedCts가 Dispose되지 않은 채 메서드를 빠져나옵니다. 이 규칙에서는 CreateLinkedTokenSource의 호출 위치에 경고를 표시하고, using 선언 또는 try-finally로 Dispose를 보장하도록 안내하는 것을 상정합니다.

UTK002에서는 CancellationToken.Register의 반환값인 CancellationTokenRegistration이 그대로 Dispose되지 않고 있지는 않은지 검사하는 것을 상정합니다. Register의 반환값을 받지 않고 버리는 코드는 등록 해제의 책임이 불분명합니다. 콜백을 비동기 처리 동안에만 유효하게 하려면 using 선언으로 관리하는 것이 좋습니다. 필드에 보관하는 경우에는 소유자의 종료 처리에서 Dispose합니다.

CODE START csharp | CancellationTokenRegistration을 관리하는 예 {{list:registration_using}}

private async UniTask WaitAsync(CancellationToken token)

{

	using var registration \= token.Register(OnCanceled);

 

	await WaitForSignalAsync(token);

}

CODE END

{{list:registration_using}}에서는 WaitForSignalAsync가 종료된 시점에 registration이 Dispose됩니다. 한편 애플리케이션 전체에서 공유하는 Token에 영구적으로 등록하는 등, 등록과 Token의 수명을 일치시키는 설계도 있습니다. 그러한 예까지 일괄적으로 경고하면 오탐이 늘어나므로, UTK002는 우선 정보 수준으로 시험하고 프로젝트에서 자주 나오는 안전한 패턴을 제외할 수 있도록 하는 것이 좋습니다.

필드에 보관된 CancellationTokenSource는 로컬 변수보다 소유권 판정이 어렵습니다. 2단계 규칙으로, 필드에 새로운 CancellationTokenSource를 대입하기 전에 이전 값을 Dispose하는지, MonoBehaviour의 OnDestroy 등 소유자의 종료 처리에서 Dispose하는지를 검사할 수 있습니다. 다만 다른 오브젝트로 소유권이 이전되는 경우가 있으므로 로컬 변수 규칙과는 분리하여 도입하는 것이 좋습니다.

### **Code Fix로 수정 방법 통일하기**

Roslyn Analyzer에는 검출한 문제를 IDE에서 자동으로 수정하는 Code Fix도 준비할 수 있습니다. 예를 들어 UTK001에서 소유권이 메서드 내에서 완결되는 단순한 경우에 var를 using var로 변환하는 Code Fix를 제공하면, 안전하게 기계적으로 변환할 수 있는 패턴의 수정 방법을 팀 내에서 통일하기 쉬워집니다. 필드 대입이나 반환값에 의한 소유권 이전처럼 Dispose 시점을 하나로 정할 수 없는 경우에는 자동 수정을 제시하지 않고, 설계 확인을 촉구하는 진단에 그치는 것이 좋습니다.

코드 리뷰에서는 Analyzer의 결과뿐 아니라 생성, Cancel, Dispose의 담당이 코드에서 읽히는지도 확인하는 것이 좋습니다. Analyzer에 경고가 없다는 것만으로는 안전성을 증명할 수 없다는 점에도 주의가 필요합니다.

### **Analyzer만으로는 검출할 수 없는 문제**

정적 분석에는 한계가 있습니다. CancellationTokenSource를 컬렉션에 저장한 경우나 다른 오브젝트에 전달하여 소유권을 넘긴 경우 등에는 최종적인 Dispose 위치를 정확히 판단하지 못할 수 있습니다. 오탐을 피하려고 분석을 약하게 하면 이번에는 검출 누락이 늘어납니다.

이 영역은 Development Build나 테스트 환경에서의 런타임 검증으로 보완하는 방법이 있습니다. 프로젝트 공통의 생성용 구조나 수명을 관리하는 타입을 거쳐 CancellationTokenSource를 생성하고 생성 수와 Dispose 수를 기록하면, 씬 전환이나 테스트 종료 시에 Dispose되지 않은 스코프가 남아 있지 않은지 확인할 수 있습니다. 생성 시점의 스택 트레이스도 기록하는 경우에는 부하를 고려하여 개발 환경에서만 활성화하는 것이 좋습니다.

필요에 따라 Profiler로 메모리 증가를 확인하는 조사, 런타임의 Dispose 누락 검출, Roslyn Analyzer에 의한 편집 시 경고를 조합하는 것이 좋습니다. 문제가 발생한 뒤에 담당자가 조사하는 흐름만이 아니라, 코드를 작성한 시점에 수정 방법까지 제시할 수 있는 흐름으로 만들면 팀 차원의 재발 방지로 이어집니다.

{{pagebreak}}

### **참고 자료**

Unity Manual 『Roslyn 애널라이저와 소스 제너레이터』

Microsoft Learn 『CA2000: 범위를 잃기 전에 개체 삭제』

Unity Technologies 『Mono CancellationTokenSource.cs(unity-2022.3-mbe 브랜치)』

Microsoft Learn『CancellationTokenSource.Dispose Method』

Microsoft Learn『CancellationTokenRegistration.Dispose Method』

Unity Manual 『관리되는 메모리』

脚注: {{fn:roslyn_analyzers}}

https://docs.unity3d.com/ja/2022.3/Manual/roslyn-analyzers.html (참조일: 2026년 9월 8일)

脚注: {{fn:ca2000}}

https://learn.microsoft.com/ja-jp/dotnet/fundamentals/code-analysis/quality-rules/ca2000 (참조일: 2026년 9월 8일)

脚注: {{fn:mono_cancellation_token_source}}

https://github.com/Unity-Technologies/mono/blob/unity-2022.3-mbe/mcs/class/referencesource/mscorlib/system/threading/CancellationTokenSource.cs (참조일: 2026년 9월 8일)

脚注: {{fn:cancellation_token_source_dispose}}

https://learn.microsoft.com/ja-jp/dotnet/api/system.threading.cancellationtokensource.dispose (참조일: 2026년 9월 8일)

脚注: {{fn:cancellation_token_registration_dispose}}

https://learn.microsoft.com/ja-jp/dotnet/api/system.threading.cancellationtokenregistration.dispose (참조일: 2026년 9월 8일)

脚注: {{fn:managed_memory}}

https://docs.unity3d.com/ja/2022.3/Manual/performance-managed-memory.html (참조일: 2026년 9월 8일)