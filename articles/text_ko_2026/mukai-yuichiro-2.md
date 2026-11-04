# **Unity Managed Code Stripping 실전 가이드**

Mukai Yuichiro / 소속: 주식회사 Applibot  
yucchiy\_  / blog.yucchiy.com / yucchiy  
\---

앱 크기를 줄이는 수단으로, Unity가 제공하는 Managed Code Stripping(빌드 시 사용하지 않는 코드를 제거하는 기능)을 소개합니다.  
설정 하나로 활성화할 수 있는 간편한 기능이지만, 절감 수준을 높이면 '에디터에서는 동작하지만 빌드한 앱에서만 특정 기능이 깨지는' 문제가 발생할 수 있습니다.  
이 장에서는 Managed Code Stripping의 구조와 사용법, 코드가 필요 이상으로 제거되는 전형적인 패턴과 그 대처 방법 등을 소개합니다.

검증 환경은 다음과 같습니다.

*  Unity 6000.3.19f1  
* Addressables 4.0.1  
* macOS Sequoia 15.7.3

결론부터 말하면, Managed Code Stripping은 설정을 High로 바꾸는 것으로 끝낼 수 있는 최적화가 아닙니다.

정적 분석에서 보이지 않는 의존성을 명시하고, 빌드한 앱의 크기와 동작을 계속 검증하는 운용이 함께 이루어질 때 비로소 안전하게 사용할 수 있는 기능입니다.

# **Managed Code Stripping이란 무엇인가**

## **기능 개요**

Managed Code Stripping은 빌드 시 사용하지 않는 매니지드 코드(C\# 코드)를 검출하여 빌드한 앱에서 제거하는 Unity 표준 기능입니다.

씬이나 스크립트에서 사용되지 않는 클래스와 메서드가 제거되므로 앱의 코드 크기를 줄일 수 있습니다.

대상은 사용자 코드뿐만 아니라 UnityEngine 모듈, .NET 클래스 라이브러리, 프로젝트에 포함된 패키지와 플러그인의 어셈블리까지 포함하므로, 직접 수정하기 어려운 라이브러리 내부의 사용하지 않는 코드까지 절감할 수 있습니다.

특히 IL2CPP 빌드에서는 매니지드 코드가 줄어들면 이후 IL2CPP가 생성하는 C++ 코드도 줄어들어, 앱 크기와 빌드 시간 단축으로도 이어집니다.

참고로 이름이 비슷한 설정으로 Strip Engine Code가 있지만, 이는 Unity 엔진의 네이티브 코드를 대상으로 한 절감 설정이며 매니지드 코드를 대상으로 하는 Managed Code Stripping과는 별개의 기능입니다.

### **\#\#\# Managed Stripping Level**

코드를 얼마나 적극적으로 제거할지는 Managed Stripping Level 설정으로 제어합니다.

Edit \> Project Settings \> Player \> Other Settings \> Optimization의 'Managed Stripping Level'에서 설정할 수 있으며, 다음 레벨을 선택할 수 있습니다.

表: Managed Stripping Level의 선택지 {{table:stripping_levels}}

| Level | 개요 |
| :---- | :---- |
| Disabled | 스트리핑을 수행하지 않습니다. Mono 빌드에서만 선택할 수 있습니다 |
| Minimal | UnityEngine과 .NET 클래스 라이브러리만 대상으로 절감하며, 사용자 코드는 제거하지 않습니다. IL2CPP 빌드의 기본값입니다 |
| Low | 사용자 코드도 분석 대상에 포함하지만, 보수적으로 절감합니다 |
| Medium | 모든 어셈블리를 대상으로 크기와 안전성의 균형을 맞추어 절감합니다 |
| High | 모든 어셈블리를 가장 적극적으로 절감합니다 |

Mono 빌드에서는 Disabled가 기본값이므로, 아무것도 설정하지 않으면 스트리핑은 수행되지 않습니다.

반면 IL2CPP 빌드에서는 Minimal이 기본값이고 Disabled는 선택할 수 없으므로, 정도의 차이는 있어도 반드시 스트리핑이 수행됩니다.

참고로 Low는 Unity 6.3 문서에서 향후 폐지가 예고되어 있어 사용이 권장되지 않습니다.

참고: \[Managed code stripping\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping.html) / \[Configure managed code stripping\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-configure.html)

## **기본 동작 원리**

Managed Code Stripping은 \*\*UnityLinker\*\*라는 Unity 자체 개발 도구로 구현됩니다.

UnityLinker는 앱 빌드 파이프라인에서 C\# 코드의 컴파일이 완료된 후에 실행되며, 컴파일된 어셈블리(DLL)에서 사용하지 않는 코드를 제거합니다.

IL2CPP 빌드에서는 이 스트리핑을 거친 어셈블리가 이후 C++ 코드 변환의 입력이 됩니다.

UnityLinker는 다음 세 단계로 사용하지 않는 코드를 판정합니다.

1. 분석의 시작점이 되는 코드(\*\*root\*\*)를 결정한다  
2. root에서 정적으로 참조되는 타입과 멤버를 재귀적으로 따라가며 사용 중 표시를 붙인다(\*\*mark\*\*)  
3. 끝까지 표시가 붙지 않은 코드를 도달 불가로 간주하여 어셈블리에서 제거한다

root로 취급되는 대표적인 대상은 다음과 같습니다.

* 빌드에 포함된 씬이나 에셋에서 참조되는 타입  
* MonoBehaviour나 ScriptableObject를 상속한 타입  
* \[RuntimeInitializeOnLoadMethod\] 등의 속성이 붙은 메서드  
* link.xml이나 \[Preserve\] 속성으로 명시적으로 보존을 지정한 코드(뒤에서 다루는 대처 방법에서 설명합니다)

여기서 중요한 점은 코드를 남길지 여부가 '런타임에 실제로 사용되는가'가 아니라 '빌드 시 정적 분석에서 root로부터 도달할 수 있는가'로 판단된다는 것입니다.

정적 분석이란 코드를 실행하지 않고, 컴파일된 코드에 나타나는 참조만을 기계적으로 따라가는 분석을 말합니다.

예를 들어 new를 통한 타입 생성이나 메서드의 직접 호출은 컴파일된 코드에 참조 대상 타입과 멤버의 정보가 그대로 남으므로 UnityLinker가 따라갈 수 있습니다.

반면 코드상에 참조가 나타나지 않는 형태로 호출되는 코드는 런타임에 사용되는지와 관계없이 도달 불가로 간주됩니다.

따라서 정적 분석에서 보이지 않는 참조만 가진 코드는 런타임에 필요하더라도 사용하지 않는 것으로 판정되어 제거됩니다.

또한 mark는 타입 단위가 아니라 멤버 단위로 수행되므로, 남은 타입에서도 사용되지 않는 메서드는 개별적으로 제거됩니다.

실제로 어떤 코드가 제거되는지는 다음 절에서 구체적인 코드와 계측 결과와 함께 확인합니다.

### **무엇이 제거되었는지 Mono.Cecil로 확인하기**

이 장에서는 이후 절에서 '어떤 타입과 메서드가 빌드에 남았는가'를 반복해서 확인합니다.

이 확인에는 .NET 어셈블리를 분석하는 라이브러리인 \*\*Mono.Cecil\*\*을 사용합니다.

빌드 출력에 포함된 어셈블리(macOS용 Mono 빌드라면 '앱 이름.app/Contents/Resources/Data/Managed/Assembly-CSharp.dll')를 읽어 타입과 메서드 목록을 파일로 출력합니다.

CODE START csharp | 타입과 메서드 목록 출력 {{list:cecil_dump}}  
// 어셈블리를 읽어 타입과 메서드(.ctor 포함)의 목록을 출력한다  
using var assembly \= AssemblyDefinition.ReadAssembly(dllPath);  
var lines \= assembly.MainModule.Types  
    .Where(type \=\> type.Name \!= "\<Module\>")  
    .SelectMany(type \=\> new\[\] { type.FullName }  
        .Concat(type.Methods.Select(method \=\> $"  {method.Name}")));  
File.WriteAllLines(outputPath, lines);  
CODE END

메서드 목록에는 생성자(.ctor)도 포함되므로, 뒤에서 다룰 '타입은 남아 있는데 생성자만 사라지는' 증상을 확인하는 데도 사용할 수 있습니다.

Mono.Cecil은 Unity Package Manager를 통해 설치하여 사용할 수 있습니다. 에디터용 Assembly Definition에서 overrideReferences를 활성화하고 precompiledReferences에 Mono.Cecil.dll을 추가하면 에디터 코드에서 참조할 수 있습니다.

Level을 바꾼 두 빌드에서 이 목록을 추출해 차이를 비교하면, 어떤 타입이 사라졌고 어떤 메서드가 멤버 단위로 제거되었는지 특정할 수 있습니다.

빌드 후에는 스트리핑된 어셈블리가 중간 생성물로 Library 아래에 남는 경우가 있습니다.

이번 환경에서는 Library/Bee/artifacts/MacStandalonePlayerBuildProgram/ManagedStripped/에 빌드 출력과 동일한 내용의 DLL이 남아 있어, 빌드 결과물을 풀지 않아도 이쪽을 분석 대상으로 삼을 수 있습니다.

이 경로에 스트리핑 후 어셈블리가 저장된다는 점은 공식 문서에 명시되어 있지 않으므로, 찾을 수 없는 경우에는 빌드 출력 쪽을 분석합니다.

이후 절에서 '타입과 메서드를 확인했다'고 적은 부분은 기본적으로 이 방법으로 확인한 것입니다. 스트리핑 전후의 DLL을 비교하면 절감 내역까지 확인할 수 있습니다.

참고: \[Unity linker marking rules\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-marking-rules.html)

## **왜 코드를 제거하고 싶은가**

Managed Code Stripping을 사용하는 주된 목적은 앱 크기 절감입니다.

부차적으로는 사용하지 않는 코드가 메모리에 올라가지 않으므로 런타임 메모리 사용량 절감도 기대할 수 있습니다.

효과가 어느 정도인지 확인하기 위해, 종료 처리 스크립트 하나만 둔 단순한 씬을 빌드하여 Managed Stripping Level의 Minimal과 High를 비교합니다.

계측 결과는 다음과 같습니다({{table:size_comparison}}).

表: 단순한 씬에서의 Minimal과 High 비교 {{table:size_comparison}}

| 지표 | Minimal | High | 절감 |
| :---- | :---- | :---- | :---- |
| 매니지드 어셈블리 합계 크기 | 7,702,016 bytes | 7,188,480 bytes | 약 6.7% |
| 어셈블리(DLL) 수 | 52 | 51 | 1개 |
| 앱 전체 크기 | 89,069,307 bytes | 88,548,299 bytes | 약 0.6% |

작은 샘플 프로젝트에서도 매니지드 코드가 6.7% 줄었습니다.

내역을 보면 어디에서도 사용되지 않는 System.Runtime.Serialization.dll이 통째로 제거되었고, mscorlib.dll이 약 272KB 줄었으며, 씬에서 참조되지 않는 코드를 많이 포함한 Assembly-CSharp.dll도 22.5% 줄었습니다.

매니지드 코드의 절감량은 프로젝트가 참조하는 라이브러리의 양과, 그중 실제로 사용되지 않는 코드의 비율로 결정됩니다.

대규모 프로젝트에서는 많은 라이브러리를 도입하는 경향이 있고, 라이브러리 내에서도 실제로 사용되지 않는 기능이 많은 경우가 흔하므로 코드 절감의 이점을 얻기 쉬울 것입니다.

IL2CPP 빌드에서는 절감된 매니지드 코드만큼 C++ 변환과 컴파일 대상도 줄어들므로, 네이티브 바이너리의 크기와 빌드 시간에도 효과가 미칩니다.

# **Managed Code Stripping 적용 시 주의점**

앞 절에서 본 것처럼 Managed Code Stripping의 판단은 빌드 시 정적 분석에 의존합니다.

따라서 정적 분석에서 보이지 않는 참조만 가진 코드는 런타임에 필요하더라도 제거됩니다.

이 문제가 까다로운 이유는 증상이 빌드한 앱에서만 나타난다는 점입니다.

에디터에서의 실행이나 스트리핑이 비활성화된 기본 Mono 빌드에서는 정상적으로 동작하므로 개발 중에는 알아차리기 어렵고, Stripping Level을 올린 순간 특정 기능만 깨지는 형태로 문제가 드러납니다.

이 절에서는 코드가 필요 이상으로 제거되는 대표적인 두 가지 패턴을 다룹니다.

하나는 리플렉션을 통한 코드 호출이고, 다른 하나는 Unity의 엔트리 포인트에서 참조되지 않는 코드입니다.

## **리플렉션으로 호출하는 코드가 제거된다**

앞 절에서 설명한 '정적 분석에서 root로부터 도달할 수 있는가'라는 판단을 구체적인 코드로 확인합니다.

참조되는 방식이 다른 세 클래스를 준비합니다({{list:probe_targets}}).

CODE START csharp | 참조되는 방식이 다른 세 클래스 {{list:probe_targets}}  
// new로 직접 생성된다  
public class DirectlyUsedService  
{  
    public string Describe() \=\> "DirectlyUsedService is alive";  
}

// typeof로 타입만 참조된다  
public class TypeofOnlyService  
{  
    public string Describe() \=\> "TypeofOnlyService is alive";  
}

// 리플렉션을 통해서만 생성된다  
public class ReflectionOnlyService  
{  
    public string Describe() \=\> "ReflectionOnlyService is alive";  
}  
CODE END

씬에 배치한 MonoBehaviour에서 이 세 클래스를 다음과 같이 사용합니다({{list:stripping_probe}}).

CODE START csharp | 세 클래스를 사용하는 MonoBehaviour {{list:stripping_probe}}  
public class StrippingProbe : MonoBehaviour  
{  
    private static readonly string ReflectionTypeName \= "ReflectionOnlyService";  
    private static readonly string TypeofTypeName \= "TypeofOnlyService";  
    private static readonly string TypeofMethodName \= "Describe";

    private void Start()  
    {  
        // 정적 참조: 참조 대상 타입과 메서드의 정보가 컴파일된 코드에 남는다  
        var direct \= new DirectlyUsedService();  
        Debug.Log($"direct: {direct.Describe()}");

        // 정적 참조가 타입뿐: typeof는 타입을 참조하지만 멤버에 대한 정적 참조는 없다  
        Debug.Log($"typeof: type is referenced: {typeof(TypeofOnlyService).Name}");

        // 멤버가 남았는지는 정적 분석에서 보이지 않는 경로로 타입을 다시 가져와 확인한다  
        var typeofType \= Type.GetType(TypeofTypeName);  
        var describeMethod \= typeofType?.GetMethod(TypeofMethodName);  
        if (describeMethod \== null)  
        {  
            Debug.Log($"typeof: type {typeofType?.Name} exists but Describe is stripped");  
        }  
        else  
        {  
            Debug.Log($"typeof: {describeMethod.Invoke(Activator.CreateInstance(typeofType), null)}");  
        }

        // 정적 분석에서 보이지 않는 참조: 리플렉션을 통해 생성한다  
        var type \= Type.GetType(ReflectionTypeName);  
        if (type \== null)  
        {  
            Debug.Log($"reflection: Type.GetType(\\"{ReflectionTypeName}\\") returned null");  
        }  
        else  
        {  
            var instance \= Activator.CreateInstance(type);  
            Debug.Log($"reflection: {type.GetMethod("Describe")?.Invoke(instance, null)}");  
        }  
    }  
}  
CODE END

이 샘플을 Managed Stripping Level을 Minimal과 High로 바꿔 빌드하여 실행하면 결과는 다음과 같습니다.

CODE START text  
\# Minimal  
direct: DirectlyUsedService is alive  
typeof: type is referenced: TypeofOnlyService  
typeof: TypeofOnlyService is alive  
reflection: ReflectionOnlyService is alive

\# High  
direct: DirectlyUsedService is alive  
typeof: type is referenced: TypeofOnlyService  
typeof: type TypeofOnlyService exists but Describe is stripped  
reflection: Type.GetType("ReflectionOnlyService") returned null  
CODE END

High에서는 typeof 케이스는 타입은 얻을 수 있지만 GetMethod가 null을 반환하고, 리플렉션 케이스는 Type.GetType 자체가 null을 반환합니다.

정리하면 다음과 같은 동작입니다({{table:probe_results}}).

表: Minimal과 High에서 남는 방식 {{table:probe_results}}

| 대상 | Minimal | High |
| :---- | :---- | :---- |
| StrippingProbe | 남음 | 남음 |
| DirectlyUsedService(new로 참조) | 남음 | 타입도 메서드도 남음 |
| TypeofOnlyService(typeof로 타입만 참조) | 남음 | 타입은 남지만 Describe는 제거됨 |
| ReflectionOnlyService(리플렉션만 사용) | 남음 | 타입째 제거됨 |

Minimal은 사용자 코드를 제거하지 않으므로 세 가지 모두 그대로 남습니다.

High에서는 참조되는 방식의 차이가 그대로 결과에 나타납니다.

DirectlyUsedService는 new로 타입을 참조하고 Describe도 직접 호출하므로 타입도 메서드도 남습니다.

typeof로 참조되는 TypeofOnlyService는 타입으로서는 남지만, 어디에서도 호출되지 않는 Describe는 멤버 단위로 제거되었습니다.

그리고 정적 참조가 전혀 없는 ReflectionOnlyService는 타입째 제거되었습니다.

참고로 이 실험에서 타입 이름과 메서드 이름 문자열을 모두 static readonly 필드를 통해 읽는 것은 의도적인 구성입니다.

UnityLinker는 리플렉션의 전형적인 사용 방식 일부를 이해하고 있어, Type.GetType(\"ReflectionOnlyService\")처럼 상수 문자열을 직접 전달한 경우에는 High에서도 타입이 보존되는 것을 확인했습니다.

또한 typeof로 얻은 Type에 대해 직접 GetMethod를 호출한 경우에는 메서드 이름이 상수가 아니어도 그 타입의 public 메서드가 일괄적으로 보존되었습니다.

반면 필드를 통해 전달한 문자열까지는 UnityLinker가 추적하지 않습니다.

타입 이름이나 멤버 이름이 설정 파일, 직렬화된 데이터, 서버 응답처럼 빌드 시 확정되지 않는 경로에서 오는 실제 상황을 필드 읽기로 재현하고 있습니다.

필드를 const로 만들면 사용 위치에 상수 문자열로 인라인되므로, 상수를 직접 전달한 경우와 동일하게 취급됩니다.

### DI 컨테이너에서 타입 해결이 깨진다

지금까지의 예시는 설명을 위해 단순화했지만, 같은 구조의 문제는 실제 라이브러리에서 발생합니다. 대표적인 것이 DI 컨테이너입니다.

많은 DI 컨테이너는 인터페이스와 구현 타입의 대응을 등록해 두고, 해결 시 리플렉션으로 생성자를 호출하여 인스턴스를 생성합니다.

이 동작을 모방한 최소한의 컨테이너로 확인합니다. 실제 DI 컨테이너와 마찬가지로 Register로 등록하고 Resolve로 해결하는 인터페이스로 구성합니다({{list:mini_container}}).

CODE START csharp | 최소한의 DI 컨테이너 {{list:mini_container}}  
public sealed class MiniContainer  
{  
    private readonly Dictionary\<Type, Type\> \_registrations \= new Dictionary\<Type, Type\>();

    // 구현 타입을 타입 인자로 등록한다. TImplementation은 typeof에 상응하는 정적 참조가 된다  
    public void Register\<TInterface, TImplementation\>() where TImplementation : TInterface  
    {  
        \_registrations\[typeof(TInterface)\] \= typeof(TImplementation);  
    }

    // 등록된 구현 타입을 리플렉션으로 인스턴스화하여 반환한다  
    public TInterface Resolve\<TInterface\>()  
    {  
        var implementationType \= \_registrations\[typeof(TInterface)\];  
        return (TInterface)Activator.CreateInstance(implementationType);  
    }  
}  
CODE END

등록하는 구현 타입은 컨테이너를 통해서만 사용된다고 가정하며, 어디에서도 정적으로 참조하지 않습니다({{list:notification_service}}).

CODE START csharp | 등록하는 인터페이스와 구현 {{list:notification_service}}  
public interface INotificationService  
{  
    string Notify();  
}

// 컨테이너를 통해서만 사용되며 어디에서도 정적으로 참조되지 않는 구현  
public sealed class NotificationService : INotificationService  
{  
    public string Notify() \=\> "NotificationService.Notify succeeded";  
}  
CODE END

씬에 배치한 MonoBehaviour에서 등록과 해결을 수행합니다({{list:di_probe}}).

CODE START csharp | 등록과 해결을 수행하는 MonoBehaviour {{list:di_probe}}  
public sealed class DiContainerProbe : MonoBehaviour  
{  
    private void Start()  
    {  
        try  
        {  
            var container \= new MiniContainer();  
            container.Register\<INotificationService, NotificationService\>();  
            Debug.Log($"\[DiContainerProbe\] explicit: {container.Resolve\<INotificationService\>().Notify()}");  
        }  
        catch (Exception exception)  
        {  
            Debug.Log($"\[DiContainerProbe\] explicit failed: {exception.GetType().Name}: {exception.Message}");  
        }  
    }  
}  
CODE END

이를 앞서와 같은 조건으로 빌드하면 결과는 다음과 같습니다.

CODE START text  
\# Minimal  
\[DiContainerProbe\] explicit: NotificationService.Notify succeeded

\# High  
\[DiContainerProbe\] explicit failed: MissingMethodException: Default constructor not found for type NotificationService  
CODE END

High에서는 Register\<INotificationService, NotificationService\>()의 타입 인수가 정적 참조가 되므로 NotificationService 타입 자체는 남습니다.

또한 인터페이스를 통해 정적으로 호출되는 Notify도 남습니다.

그러나 리플렉션으로만 호출되는 기본 생성자는 정적 참조를 갖지 않으므로 멤버 단위로 제거됩니다.

그 결과 Activator.CreateInstance가 생성자를 찾지 못하고, MissingMethodException으로 해결에 실패합니다.

타입을 등록했는데도 빌드한 앱에서만 해결에 실패하는 문제의 원인입니다.

실제 DI 컨테이너에서도 구조는 같습니다.

생성자 해결이 리플렉션에 의존하는 이상, 명시적으로 등록한 타입이라도 암시적 기본 생성자는 제거 후보가 됩니다.

어셈블리에서 구현 타입을 스캔하여 자동 등록하는 방식이라면, 정적 참조를 갖지 않는 구현 타입이 타입째 제거되어 등록 자체가 실패합니다.

## **Unity 엔트리 포인트에서 참조되지 않는 코드가 제거된다**

또 다른 패턴은 씬에서도 스크립트에서도 참조되지 않고, 에셋 번들 같은 외부 콘텐츠를 통해서만 필요해지는 코드입니다.

앞 절에서 본 것처럼 root의 공급원은 빌드에 포함된 씬과 에셋입니다.

에셋 번들로 별도 빌드되는 콘텐츠는 앱 본체 빌드에 포함되지 않으므로, 그 콘텐츠가 참조하는 타입은 root로 취급되지 않습니다.

따라서 콘텐츠 쪽에서만 필요한 타입은 앱 코드에 정적 참조가 없으면 런타임에 로드되는지와 관계없이 사용하지 않는 것으로 판정됩니다.

이 패턴을 Timeline 에셋으로 재현합니다.

Timeline 에셋을 에셋 번들에만 포함하고, 앱 쪽은 Unity.Timeline을 정적으로 참조하지 않은 채 에셋 이름으로 로드하여 PlayableDirector로 재생합니다({{list:timeline_probe}}).

CODE START csharp | 에셋 번들에서 Timeline 에셋을 재생하는 MonoBehaviour {{list:timeline_probe}}  
public class TimelineAssetBundleProbe : MonoBehaviour  
{  
    private const string BundleDirectoryName \= "TimelineBundles";  
    private const string BundleName \= "timeline.bundle";  
    private const string TimelineAssetName \= "TimelineOnly";

    private void Start()  
    {  
        var bundlePath \= Path.Combine(Application.streamingAssetsPath, BundleDirectoryName, BundleName);  
        var bundle \= AssetBundle.LoadFromFile(bundlePath);  
        if (bundle \== null)  
        {  
            Debug.Log("timeline bundle: failed to load asset bundle");  
            return;  
        }

        var asset \= bundle.LoadAsset(TimelineAssetName);  
        if (asset \== null)  
        {  
            Debug.Log($"timeline bundle: asset {TimelineAssetName} failed to load");  
            bundle.Unload(true);  
            return;  
        }

        Debug.Log($"timeline bundle: loaded {asset.GetType().AssemblyQualifiedName}");  
        var playableAsset \= asset as PlayableAsset;  
        if (playableAsset \== null)  
        {  
            Debug.Log($"timeline bundle: asset {TimelineAssetName} is not playable");  
            bundle.Unload(true);  
            return;  
        }

        var director \= gameObject.AddComponent\<PlayableDirector\>();  
        director.playableAsset \= playableAsset;  
        director.Play();  
        Debug.Log($"timeline bundle: play state={director.state}, graph valid={director.playableGraph.IsValid()}");  
        bundle.Unload(false);  
    }  
}  
CODE END

에셋 번들을 만든 뒤 원본 Timeline 에셋을 프로젝트에서 삭제하고 앱을 빌드함으로써, 앱과 별도로 콘텐츠를 업데이트하는 방식을 모방합니다.

Managed Stripping Level을 Minimal과 High로 바꿔 빌드합니다. 어느 레벨에서도 결과는 같습니다.

CODE START text  
\# Minimal / High 모두  
The referenced script (UnityEngine.Timeline.TimelineAsset) on this Behaviour is missing\!  
timeline bundle: asset TimelineOnly failed to load  
CODE END

또한 빌드한 앱의 Managed 폴더를 확인하면 어느 빌드에서도 Unity.Timeline.dll 자체가 존재하지 않았습니다.

주의할 점은 Minimal과 High에서 결과가 달라지지 않는다는 것입니다.

이 패턴에 대한 대처는 Stripping Level 조정이 아니라 필요한 타입이나 어셈블리의 보존을 명시하는 것입니다.

그 수단은 다음 절에서 다루고, Addressables를 사용하는 경우의 구조는 응용편에서 다룹니다.

참고: \[How code stripping affects content\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-content.html) / \[Timeline 에셋이 사라지는 사례(Qiita, 2021)\](https://qiita.com/KyoheiOkawa/items/0cf9cc09679eababec75)

# **코드를 과도하게 제거하는 문제의 대처 방법**

## **기본적인 대처 방법**

필요한 코드가 제거되는 것을 막는 가장 기본적인 수단은 UnityLinker에 '이 코드는 사용된다'고 명시적으로 알리는 것입니다.

Unity는 이를 위한 수단으로 link.xml 파일과 \[Preserve\] 속성을 제공합니다.

어느 쪽으로 지정한 코드든 분석의 root로 취급되어, 정적 참조가 없어도 제거되지 않습니다.

### **link.xml**

link.xml은 보존하려는 어셈블리, 타입, 멤버를 나열하는 XML 파일입니다.

프로젝트의 Assets 아래에 두면 UnityLinker가 빌드 시 읽어 들입니다.

앞 절의 샘플에서 타입째 제거된 ReflectionOnlyService는 다음 link.xml로 보존할 수 있습니다({{list:linkxml_reflection}}).

CODE START xml | ReflectionOnlyService를 보존하는 link.xml {{list:linkxml_reflection}}  
\<linker\>  
  \<assembly fullname="Assembly-CSharp"\>  
    \<type fullname="ReflectionOnlyService" preserve="all" /\>  
  \</assembly\>  
\</linker\>  
CODE END

이 link.xml을 Assets 바로 아래에 두고 High로 다시 빌드하자 실행 결과가 다음과 같이 바뀌었습니다.

CODE START text  
\# High(link.xml 없음)  
reflection: Type.GetType("ReflectionOnlyService") returned null

\# High(link.xml 있음)  
reflection: ReflectionOnlyService is alive  
CODE END

빌드에 포함된 어셈블리를 확인해도 ReflectionOnlyService가 Describe와 함께 보존되어 있음을 확인할 수 있습니다.

link.xml은 보존 대상을 코드 외부에서 지정할 수 있으므로, 직접 수정할 수 없는 DLL이나 패키지의 타입에도 사용할 수 있습니다.

보존 범위는 어셈블리 단위, 타입 단위, 메서드나 필드 같은 멤버 단위로 지정할 수 있습니다.

참고로 어셈블리 전체를 preserve=\"all\"로 보존하는 방식은 간편하지만, 해당 어셈블리에 대한 스트리핑 효과를 상쇄합니다.

문제를 분리하는 데는 사용할 수 있지만, 영구적인 대응으로는 필요한 타입과 멤버까지 좁히는 편이 좋습니다.

### **\[Preserve\] 속성**

직접 수정할 수 있는 코드라면 UnityEngine.Scripting 네임스페이스의 \[Preserve\] 속성을 대상에 직접 붙일 수 있습니다.

주의할 점은 \[Preserve\]는 붙인 위치에 따라 보존 범위가 달라진다는 것입니다.

이 차이를 타입에 붙인 클래스와 메서드에만 붙인 클래스, 두 가지로 확인합니다({{list:type_preserved}}, {{list:method_preserved}}).

CODE START csharp | 타입에 Preserve를 붙인 경우 {{list:type_preserved}}  
using UnityEngine.Scripting;

// 타입에 \[Preserve\]를 붙인다. 메서드에는 붙이지 않는다  
\[Preserve\]  
public sealed class TypePreservedService  
{  
    public string Describe() \=\> "TypePreservedService.Describe is alive";  
}  
CODE END

CODE START csharp | 메서드에만 Preserve를 붙인 경우 {{list:method_preserved}}  
using UnityEngine.Scripting;

// 타입에는 붙이지 않고 메서드에만 \[Preserve\]를 붙인다  
public sealed class MethodPreservedService  
{  
    \[Preserve\]  
    public string Describe() \=\> "MethodPreservedService.Describe is alive";  
}  
CODE END

이 두 클래스를 정적 참조를 갖지 않는 형태로 리플렉션으로 생성하여 호출하면 High에서의 결과는 다음과 같았습니다({{table:preserve_results}}).

表: Preserve를 붙인 위치와 High에서의 보존 범위 {{table:preserve_results}}

| 대상 | 타입 | 기본 생성자 | Describe |
| :---- | :---- | :---- | :---- |
| TypePreservedService | 남음 | 남음 | 제거됨 |
| MethodPreservedService | 남음 | 제거됨 | 남음 |

타입에 붙인 \[Preserve\]는 타입과 기본 생성자를 보존하지만, 속성을 붙이지 않은 Describe는 보존하지 않습니다.

메서드에 붙인 \[Preserve\]는 그 타입과 메서드를 보존하지만, 이번에는 기본 생성자가 보존되지 않아 Activator.CreateInstance를 통한 생성이 MissingMethodException으로 실패합니다.

리플렉션으로 생성하여 호출하는 데까지 동작시키려면 타입과 메서드 양쪽에 \[Preserve\]를 붙여야 합니다.

'\[Preserve\]를 붙였는데 동작하지 않는다'는 경우에는 먼저 속성을 붙인 위치와 보존 범위의 대응을 의심하시기 바랍니다.

참고: \[Preserving code using annotations\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-preserving.html)

## **응용편**

###  **Preserve 이외의 속성 활용**

\[Preserve\]가 '이 요소를 남긴다'는 점 단위의 지정인 데 비해, UnityEngine.Scripting 네임스페이스에는 '이 인터페이스의 구현을 모두 남긴다'처럼 패턴으로 보존을 선언하는 속성이 있습니다.

'사용자가 구현한 타입을 리플렉션으로 찾아 호출한다'와 같은 프레임워크에 흔한 구조에서는, 구현 타입을 하나씩 \[Preserve\]로 지정하는 대신 인터페이스나 기반 클래스에 속성을 붙여 대응할 수 있습니다.

* \[RequireImplementorsAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireImplementorsAttribute.html): 인터페이스에 붙이면 해당 인터페이스를 구현하는 타입을 보존합니다](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireImplementorsAttribute.html\): 인터페이스에 붙이면 해당 인터페이스를 구현하는 타입을 보존합니다)  
* \[RequireDerivedAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireDerivedAttribute.html): 클래스에 붙이면 해당 타입을 상속하는 타입을 보존합니다](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireDerivedAttribute.html\): 클래스에 붙이면 해당 타입을 상속하는 타입을 보존합니다)  
* \[RequiredMemberAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredMemberAttribute.html): 멤버에 붙이면 해당 타입이 남는 경우 그 멤버도 보존합니다](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredMemberAttribute.html\): 멤버에 붙이면 해당 타입이 남는 경우 그 멤버도 보존합니다)  
* \[RequiredInterfaceAttribute\](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredInterfaceAttribute.html): 타입에 붙이면 지정한 인터페이스의 구현 정보를 보존합니다

대표로 \[RequireImplementors\]의 동작을 확인합니다.

인터페이스에 속성을 붙이고, 구현은 어디에서도 정적으로 참조하지 않습니다({{list:require_implementors}}).

CODE START csharp | RequireImplementors를 붙인 인터페이스와 구현 {{list:require_implementors}}  
// 인터페이스에 붙이면 구현 타입이 보존 대상이 된다  
\[RequireImplementors\]  
public interface IRequiredContract  
{  
}

// 어디에서도 정적으로 참조되지 않는 구현  
public sealed class RequiredContractImpl : IRequiredContract  
{  
}  
CODE END

High로 빌드하면 RequiredContractImpl이 기본 생성자와 함께 보존되었습니다.

정적 참조를 갖지 않는 구현 타입이 인터페이스 쪽 선언만으로 남습니다.

\[RequireDerived\]를 붙인 기반 클래스의 파생 타입에서도 마찬가지로 보존을 확인할 수 있었습니다.

### **IUnityLinkerProcessor로 빌드 시 link.xml 생성하기**

보존할 대상 코드가 정해져 있다면 link.xml에 대상 클래스와 멤버를 지정하는 것만으로 충분합니다.

반면 빌드 타깃마다 보존하려는 타입이 다르거나 빌드에 포함하는 콘텐츠에 따라 필요한 타입이 달라지는 상황에서는, link.xml을 수작업으로 계속 갱신하는 것은 현실적이지 않습니다.

\*\*IUnityLinkerProcessor\*\*를 사용하면 빌드 시 link.xml을 동적으로 생성하여 UnityLinker에 전달할 수 있습니다.

IUnityLinkerProcessor는 에디터용 인터페이스이며, 구현 클래스를 에디터 어셈블리에 두면 빌드 시 자동으로 호출됩니다.

GenerateAdditionalLinkXmlFile이 반환한 경로의 link.xml이 UnityLinker의 입력에 추가됩니다.

최소한의 구현은 다음과 같습니다({{list:linker_processor}}).

CODE START csharp | link.xml을 생성하는 최소한의 IUnityLinkerProcessor 구현 {{list:linker_processor}}  
using System.IO;  
using UnityEditor.Build;  
using UnityEditor.Build.Reporting;  
using UnityEditor.UnityLinker;  
using UnityEngine;

public sealed class PreserveListLinkerProcessor : IUnityLinkerProcessor  
{  
    private const string LinkXmlContent \= @"\<linker\>  
  \<assembly fullname=""Assembly-CSharp""\>  
    \<type fullname=""GeneratedPreserveService"" preserve=""all"" /\>  
  \</assembly\>  
\</linker\>  
";

    public int callbackOrder \=\> 0;

    // 반환한 경로의 link.xml이 UnityLinker의 입력에 추가된다  
    public string GenerateAdditionalLinkXmlFile(BuildReport report, UnityLinkerBuildPipelineData data)  
    {  
        var path \= Path.Combine(Path.GetTempPath(), "managed-stripping-sample-generated-link.xml");  
        File.WriteAllText(path, LinkXmlContent);  
        Debug.Log("\[PreserveListLinkerProcessor\] generated 1 assembly / 1 type.");  
        return path;  
    }  
}  
CODE END

이 예시에서는 설명을 위해 생성 내용을 고정했지만, 실제로는 이 부분을 설정 에셋 읽기나 콘텐츠 의존성 분석처럼 빌드 시 입력으로부터 구성하는 처리로 대체합니다.

정적 참조를 갖지 않는 GeneratedPreserveService를 준비하고 High로 빌드하자, Assets 아래에 link.xml을 두지 않았는데도 타입, 생성자, 메서드가 보존되는 것을 확인할 수 있었습니다.

CODE START text  
\[GeneratedLinkXmlProbe\] type=present  
\[GeneratedLinkXmlProbe\] ctor=present, method=GeneratedPreserveService.Describe is alive  
CODE END

참고: \[IUnityLinkerProcessor\](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Build.IUnityLinkerProcessor.html)

###  **Addressables가 생성하는 link.xml**

Addressables는 앞 절에서 본 '콘텐츠 쪽에서만 필요한 타입이 사라지는' 문제에 대해, 콘텐츠 빌드의 산출물로 link.xml을 자동 생성하여 대응합니다.

실제로 무엇이 생성되는지 확인합니다. 콘텐츠는 최소 구성으로 하여, Cube 메시를 표시하기만 하는 프리팹(GameObject에 MeshFilter와 MeshRenderer를 붙인 것)을 하나 만들어 Addressables 그룹에 등록합니다.

콘텐츠 빌드를 실행하면 link.xml이 생성됩니다.

Addressables 4.0.1에서는 생성 위치가 Library/com.unity.addressables/aa/\<platform\>/AddressablesLink/link.xml입니다.

이번에 생성된 내용은 다음과 같았습니다({{list:addressables_linkxml}}).

CODE START xml | Addressables가 생성한 link.xml {{list:addressables_linkxml}}  
\<linker\>  
  \<assembly fullname="Unity.Addressables, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null" preserve="all"\>  
    \<type fullname="UnityEngine.AddressableAssets.Addressables" preserve="all" /\>  
  \</assembly\>  
  \<assembly fullname="Unity.ResourceManager, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null" preserve="all"\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.AssetBundleProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.BundledAssetProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.IAssetBundleResource" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.InstanceProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.SceneProvider" preserve="all" /\>  
  \</assembly\>  
  \<assembly fullname="UnityEngine.CoreModule, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null"\>  
    \<type fullname="UnityEngine.GameObject" preserve="all" /\>  
    \<type fullname="UnityEngine.MeshFilter" preserve="all" /\>  
    \<type fullname="UnityEngine.MeshRenderer" preserve="all" /\>  
    \<type fullname="UnityEngine.Transform" preserve="all" /\>  
  \</assembly\>  
\</linker\>  
CODE END

내용은 두 종류로 나뉩니다.

하나는 Addressables 런타임(Unity.Addressables와 Unity.ResourceManager)의 provider 타입으로, 그룹 구성에 따라 필요해지는 것입니다.

다른 하나는 등록한 콘텐츠가 참조하는 타입입니다. 이번에는 프리팹을 구성하는 GameObject, Transform, MeshFilter, MeshRenderer가 그대로 보존 대상으로 올라와 있습니다.

Addressables가 콘텐츠 쪽 의존성을 분석하여, 앱 코드에서 참조가 없는 타입도 보존 대상에 포함하고 있음을 알 수 있습니다.

참고: \[Addressables Build Artifacts\](https://docs.unity3d.com/ja/Packages/com.unity.addressables@1.20/manual/BuildArtifacts.html)(link.xml에 관한 기술은 1.20 매뉴얼에 있습니다)

### **리플렉션을 통한 코드 호출을 Source Generator에 의한 정적 호출로 대체하기**

지금까지의 대처 방법은 모두 필요한 코드가 제거되지 않도록 보존을 지정하는 것이었습니다.

이에 대해, 정적 분석에서 보이지 않는 의존성을 만드는 리플렉션 자체를 \*\*Source Generator\*\*에 의한 코드 생성으로 대체하는 방법도 있습니다.

Source Generator는 C\# 컴파일러가 컴파일 시 소스 코드를 분석하여 추가 소스 코드를 생성하는 구조입니다.

타입의 탐색과 생성을 컴파일 시에 끝내 두면 의존성이 처음부터 정적 참조로 표현되므로, 보존 지정 자체가 필요 없어집니다.

예로 커맨드 이름 문자열을 받아 해당하는 핸들러를 실행하는 디스패치 처리를 생각해 봅니다.

핸들러는 속성으로 커맨드 이름을 선언합니다. 이를 런타임에 리플렉션으로 만든다면 속성이 붙은 타입을 어셈블리에서 찾아 생성하는 구조가 됩니다({{list:command_handlers}}).

CODE START csharp | 속성으로 커맨드 이름을 선언하는 핸들러 {{list:command_handlers}}  
\[AttributeUsage(AttributeTargets.Class)\]  
public sealed class CommandHandlerAttribute : Attribute  
{  
    public CommandHandlerAttribute(string command)  
    {  
        Command \= command;  
    }

    public string Command { get; }  
}

public interface ICommandHandler  
{  
    string Execute();  
}

// 핸들러는 어디에서도 정적으로 참조되지 않는다  
\[CommandHandler("greet")\]  
public sealed class GreetCommandHandler : ICommandHandler  
{  
    public string Execute() \=\> "GreetCommandHandler.Execute succeeded";  
}

\[CommandHandler("farewell")\]  
public sealed class FarewellCommandHandler : ICommandHandler  
{  
    public string Execute() \=\> "FarewellCommandHandler.Execute succeeded";  
}  
CODE END

두 핸들러는 어디에서도 정적으로 참조되지 않으므로, 런타임 스캔 구조 그대로 High로 빌드하면 앞 절에서 본 것처럼 타입째 제거됩니다.

이 커맨드 이름에 대한 핸들러 해결을 Source Generator로 구현합니다.

\[CommandHandler\]가 붙은 클래스를 컴파일 시 수집하고, 커맨드 문자열로부터 구체 타입을 찾는 디스패처를 생성하는 제너레이터를 준비합니다.

제너레이터가 하는 일은 두 단계로, 전반(ForAttributeWithMetadataName)은 속성이 붙은 클래스에서 커맨드 이름과 타입 이름을 수집하는 것이고, 후반(GenerateDispatcher)은 모은 쌍을 템플릿에 채워 넣는 것입니다.

다음은 제너레이터의 전체 코드입니다({{list:dispatcher_generator}}).

CODE START csharp | 디스패처를 생성하는 Source Generator {{list:dispatcher_generator}}  
\#nullable enable  
using System.Collections.Immutable;  
using System.Linq;  
using System.Text;  
using Microsoft.CodeAnalysis;  
using Microsoft.CodeAnalysis.CSharp;  
using Microsoft.CodeAnalysis.Text;  
namespace StrippingSample.SourceGenerators  
{  
    \[Generator\]  
    public sealed class CommandDispatcherGenerator : IIncrementalGenerator  
    {  
        public void Initialize(IncrementalGeneratorInitializationContext context)  
        {  
            // \[CommandHandler\]가 붙은 클래스에서 커맨드 이름과 타입 이름의 쌍을 수집한다  
            var handlers \= context.SyntaxProvider  
                .ForAttributeWithMetadataName(  
                    "CommandHandlerAttribute",  
                    static (\_, \_) \=\> true,  
                    static (syntaxContext, \_) \=\> (  
                        Command: (string)syntaxContext.Attributes\[0\].ConstructorArguments\[0\].Value\!,  
                        TypeName: ((INamedTypeSymbol)syntaxContext.TargetSymbol)  
                            .ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat)))  
                .Collect();  
            // 수집한 쌍으로 디스패처를 생성한다  
            context.RegisterSourceOutput(handlers, static (sourceContext, commandHandlers) \=\>  
            {  
                if (\!commandHandlers.IsDefaultOrEmpty)  
                {  
                    sourceContext.AddSource(  
                        "CommandDispatcher.g.cs",  
                        SourceText.From(GenerateDispatcher(commandHandlers), Encoding.UTF8));  
                }  
            });  
        }  
        private static string GenerateDispatcher(  
            ImmutableArray\<(string Command, string TypeName)\> handlers)  
        {  
            // 출력을 결정론적으로 만들기 위해 커맨드 이름으로 정렬한 뒤 출력한다  
            var cases \= string.Join("\\n", handlers  
                .Distinct()  
                .OrderBy(static handler \=\> handler.Command, System.StringComparer.Ordinal)  
                .Select(static handler \=\> $$"""  
                            case {{SymbolDisplay.FormatLiteral(handler.Command, true)}}:  
                                return new {{handler.TypeName}}().Execute();  
                """));  
            return $$"""  
                // \<auto-generated/\>  
                public static class CommandDispatcher  
                {  
                    public static string Execute(string command)  
                    {  
                        switch (command)  
                        {  
                {{cases}}  
                            default:  
                                throw new global::System.ArgumentException($"Unknown command: {command}");  
                        }  
                    }  
                }  
                """;  
        }  
    }  
}  
CODE END

커맨드 이름으로 정렬하는 것은 같은 입력으로부터 같은 코드를 생성하여 빌드할 때마다 출력이 달라지지 않도록 하기 위해서입니다.

이 제너레이터에 의해 컴파일 시 다음과 같은 코드가 생성됩니다({{list:generated_dispatcher}}).

CODE START csharp | 생성되는 CommandDispatcher {{list:generated_dispatcher}}  
// \<auto-generated/\>  
public static class CommandDispatcher  
{  
    public static string Execute(string command)  
    {  
        switch (command)  
        {  
            case "farewell":  
                return new global::FarewellCommandHandler().Execute();  
            case "greet":  
                return new global::GreetCommandHandler().Execute();  
            default:  
                throw new global::System.ArgumentException($"Unknown command: {command}");  
        }  
    }  
}  
CODE END

런타임에 문자열로부터 타입을 찾던 처리가 문자열과 구체 타입의 대응표로 바뀌었습니다.

생성(new)과 Execute 호출이 모두 생성 코드에 나타나므로, 타입, 생성자, 메서드에 대한 정적 참조가 생성 코드 안에서 완결됩니다.

사용하는 쪽은 커맨드 문자열을 전달할 뿐이며, 핸들러의 구체 타입은 일절 참조하지 않습니다.

리플렉션 실험과 마찬가지로 커맨드 문자열은 static readonly 필드를 통해 전달하여, UnityLinker가 추적할 수 없는 입력인 채로 둡니다({{list:dispatch_probe}}).

CODE START csharp | 생성된 디스패처의 사용 측 {{list:dispatch_probe}}  
// 커맨드 문자열은 필드를 통해 전달하여 UnityLinker가 추적할 수 없는 입력으로 만든다  
private static readonly string\[\] Commands \= { "farewell", "greet" };

private static void RunGeneratedDispatcher()  
{  
    foreach (var command in Commands)  
    {  
        var result \= CommandDispatcher.Execute(command);  
        Debug.Log($"\[SourceGeneratorProbe\] dispatch command={command}, result={result}");  
    }  
}  
CODE END

이를 High로 빌드하여 실행하면 두 커맨드의 실행에 성공하는 것을 확인할 수 있었습니다.

CODE START text  
\[SourceGeneratorProbe\] dispatch command=farewell, result=FarewellCommandHandler.Execute succeeded  
\[SourceGeneratorProbe\] dispatch command=greet, result=GreetCommandHandler.Execute succeeded  
CODE END

문자열 자체는 빌드 시 추적할 수 없는 입력 그대로이지만, 문자열에서 타입으로의 대응이 런타임 탐색이 아니라 생성 코드의 switch가 되었으므로, UnityLinker에는 직접 작성한 코드와 다르지 않은 일반적인 정적 참조로 보입니다.

Mono.Cecil로 확인해도 두 핸들러의 타입, 생성자, Execute와 CommandDispatcher.Execute가 보존되어 있었습니다.

이 대체의 이점은 스트리핑에 대한 내성만이 아닙니다.

\- 의존성이 UnityLinker에 보이므로 link.xml이나 속성에 의한 보존 지정을 유지 관리할 필요가 없어진다  
\- 런타임 리플렉션 탐색이 없어지므로 시작 비용과 런타임 실패 가능성이 줄어든다  
\- 대상 수집이 컴파일 시에 이루어지므로 등록 누락이나 중복을 컴파일 시에 진단할 수 있다

실제 라이브러리에서도 이러한 구현을 채택한 사례가 있습니다.

시리얼라이저에서는 MessagePack for C\#이 IL2CPP 환경을 위한 코드 생성을 오래전부터 제공해 왔으며, 현재는 Source Generator 기반 생성으로 이행하고 있습니다.

DI 컨테이너에서는 VContainer가 SourceGenerator로 주입 코드를 생성하여 런타임 리플렉션을 줄이는 선택지를 제공하며, 앞 절에서 본 '생성자가 사라져 해결에 실패하는' 증상에 대한 대응을 Source Generator로 해결하고 있습니다.

Unity 프로젝트에 Source Generator를 도입하는 절차(netstandard2.0 DLL을 RoslynAnalyzer 레이블을 붙여 배치하는 구성)는 Unity 매뉴얼을 참조하시기 바랍니다.

참고: \[Create and use a source generator\](https://docs.unity3d.com/6000.3/Documentation/Manual/create-source-generator.html) / \[MessagePack for C\#\](https://github.com/MessagePack-CSharp/MessagePack-CSharp) / \[MemoryPack\](https://github.com/Cysharp/MemoryPack) / \[VContainer\](https://github.com/hadashiA/VContainer)

# **정리**

Managed Code Stripping은 Stripping Level을 올리는 것으로 끝낼 수 있는 최적화가 아닙니다.

이 장에서 살펴본 것처럼 리플렉션이나 외부 콘텐츠를 통한 의존성은 정적 분석에서 보이지 않으며, 런타임에 필요한 코드도 빌드에서 사라집니다.

더욱이 증상은 빌드한 앱에서만 나타나므로, 보이지 않는 의존성을 명시하는 것과 빌드한 앱에서 계속 검증하는 것을 세트로 운용해야 합니다.

보존을 명시하는 수단은 대상과 상황에 따라 구분하여 사용합니다({{table:approach_selection}}).

表: 대처 방법별 사용 구분 {{table:approach_selection}}

| 방법 | 적합한 상황 |
| :---- | :---- |
| \[Preserve\] | 직접 수정할 수 있는 코드의 소수 요소를 남긴다 |
| link.xml | 수정할 수 없는 DLL을 포함해 정해진 대상을 남긴다 |
| Require 계열 속성 | 프레임워크 측에서 '구현이나 파생을 남긴다'는 계약을 선언한다 |
| IUnityLinkerProcessor | 빌드 타깃이나 콘텐츠에 따라 보존 대상이 달라진다 |
| Source Generator | 리플렉션에 의한 탐색 자체를 정적 참조로 대체한다 |

아울러 스트리핑이 잘 적용되도록 하기 위한 방침을 제시합니다.

* 문제가 되는 경우를 제외하고 \[Preserve\]나 link.xml을 과도하게 사용하지 않는다  
* 리플렉션으로 타입을 탐색하고 생성하는 일을 줄이고, 명시적 등록이나 Source Generator에 의한 정적 참조로의 전환을 검토한다  
* 코드 보존 지정은 가능한 한 타입·멤버 단위까지 좁힌다. 어셈블리 전체의 preserve=\"all\"은 문제 분리용으로만 사용한다  
* 앱과 별도로 빌드하는 콘텐츠에서만 사용하는 타입의 보존은 수작업이 아니라 Addressables의 link.xml을 이용하거나 IUnityLinkerProcessor로 생성하는 등의 구조를 활용한다  
* Stripping Level을 올린 빌드를 개발 중부터 실행하여 검증한다
