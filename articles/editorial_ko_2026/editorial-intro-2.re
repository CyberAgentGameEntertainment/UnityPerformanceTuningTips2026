=[nonum] 2026년의 스크립팅·애니메이션 동향
#@# 原稿は sources/editorial-intro-2.md。scripts/prepare-review-source.py で
#@# 変換し、見出しに [nonum] を付けたもの。本文を直すときは .md 側から。

==[nonum] 스크립팅 동향

Unity 6 세대에서는 스크립팅에 관한 상황이 그다지 변하지 않았습니다. 다만 Unity 7에서는 @<b>{CoreCLR}와 @<b>{ECS for All}이라는, C# 코드의 퍼포먼스 튜닝에 관한 전제를 크게 바꾸는 기능의 추가가 예정되어 있으며, Unity 6 세대에서도 일부 그에 대비한 API 변경이 이루어지고 있습니다. 이 글에서는 Unity 6 세대의 API를 소개함과 함께 Unity 7에서 예정된 변경 사항도 일부 소개합니다. 또한 Unity 7에 관한 정보는 집필 시점의 것이며 실제 릴리스와 다를 수 있음을 양해해 주시기 바랍니다.

=== 비동기 처리

@<b>{Awaitable}은 Unity 2023에서 등장한 Unity 표준 클래스로, 기존에는 @<b>{UniTask} 등의 외부 라이브러리로 수행하던 효율적인 async / await 처리를 Unity 표준 클래스만으로 작성할 수 있게 되었습니다. 다만 Awaitable은 UniTask에 비해 기능 세트가 적으므로, 이미 UniTask를 사용하고 있다면 그대로 UniTask를 계속 사용해도 좋습니다. 외부 패키지에 의존할 수 없는 상황에서는 Task나 ValueTask, 코루틴 대신 Awaitable을 사용함으로써 성능 향상을 기대할 수 있습니다. 또한 제2부 5장에서는 비동기 처리에서 문제가 되기 쉬운 CancellationToken의 관리를 해설합니다.

=== .NET 런타임

Unity 6 시점의 .NET 런타임은 지금까지와 같이 Mono와 IL2CPP가 존재하며, .NET Standard 2.1의 API 세트와 C# 9를 사용할 수 있습니다. Unity 7에서는 Mono가 폐지되고, 에디터와 일부 데스크톱 플레이어에 @<b>{CoreCLR}가 도입될 예정입니다. CoreCLR는 .NET 본가(.NET Core / .NET)에서 사용되는 런타임 구현으로, 퍼포먼스에서 Mono를 앞섭니다. 모바일이나 콘솔 플랫폼에는 당분간 CoreCLR가 도입되지 않아 계속 IL2CPP가 일급 선택지가 되지만, 에디터가 CoreCLR로 이행하는 것 외에 실행 시 사용할 수 있는 표준 API의 추가나 구현의 최적화 등 일부에서 이점이 있습니다.

==== CoreCLR의 JIT

CoreCLR에서는 C# 코드를 네이티브 코드로 변환하는 JIT에 다수의 최적화가 더해졌습니다. 대부분의 경우 Mono에서 CoreCLR로 이행하는 것만으로 C# 코드의 실행 속도가 향상될 것입니다. Unity가 공개한 벤치마크 결과에 따르면 GC를 사용하지 않는 워크로드에서 CoreCLR는 Mono와 비교해 실행 속도가 2~3배 상승하고, IL2CPP나 Burst에 필적하는 성능을 얻을 수 있음이 나타나 있습니다@<fn>{fn1}. 지금까지는 특히 Mono와 IL2CPP 중 하나를 선택할 수 있는 데스크톱 플랫폼에서 퍼포먼스 관점에서 IL2CPP가 최적의 선택지였지만, CoreCLR 등장 이후에는 이 전제도 바뀔지 모릅니다.

==== .NET 10의 BCL

지금까지 .NET BCL@<fn>{fn2}은 Mono에 포함된 구현을 사용했고 .NET Standard 2.1에 상당하는 API 세트로 제한되었지만, CoreCLR 도입에 따라 .NET 10에 상당하는 BCL로 업데이트될 전망입니다. 사용할 수 있는 API가 크게 늘어나는 것 외에 내부 구현이 더 정교해져 성능 개선을 기대할 수 있습니다. 새로운 BCL은 IL2CPP에서도 사용할 수 있게 될 예정이며, CoreCLR가 도입되지 않는 모바일이나 콘솔 플랫폼에서도 성능과 기능 면에서 이점이 있습니다.

==== CoreCLR의 GC / 메모리 관리

CoreCLR는 가비지 컬렉터(GC)의 구현도 Mono / IL2CPP와 다릅니다. 지금까지 Unity의 Mono와 IL2CPP에서는 @<b>{Boehm GC}(Boehm-Demers-Weiser GC, bdwgc)라 불리는 GC 구현을 사용했지만, CoreCLR의 GC는 Boehm GC와 크게 다른 특징을 가집니다@<fn>{fn3}.

먼저 CoreCLR GC는 @<b>{세대별 GC(generational GC)}입니다. 세대별 GC는 할당된 오브젝트를 「세대」라 불리는 그룹별로 관리합니다. 새로 할당된 오브젝트는 우선 gen0으로 취급됩니다. gen0에 대한 GC는 비교적 자주 수행되지만, gen0 GC에서 살아남은 오브젝트는 다음 세대인 gen1로 이동하여 gen0보다 낮은 빈도로 GC가 실행됩니다. 이는 「새로 할당된 오브젝트의 대부분은 수명이 짧고, 어느 정도 살아남은 오브젝트는 이후에도 오래 살아남는 경우가 많다」는 일반적인 관찰에 기반한 것으로, 어린 세대에 대해 자주 실행되는 GC에 걸리는 시간을 단축하는 것이 목적입니다. CoreCLR GC에서의 메모리 최적화에서는 할당된 오브젝트가 어느 세대에 속해 있는지, 어느 세대에 대한 GC가 언제 실행되는지 등이 중요한 포인트가 될 것입니다.

또한 CoreCLR GC는 @<b>{컴팩션}을 수행합니다. Boehm GC는 한 번 할당된 오브젝트를 이동시키지 않기 때문에, 오브젝트의 할당과 해제가 반복되면 점차 힙이 단편화되어 실제로 필요한 크기보다 힙이 커지기 쉽다는 문제가 있었습니다. CoreCLR GC의 컴팩션에서는 단편화된 힙 위에서 오브젝트를 이동시켜 오브젝트 사이의 틈을 메웁니다. 이를 통해 힙의 확대가 억제됩니다.

한편 IL2CPP에서는 계속 Boehm GC를 사용하므로 지금까지와 같은 메모리 최적화 전략이 유효합니다@<fn>{fn4}.

GC 외에도 CoreCLR에서는 JIT에 의한 메모리 할당 최적화도 적극적으로 이루어지고 있습니다. 예를 들어 @<b>{Object Stack Allocation}은 메서드 내에서 사용이 완결되는 매우 수명이 짧은 오브젝트를 힙이 아니라 스택에 할당하는 JIT 기능입니다. 특히 루프 내부 등 핫 패스에서 대량으로 할당되는 수명이 짧은 오브젝트 등은 Mono나 IL2CPP만큼 문제를 일으키지 않게 될지도 모릅니다.

==== C# 14 지원

Unity 6 시점에서 공식적으로 지원되는 것은 C# 9이지만, Unity 7에서는 C# 14를 사용할 수 있게 될 예정입니다. 최근 C# 릴리스에서는 퍼포먼스 개선을 위한 새로운 기능도 다수 추가되었으며, 퍼포먼스 튜닝 관점에서 알아 두어야 할 언어 기능도 늘어나게 됩니다.

==== Unity 6 세대에 선행 도입된 기능

CoreCLR의 정식 도입은 Unity 7에서 이루어질 예정이지만, 일부 플랫폼에서는 Unity 6 계열에서 실험적인 CoreCLR 도입이 시작되었습니다. 집필 시점 최신인 Unity 6.7 alpha(6000.7.0a6)에서는 Windows, macOS, Linux의 스탠드얼론 플레이어에서 CoreCLR를 사용할 수 있습니다. 다만 API 세트는 아직 .NET Standard 2.1로 제한됩니다. 마찬가지로 Unity 6.7 alpha의 iOS 플레이어(IL2CPP)에서는 .NET 10에 상당하는 BCL 구현을 사용할 수 있게 되었지만, 이쪽도 .NET Standard 2.1로 제한됩니다. 현 단계에서는 Unity 7에서의 CoreCLR 이행에 대비한 퍼포먼스와 호환성 검증에만 사용할 수 있다고 생각해야 할 것입니다.

=== DOTS

Unity 6에서는 DOTS(Data-Oriented Technology Stack) 기능군이 계속 유효합니다. LLVM을 활용하여 C# 코드를 고도로 최적화된 네이티브 코드로 변환하는 @<b>{Burst}, 코드를 안전하게 멀티스레드화하는 @<b>{C# Job System}, 그리고 이를 활용하여 MonoBehaviour보다 빠르게 게임 상태를 갱신하는 구조인 @<b>{Entity Component System(ECS)}을 사용할 수 있습니다.

Unity 7에서는 @<b>{ECS for All}이라는 기능군을 통해 ECS와 GameObject의 연계가 강화될 전망입니다. 지금까지 ECS에서 컴포넌트를 저장하기 위한 컨테이너인 Entity는 GameObject와 호환되지 않았고, ECS의 상태는 GameObject와 완전히 독립적으로 관리되었습니다. Unity 7에서는 GameObject가 Entity로도 기능하게 되어, GameObject에 ECS 컴포넌트를 어태치할 수 있게 됩니다. 지금까지보다 ECS와 GameObject를 혼용·연계하기 쉬워짐으로써 병목이 되기 쉬운 MonoBehaviour의 Update 처리 군을 ECS로 옮기기 쉬워지고, 메인 스레드 부하 절감과 프레임 레이트 향상에 도움이 될 것입니다.

#@# この節の最後の1行だけが次のページにあふれるため、このページを1行延ばす。
//embed[latex]{
\enlargethispage{\baselineskip}
//}

==[nonum] 애니메이션 동향

Unity 6 세대에서는 애니메이션의 퍼포먼스에 몇 가지 개선이 더해졌습니다. Unity 6.3에서 Animator와 AnimationPlayableGraph에 @<b>{ResetControllerState} 메서드가 추가되어 Animator를 쉽게 풀링할 수 있게 되었습니다. 자세한 내용은 제2부 2장에서 해설합니다. Unity 6.5에서는 @<b>{AnimationEventInfo} 구조체가 추가되어 Animation Event를 받을 때 AnimationEvent 클래스로 인한 힙 메모리 할당을 피할 수 있게 되었습니다.

//footnote[fn1][Path to CoreCLR | GDC 2026 - Unity - YouTube @<href>{https://youtu.be/_t6xVfrmEWU?si=edILni6YnObyWkoj&t=218}]
//footnote[fn2][Base Class Library. System 네임스페이스 등 표준 클래스 라이브러리 구현을 가리킵니다]
//footnote[fn3][Fundamentals of garbage collection - .NET | Microsoft Learn - @<href>{https://learn.microsoft.com/en-us/dotnet/standard/garbage-collection/fundamentals}]
//footnote[fn4][Understanding Memory Management in Unity - Unity Discussion - @<href>{https://discussions.unity.com/t/understanding-memory-management-in-unity/1700107}]
