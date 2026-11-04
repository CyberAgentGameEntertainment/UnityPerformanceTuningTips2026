# **iOS의 메모리 상한과 가상 주소 공간**

이름 / 로마자：Minhyuk Kim  
소속：주식회사 사이버에이전트 SGE 코어 기술본부

게임이 사용할 수 있는 기기의 메모리에는 당연히 상한이 있습니다. 씬 전환 중에 에셋이 겹치거나 임시 렌더링 버퍼가 늘어나면 이 상한이 문제가 됩니다.

게임 내에서 메모리 피크를 줄이는 것 외에, 메모리 상한 자체에도 조정할 여지가 있습니다. Xcode의 'Increased Memory Limit'은 지원 기기에서 그 상한을 끌어올리는 설정입니다. 별도 설정인 'Extended Virtual Addressing'은 프로세스가 사용할 수 있는 가상 주소 공간을 넓힙니다. 이름은 비슷하지만 대처할 수 있는 영역이 다릅니다.

이 장에서는 두 설정의 구분 사용과 실기기에서의 효과, App Store에서의 운용 경험을 소개합니다. 수치는 필자의 실기기 계측에 따른 것이며, Apple이 보증하는 기기별 상한이 아닙니다.

# **메모리 상한과 가상 주소 공간은 별개의 제약**

앱이 OOM(Out of Memory)에 이르렀을 때 최소한 다음 두 가지는 나누어 생각해야 합니다.

表: iOS 앱에서 구분해야 할 2종류의 메모리 제약 {{table:memory_constraints}}

| 제약 | 주로 보는 값 | 대표적인 발생 양상 |
| :---- | :---- | :---- |
| 물리 메모리 측 | `phys_footprint` | 높은 메모리 사용으로 시스템에 의해 종료된다(jetsam) |
| 가상 주소 공간 측 | VM Region Summary, Virtual Size | 메모리 할당이 실패하고 이후 크래시가 발생한다 |

물리 메모리는 실제로 RAM에 유지해야 하는 페이지와 관련됩니다. iOS에서 앱이 사용할 수 있는 물리 메모리 양은 기기의 탑재 RAM 그 자체가 아니라, 그 시점의 시스템 상태나 앱에 적용된 상한에도 좌우됩니다.

Apple의 `os_proc_available_memory()`는 현재 앱이 메모리 상한에 도달하기까지 할당할 수 있는 양을 반환합니다. Apple의 설명에 따르면 이 반환값은 「현재 메모리 상한에서 앱의 memory footprint를 뺀 값」에 대응하며, 기기의 여유 물리 메모리와 같다고는 할 수 없습니다. 또한 앱의 라이프사이클 중에도 상한이 바뀔 수 있으므로, 값은 캐시하지 말고 참고 정보로 다뤄야 합니다{{fn:os_proc_available_memory}}.

가상 주소 공간은 메모리나 파일을 어느 주소에 대응시킬지 관리하는 범위입니다. 예를 들어 큰 파일을 `mmap`해도 그 모든 페이지가 즉시 RAM에 로드되는 것은 아닙니다. 그러나 그 범위를 배치하기 위한 가상 주소는 필요합니다. 반대로 물리 메모리 footprint가 아직 낮고 `os_proc_available_memory()`에도 여유가 있는데, 새로운 주소 범위를 확보하지 못해 `malloc` 등이 실패하는 경우가 있습니다.

따라서 {{table:memory_constraints}}의 두 종류의 제약을 구분하려면 jetsam report가 남아 있는지, 일반 크래시 리포트에서 할당 실패가 시작점이 되었는지를 확인해야 합니다.
`os_proc_available_memory()`가 큰 값을 반환하더라도 가상 주소 공간에 같은 만큼의 여유가 있다고는 할 수 없습니다.

# **두 Entitlement의 역할**

## **Increased Memory Limit**

Increased Memory Limit의 키는 `com.apple.developer.kernel.increased-memory-limit`입니다. Apple은 지원하는 기기에서 기본 앱 메모리 상한을 넘음으로써 앱의 핵심 기능이 개선될 수 있음을 시스템에 알리는 Entitlement라고 설명합니다{{fn:increased_memory_limit}}. Xcode의 Capability 목록에서도 {{fig:xcode_increased_memory_limit}}와 같이 「지원 기기에서」라는 조건이 붙은 설명이 표시됩니다.

![][image1]{scale=0.8}
図: Xcode의 Capability 목록에 표시되는 Increased Memory Limit {{fig:xcode_increased_memory_limit}}

즉, 지정한다고 해서 모든 기기에서 일정량이 더해지는 것은 아닙니다. Apple 문서에도 증가한 상한을 사용할 수 있는 것은 일부 기기뿐이며, 추가 메모리를 사용할 수 없는 경우에도 앱이 올바르게 동작하도록 요구하는 기술이 있습니다.

### **Increased Memory Limit의 실기기 계측**

表: Increased Memory Limit 유무에 따른 OOM 시 메모리 계측 결과(iPhone) {{table:increased_memory_observations}}

| 탑재 RAM | 비활성 시 | 활성 시 |
| :---- | ----: | ----: |
| 3GB | 약 1,850MB | 약 1,850MB |
| 4GB | 약 2,050MB | 약 2,290MB |
| 6GB | 약 3GB | 약 4GB |

4GB와 6GB iPhone에서는 메모리가 각각 늘어난 반면, 3GB iPhone에서는 활성과 비활성 사이에 차이가 없었습니다.
또한 iPad는 애초에 OOM 시의 메모리가 iPhone과 다르므로 활성 시 결과도 다를 가능성이 높습니다.
대상 사용자의 기기 비율에 3GB iPhone이 많다면, Increased Memory Limit을 추가하는 것만으로 구제할 수 있다고 기대해서는 안 됩니다.

## **Extended Virtual Addressing**

Extended Virtual Addressing의 키는 `com.apple.developer.kernel.extended-virtual-addressing`입니다. Apple은 더 큰 주소 공간에 접근할 수 있음을 나타내는 Boolean 값이라고 설명하며, 예로 게임을 들고 있습니다{{fn:extended_virtual_addressing}}.

![][image2]{scale=0.8}
図: Xcode의 Capability 목록에 표시되는 Extended Virtual Addressing {{fig:xcode_extended_virtual_addressing}}

이 설정은 RAM을 늘리지 않습니다. 물리 메모리 상한에 가까워져 jetsam되는 앱에 Extended Virtual Addressing만 붙여도, 같은 양의 RAM을 계속 사용할 수 있다는 보장은 없습니다. 효과를 기대할 수 있는 것은 큰 메모리 맵, 단편화된 주소 영역, 큰 고정 예약 등으로 인해 물리 메모리를 다 쓰기 전에 가상 주소 공간이 먼저 빠듯해지는 경우입니다.

활성화하는 것 자체의 비용은 거의 없습니다. 페이지 테이블은 실제로 매핑한 범위에만 만들어지므로 상한을 넓혀도 늘어나지 않습니다{{fn:xnu_pmap}}. 고려해야 할 것은 효과가 나타나는 기기가 있는지입니다. Increased Memory Limit을 추가한 시점에 주소 공간도 넓어지므로, 둘 다 추가해도 `max_address`이 변하지 않는 기기가 있습니다.

한편 {{table:increased_memory_observations}}의 3GB 기기처럼 Increased Memory Limit의 효과가 나타나지 않는 기기에서는 Extended Virtual Addressing이 주소 공간을 넓히는 유일한 수단이 됩니다. Increased Memory Limit은 기기가 지원하지 않으면 서명이 올바르더라도 무효가 되지만, Extended Virtual Addressing에는 이 조건이 없습니다.

### **가상 주소 공간의 상한값**

Extended Virtual Addressing이 넓히는 것은 footprint 상한과는 별개의 주소 공간 상한입니다. 이 상한은 `pmap_max_64bit_offset()`이 탑재 메모리에 따라 반환하는 {{table:va_ceilings}} 중 하나의 값으로 결정됩니다{{fn:xnu_pmap}}.

表: arm64 사용자 프로세스의 주소 공간 상한 {{table:va_ceilings}}

| 조건 | 상수 | 상한 |
| :---- | :---- | ----: |
| `max_mem` > 3GB | `ARM64_MAX_OFFSET_DEVICE_LARGE` | 17.375GB |
| `max_mem` 1GB 초과~3GB | `ARM64_MAX_OFFSET_DEVICE_SMALL` | 13.375GB |
| `max_mem` 1GB 이하 | `ARM64_MIN_MAX_ADDRESS` | 12.5GB |
| entitlements를 포함한 어느 수단으로 확장 | `MACH_VM_JUMBO_ADDRESS` | 63GB |

`max_mem`는 커널이 인식하는 물리 메모리로, 카탈로그상의 RAM 용량보다 작아집니다.
모두 커널의 컴파일 시 상수이므로 OS 버전마다 바뀔 가능성이 있습니다.

## **App Store에서의 운영 실적**

이러한 Entitlements를 채택할 때 신경 쓰이는 것이 App Store 심사에 영향을 주지 않는가 하는 점입니다. 필자가 파악하고 있는 운영 중인 여러 타이틀에는 Increased Memory Limit, Extended Virtual Addressing 모두 설정을 활성화하여 App Store 심사를 통과하고 배포하고 있는 실적이 있습니다.

Apple 자신도 게임 개발자용 'Bring your high-end game to iPhone 15 Pro'{{fn:apple_game_memory}}에서 사용 가능한 메모리를 늘리기 위해 Increased Memory Limit을 활성화하도록 안내하고 있습니다. 이 두 가지는 제품에서의 채택을 검토할 수 있는 공개 기능입니다. 개별 앱 전체의 심사 결과를 보증하는 것은 아니지만, '메모리 상한과 관련된 설정이므로 심사에서 설명을 요구받거나 심사에서 탈락하지 않을까'라는 우려로 후보에서 제외할 필요는 없습니다.

# **App ID와 개발자 계정을 확인한다**

## **와일드카드 프로파일은 사용할 수 없다**

이 두 Entitlement는 와일드카드 형식의 App ID에서는 활성화할 수 없습니다.
`com.example.*`처럼 여러 앱을 한꺼번에 서명하는 구성의 경우,
게임의 Bundle ID와 일치하는 Explicit App ID로 전환해야 합니다.

이대로 서명하려고 하면 Xcode의 Signing & Capabilities에 {{fig:xcode_wildcard_profile_error}} 오류가 표시됩니다. Capability 이름과 Entitlement 키가 따로 보고되는 점에 주목하십시오. 프로파일 측이 Capability를 포함하고 있지 않다는 점과, 키에 대응하는 허가가 없다는 점이 모두 지적되고 있습니다.

![][image3]
図: 와일드카드 Provisioning Profile로 서명하려 할 때의 오류 {{fig:xcode_wildcard_profile_error}}

Apple Developer의 Certificates, Identifiers & Profiles에서 대상 App ID를 선택하고, {{fig:appid_increased_memory_limit}}와 같이 필요한 Capability를 활성화합니다. Apple도 App ID에 설정한 Capability가 앱이 이용할 수 있는 기능의 허용 목록이 되며, Explicit App ID가 필요한 기능은 와일드카드에서는 선택할 수 없다고 설명합니다{{fn:explicit_app_id}}.

![][image4]
図: App ID 설정에서 Increased Memory Limit을 활성화한다 {{fig:appid_increased_memory_limit}}

수동 서명에서는 Capability를 변경한 뒤 Provisioning Profile을 재생성하고, 빌드에서 사용하는 것을 갱신합니다. 자동 서명에서는 올바른 Team과 Bundle ID로 갱신된 프로파일이 선택되는지 확인합니다. 오래된 프로파일인 채로 `.entitlements`에 키만 추가해도, 요구하는 권한과 서명 측의 허가가 일치하지 않습니다. Unity 측 설정을 조사하기 전에 먼저 여기를 맞춥니다.

## **Extended Virtual Addressing에는 유료 멤버십이 필요하다**

Extended Virtual Addressing은 무료 Apple Account를 Xcode의 Personal Team으로 사용하는 구성에서는 활성화할 수 없습니다.
Apple의 Capability 대응표에서 대상으로 되어 있는 것은
Apple Developer Program과 Apple Developer Enterprise Program의 멤버십이며,
무료 Apple Account는 포함되어 있지 않습니다{{fn:capability_membership}}.

한편 Increased Memory Limit은 Personal Team에서도 활성화할 수 있습니다.

# **Unity의 빌드 후처리로 자동 설정한다**

Entitlements 파일에 들어가는 최소한의 내용은 {{list:memory_entitlements}}입니다. 두 가지를 동시에 활성화하는 예입니다.

CODE START xml | 메모리 관련 Entitlement의 최소 구성 {{list:memory_entitlements}}
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.developer.kernel.increased-memory-limit</key>
    <true/>
    <key>com.apple.developer.kernel.extended-virtual-addressing</key>
    <true/>
</dict>
</plist>
CODE END

Xcode의 Signing & Capabilities에서 수동으로 추가할 수도 있지만, 빌드를 내보낼 때마다 다시 추가해야 하므로 설정이 누락된 빌드가 섞이기 쉽습니다. Unity에서는 빌드 후처리에서 Entitlements 파일과 이를 참조하는 `CODE_SIGN_ENTITLEMENTS`을 한꺼번에 갱신할 수 있습니다. 처리는 서명 전의 빌드 시에 실행되며, 앱 시작 후에 C#에서 권한을 획득하는 것이 아닙니다.

## **Unity 6000.0.11f1 이후에서는 전용 API를 사용한다**

Unity 6000.0.11f1에서 `ProjectCapabilityManager`에 다음 메서드가 추가되었습니다{{fn:unity_memory_api}}.

* `AddIncreasedMemoryLimit()`
* `AddExtendedVirtualAddressing()`

이를 사용하면 Entitlements에 Boolean 값을 추가하는 작업과 Xcode 프로젝트에 반영하는 작업을 한꺼번에 수행할 수 있습니다.

대응하는 `PBXCapabilityType`의 정의도 마련되어 있습니다. 다만 이것은 C#의 enum이 아니라 Capability의 요건을 나타내는 클래스입니다. `PBXProject.AddCapability()`에 이 값을 전달하는 것과 Entitlements에 필요한 키를 추가하는 것은 같지 않습니다. 따라서 기본적으로는 둘 다 처리하는 `ProjectCapabilityManager`를 사용합니다.

다음은 `ProjectCapabilityManager`을 사용하여 Capability를 추가하는 샘플입니다.

CODE START csharp | Unity 6000.0.11f1 이후에서 메모리 Capability를 추가하는 빌드 후처리 {{list:unity_memory_postprocess}}
#if UNITY_IOS
using UnityEditor;
using UnityEditor.Callbacks;
using UnityEditor.iOS.Xcode;

static class IosMemoryCapabilities
{
    [PostProcessBuild]
    static void OnPostprocessBuild(BuildTarget target, string buildPath)
    {
        if (target != BuildTarget.iOS) return;

        string pbxPath = PBXProject.GetPBXProjectPath(buildPath);
        var pbx = new PBXProject();
        pbx.ReadFromFile(pbxPath);
        string mainTarget = pbx.GetUnityMainTargetGuid();

        string entitlements = pbx.GetBuildPropertyForAnyConfig(
            mainTarget, "CODE_SIGN_ENTITLEMENTS");
        if (string.IsNullOrEmpty(entitlements))
            entitlements = "Unity-iPhone.entitlements";

        var capabilities = new ProjectCapabilityManager(
            pbxPath, entitlements, targetGuid: mainTarget);
        capabilities.AddIncreasedMemoryLimit();
        capabilities.AddExtendedVirtualAddressing();
        capabilities.WriteToFile();
    }
}
#endif
CODE END

`UnityEditor.iOS.Xcode`은 iOS Build Support에 포함되어 있으므로 `#if UNITY_IOS`로 감쌉니다. 그렇지 않으면 이 모듈이 설치되지 않은 환경에서 Editor 전체의 컴파일이 통과하지 않게 되므로 주의하시기 바랍니다.

이 샘플은 모든 빌드 구성에서 공통 Entitlements 파일을 사용하고, 그 경로를 Xcode 프로젝트로부터의 상대 경로로 지정하는 구성을 대상으로 합니다. Debug와 Release에서 파일을 나누고 있거나 경로에 빌드 변수가 포함된 경우에는 각 구성의 실제 파일에 반영하는 처리에 맞추시기 바랍니다.

마지막 저장은 `capabilities.WriteToFile()`로 수행합니다. 다른 빌드 후처리에서 Xcode 프로젝트를 편집하는 경우,
`pbx`를 다시 저장하면 Capability Manager가 저장한 변경을 덮어쓸 수 있으므로,
갱신된 파일을 다시 읽은 뒤에 변경하도록 주의하십시오.

## **전용 API가 없는 버전에서는 plist를 직접 갱신한다**

전용 API가 없는 Editor에서 같은 처리를 하려면 {{list:unity_memory_postprocess}}의 `ProjectCapabilityManager`을 생성하고 저장하기까지의 5줄을 {{list:unity_memory_legacy}}로 교체합니다.

CODE START csharp | 전용 Capability API가 없는 Unity에서의 대체 처리 {{list:unity_memory_legacy}}
string fullPath = System.IO.Path.Combine(buildPath, entitlements);
System.IO.Directory.CreateDirectory(
    System.IO.Path.GetDirectoryName(fullPath));
var plist = new PlistDocument();
if (System.IO.File.Exists(fullPath))
    plist.ReadFromFile(fullPath);

plist.root.SetBoolean(
    "com.apple.developer.kernel.increased-memory-limit", true);
plist.root.SetBoolean(
    "com.apple.developer.kernel.extended-virtual-addressing", true);
plist.WriteToFile(fullPath);

if (string.IsNullOrEmpty(pbx.FindFileGuidByProjectPath(entitlements)))
    pbx.AddFile(entitlements, entitlements);
pbx.SetBuildProperty(mainTarget, "CODE_SIGN_ENTITLEMENTS", entitlements);
pbx.WriteToFile(pbxPath);
CODE END

기존 파일을 읽어 들인 후 두 키를 추가함으로써 Push 알림 등의 Entitlement를 지우지 않고 갱신합니다. 변경하는 것은 `.entitlements`이며 `Info.plist`이 아닙니다. 또한 파일은 `CODE_SIGN_ENTITLEMENTS`에서 참조하게 해야 비로소 서명에 사용됩니다.

C# 코드를 추가하는 것만으로 계정 측의 권한까지 활성화되는 것은 아닙니다.
어떤 방법이든 Apple Developer 측의 App ID나 Provisioning Profile은 별도로 설정해야 합니다.

# **적용을 2단계로 확인한다**

설정이 반영되었는지는 서명에 키가 들어 있는지와 실기기에서 커널이 반영했는지의 2단계로 나누어 확인합니다.

## **서명에 포함되어 있는지 확인한다**

수작업이든 빌드 후처리든 {{list:memory_entitlements}} 파일이 프로젝트 안에 생겼다는 것만으로는 실기기에 들어간 앱에 반영되었다는 증거가 되지 않습니다. Entitlement는 최종적으로 코드 서명에 포함됩니다. 내보낸 `.ipa`는 zip 형식이므로, 압축을 풀어 `Payload` 내의 앱 본체에 대해 {{list:codesign_entitlements}}을 실행합니다.

CODE START bash | 서명된 앱에 포함된 Entitlement 확인 {{list:codesign_entitlements}}
unzip -q YourApp.ipa -d ipa
codesign --display --entitlements :- ipa/Payload/YourApp.app
CODE END

출력된 plist에서 {{list:memory_entitlements}}의 두 키가 들어 있는지 확인합니다. Apple도 이 명령어를 통한 서명 후 확인을 안내하고 있습니다{{fn:diagnosing_entitlements}}.

## **실기기에서 상한이 바뀌었는지 확인한다**

{{list:codesign_entitlements}}로 키가 보이더라도 커널이 실제로 반영했다고는 할 수 없습니다. Increased Memory Limit은 기기 측이 지원하지 않는 경우 서명이 올바르더라도 무효로 취급됩니다.

실기기에서의 적용은 커널이 자기 프로세스에 대해 가지고 있는 값을 직접 읽어 판단합니다. 이를 취득하는 것이 `task_info()`이며, iOS SDK에 기본 포함된 Mach API입니다(`<mach/mach.h>`). C#에서는 호출할 수 없으므로 `Assets/Plugins/iOS/`에 둔 `.mm`에서 읽어 C#으로 반환합니다.

CODE START objc | 적용 상황을 읽는 네이티브 플러그인 {{list:task_vm_info_probe}}
#import <mach/mach.h>

extern "C" void UPT_ReadLimits(uint64_t *memlimit, uint64_t *ceiling)
{
    task_vm_info_data_t i;
    mach_msg_type_number_t c = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&i, &c) != KERN_SUCCESS) {
        return;
    }

    // footprint 상한(Increased Memory Limit)
    *memlimit = i.limit_bytes_remaining + i.phys_footprint;
    // 주소 공간 상한: 0xF_C000_0000까지 확장되어 있으면 확장된 상태
    *ceiling  = i.max_address;
}
CODE END

C# 측은 `DllImport("__Internal")`로 `UPT_ReadLimits`를 선언하여 호출합니다.

봐야 할 값은 상한입니다. `os_proc_available_memory()`가 반환하는 잔량은 footprint가 늘어나면 그만큼 작아지므로, 값만 봐서는 상한이 오르지 않은 것인지 단순히 많이 사용한 것인지 구별할 수 없습니다.

`limit_bytes_remaining`는 그 `os_proc_available_memory()`와 커널 내에서 같은 함수로부터 얻어지는 값이므로 {{fn:xnu_memorystatus}}, 현재의 footprint를 더하면 적용 중인 상한으로 되돌릴 수 있습니다. 주소 공간 측의 `max_address`도 같은 구조체에서 가져올 수 있으므로, 두 Entitlement를 한 번의 호출로 확인할 수 있습니다. `max_address`는 `TASK_VM_INFO_COUNT`를 전달하지 않으면 채워지지 않습니다.

다만 `max_address`에서 Extended Virtual Addressing 단독의 효과는 분리할 수 없습니다. Increased Memory Limit을 추가한 빌드나 Xcode의 Enhanced Security를 활성화한 빌드에서도 같은 상한이 됩니다. 확인할 수 있는 것은 '상한이 63GB까지 넓어졌는가'까지입니다.

# **iOS 26.4에서 드러난 가상 주소 예약**

## **`JS VM Gigacage (reserved)`는 1GB의 RAM 사용량이 아니다**

iOS 26.4를 탑재한 3GB iPhone에서의 검증에서는 크래시 리포트의 VM Region Summary에 `JS VM Gigacage (reserved)`로서 약 1GB의 영역이 나타났고, Extended Virtual Addressing을 활성화하면 OOM 시 메모리 양의 저하를 방지할 수 있었습니다.
이 영역은 본래의 Gigacage가 아니라, 기존 VM tag 이름을 재사용하여 표시된 JavaScriptCore의 Structure Heap 예약입니다{{fn:apple_forum_826325}}.

## **WebKit의 수정은 예약량을 기기 RAM에 따라 축소한다**

JavaScriptCore의 공개 버그 `315240`에는, 저메모리 기기에서 Structure Heap의 예약이 가상 주소 공간의 큰 비율을 차지하므로 물리 메모리 양에 따라 예약을 축소하는 방침이 기록되어 있습니다{{fn:webkit_bug_315240}}.

이 문제에 대응하는 WebKit 커밋에서, iOS 계열 플랫폼에 대해 기본 예약 크기와 물리 메모리의 20% 중 작은 쪽을 취하며,
다시 2의 거듭제곱으로 내림하여 Structure Heap의 희망 예약 크기를 결정하게 되었습니다.
공개 커밋의 설명에서는 3GB 기기에서 512MB가 되는 예가 제시되어 있습니다{{fn:webkit_commit_structure_heap}}.

이 대응이 들어간 iOS 26.6.1의 iPhone SE(2세대, RAM 3GB)에서 계측하면 표시상의 예약이 약 1GB에서 512MB로 축소되었으며,
26.4에서 저하되었던 OOM 시 메모리 양이 Extended Virtual Addressing을 활성화하지 않아도 해소되어 있었습니다.

# **정리**

Increased Memory Limit과 Extended Virtual Addressing은 활성화해도 잃는 것이 거의 없는 설정입니다. 페이지 테이블은 실제로 매핑한 범위에만 만들어지므로 주소 공간을 넓혀도 비용이 늘어나지 않습니다. App Store 심사를 통과하여 배포하고 있는 실적도 있습니다. Explicit App ID와 프로파일 준비만 되어 있다면 기본적으로 둘 다 활성화해 두는 것이 좋다고 생각합니다.

다만 효과는 기기에 따라 다릅니다. 필자의 iPhone 계측에서 Increased Memory Limit에 의한 여유는 4GB 기기에서 약 240MB, 6GB 기기에서 약 1GB였지만, 3GB 기기에서는 차이가 없었습니다. iPad에서는 별도 계측이 필요합니다. Extended Virtual Addressing은 물리 메모리를 늘리지 않으며, 가상 주소 공간이 먼저 부족해지는 경우에만 효과가 있습니다. iOS 26.4에서 나타난 Structure Heap 예약 문제는 iOS 26.6 계열에서 완화되었지만, 오래된 OS를 지원하는 경우나 앱 자체의 메모리 맵에서 주소 공간을 많이 사용하는 경우에는 계속 효과를 기대할 수 있습니다.

어느 쪽도 메모리 사용량 자체를 줄이지는 않습니다. 추가 상한이 없는 기기에서도 동작을 계속할 수 있도록, 텍스처나 임시 RenderTexture의 피크, 에셋의 상주 기간, 네이티브 플러그인의 큰 allocation, 파일 매핑의 수명을 줄이는 작업은 별도로 필요합니다. 두 Entitlement는 그 위에서 남는 여유를 넓히기 위한 설정으로 사용합니다.

脚注: {{fn:increased_memory_limit}} Apple Developer Documentation, `com.apple.developer.kernel.increased-memory-limit` [https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.increased-memory-limit](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.increased-memory-limit)

脚注: {{fn:extended_virtual_addressing}} Apple Developer Documentation, Extended Virtual Addressing Entitlement [https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.extended-virtual-addressing](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.extended-virtual-addressing)

脚注: {{fn:os_proc_available_memory}} Apple Developer Documentation, `os_proc_available_memory` [https://developer.apple.com/documentation/os/os_proc_available_memory](https://developer.apple.com/documentation/os/os_proc_available_memory)

脚注: {{fn:diagnosing_entitlements}} Apple Developer Documentation, Diagnosing Issues with Entitlements [https://developer.apple.com/documentation/bundleresources/diagnosing-issues-with-entitlements](https://developer.apple.com/documentation/bundleresources/diagnosing-issues-with-entitlements)

脚注: {{fn:apple_forum_826325}} Apple Developer Forums, iPadOS 26.4+ significantly reduced per-app memory limit from 6GB to 3GB on 8GB iPad, breaking memory-intensive apps [https://developer.apple.com/forums/thread/826325](https://developer.apple.com/forums/thread/826325)

脚注: {{fn:webkit_bug_315240}} WebKit Bugzilla 315240, Scale down StructureAlignedMemoryAllocator reservation in proportion to phys-mem size [https://bugs.webkit.org/show_bug.cgi?id=315240](https://bugs.webkit.org/show_bug.cgi?id=315240)

脚注: {{fn:webkit_commit_structure_heap}} WebKit commit `cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b`, Scale down structure-heap reservation size with physical memory [https://github.com/WebKit/WebKit/commit/cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b](https://github.com/WebKit/WebKit/commit/cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b)

脚注: {{fn:apple_game_memory}} Apple Developer Tech Talks, Bring your high-end game to iPhone 15 Pro [https://developer.apple.com/videos/play/tech-talks/111372/](https://developer.apple.com/videos/play/tech-talks/111372/)

脚注: {{fn:explicit_app_id}} Apple Developer, Register an App ID [https://developer.apple.com/help/account/identifiers/register-an-app-id/](https://developer.apple.com/help/account/identifiers/register-an-app-id/)

脚注: {{fn:capability_membership}} Apple Developer, Supported capabilities (iOS) [https://developer.apple.com/help/account/reference/supported-capabilities-ios/](https://developer.apple.com/help/account/reference/supported-capabilities-ios/)

脚注: {{fn:unity_memory_api}} Unity 6.0 Scripting API, ProjectCapabilityManager [https://docs.unity3d.com/6000.0/Documentation/ScriptReference/iOS.Xcode.ProjectCapabilityManager.html](https://docs.unity3d.com/6000.0/Documentation/ScriptReference/iOS.Xcode.ProjectCapabilityManager.html)

脚注: {{fn:unity_capability_release}} Unity 6000.0.11f1 Release Notes, iOS: Updated capability list(UUM-65394). API의 추가 내용은 공식 배포된 6000.0.10f1과 6000.0.11f1의 iOS Build Support 안에 있는 UnityEditor.iOS.Extensions.Xcode.dll을 비교하여 확인. [https://unity.com/releases/editor/whats-new/6000.0.11f1](https://unity.com/releases/editor/whats-new/6000.0.11f1)

脚注: {{fn:xnu_memorystatus}} Apple Open Source, xnu-12377.121.6, `bsd/kern/kern_memorystatus.c`（`memorystatus_available_memory_internal`） [https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/bsd/kern/kern_memorystatus.c](https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/bsd/kern/kern_memorystatus.c)

脚注: {{fn:xnu_pmap}} Apple Open Source, xnu-12377.121.6, `osfmk/arm/pmap/pmap.c`（`pmap_max_64bit_offset`） [https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/osfmk/arm/pmap/pmap.c](https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/osfmk/arm/pmap/pmap.c)

[image1]: assets/kim-minhyuk-2/media/image1.png
[image2]: assets/kim-minhyuk-2/media/image2.png
[image3]: assets/kim-minhyuk-2/media/image3.png
[image4]: assets/kim-minhyuk-2/media/image4.png
