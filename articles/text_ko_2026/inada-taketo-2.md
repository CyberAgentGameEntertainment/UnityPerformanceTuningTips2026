# **AssetBundle의 라이프타임 관리**

이름 / 로마자：Inada Taketo  
소속：주식회사 사이버에이전트 SGE 코어 기술본부  
X 계정 / 웹사이트 / GitHub 계정(선택):

게임에서는 텍스처, 메시, 오디오 등 많은 에셋을 런타임에 로드합니다. 이를 각 구현자가 개별적으로 관리하면 해제 누락이나 불필요한 에셋의 상주가 발생하기 쉽습니다.

따라서 AssetBundle이나 Addressables 등에서 동적으로 로드하는 에셋은 원칙적으로 로드와 해제의 창구를 시스템 기반에 집약하시기 바랍니다. 로드 시점에 에셋의 라이프타임을 결정하고, 그 라이프타임이 끝날 때 기반 측에서 일괄 해제할 수 있는 구조로 만듭니다.

이 절에서 말하는 '해제'는 로드 기반이 관리하는 참조나 해제용 핸들을 해제하는 것을 가리킵니다. 실제로 에셋이 메모리에서 언로드되는지는 기반의 구현, 참조 수, 의존 관계의 상태에 따라 달라집니다.

또한 이후에 소개하는 분류와 구현 방법은 참고 예시입니다. 각 프로젝트의 게임 구조에 맞는 방법을 선택하시기 바랍니다.

### **에셋의 라이프타임을 나누어 관리한다**

먼저 게임 내에서 에셋이 필요한 기간을 정리합니다. 예를 들어 다음과 같이 분류할 수 있습니다.

表: 에셋 라이프타임의 분류 예시 {{table:asset_lifetime_types}}

라이프타임	유지 기간의 예  
Common	로그아웃, 계정 전환, 타이틀 복귀 등 공통 데이터를 리셋할 때까지  
Scene	대상 씬을 종료할 때까지  
View	대상 화면이나 다이얼로그를 닫을 때까지  
Custom	전투나 이벤트 등 독자적으로 정한 기간이 끝날 때까지

중요한 것은 Common, Scene, View와 같은 이름이 아니라, 각 라이프타임이 언제 시작되고 언제 끝나는지를 명확히 하는 것입니다.

분류가 너무 거칠면 불필요한 에셋이 오랫동안 남고, 너무 세밀하면 로드와 해제 횟수 및 관리 비용이 늘어납니다. 또한 수명이 다른 에셋이 같은 AssetBundle에 포함되어 있으면, AssetBundle 단위의 언로드로는 일부 에셋만 골라서 언로드할 수 없습니다. 라이프타임 분류는 AssetBundle의 단위(전작 『Unity 퍼포먼스 튜닝 바이블』 제5장 「Tuning Practice \- AssetBundle」)와 함께 설계하시기 바랍니다.

### **로드 시점에 라이프타임을 결정한다**

라이프타임을 분류했다면 에셋을 로드할 때 등록 대상을 결정합니다.

구현 예시 중 하나로, 로드 함수의 인자로 라이프타임을 지정하는 방법이 있습니다.

CODE START csharp | 라이프타임을 지정하여 로드하는 예시 {{list:asset_load_with_lifetime}}  
var background \=  
    await AssetLoader.LoadAsync\<GameObject\>(  
        address,  
        AssetLifetime.Scene);  
CODE END

{{list:asset_load_with_lifetime}}의 예시에서는 로드한 에셋을 Scene 라이프타임에 등록합니다. 씬이 종료될 때 기반 측에서 등록된 에셋을 일괄 해제합니다.

다른 구현 예시로, 라이프타임마다 로드 함수를 나누는 방법도 있습니다.

CODE START csharp | 라이프타임마다 로드 함수를 나누는 예시 {{list:asset_load_by_lifetime}}  
var commonIcon \=  
    await AssetLoader.LoadCommonAsync\<Sprite\>(  
        commonAddress);

var stagePrefab \=  
    await AssetLoader.LoadSceneAsync\<GameObject\>(  
        stageAddress);

var dialogPrefab \=  
    await AssetLoader.LoadViewAsync\<GameObject\>(  
        dialogAddress,  
        viewId);  
CODE END

LoadCommonAsync, LoadSceneAsync, LoadViewAsync처럼 함수를 나누면 호출 지점에서 라이프타임을 판단할 수 있고, 등록 대상의 지정 실수도 방지하기 쉽습니다. 한편 인자로 지정하는 방법은 독자적인 라이프타임을 추가하기 쉽다는 특징이 있습니다.

어느 방법이든 중요한 것은 로드 시점에 라이프타임이 반드시 정해지고, 그 라이프타임이 끝날 때 기반 측에서 해제할 수 있다는 점입니다.

비동기 로드가 완료되기 전에 라이프타임이 끝나는 경우에 대비하여, 로드 시작 시점부터 처리를 추적하고 도중에 종료되면 취소하거나 완료 후의 결과를 해제할 수 있도록 합니다. 또한 같은 에셋을 여러 라이프타임에서 사용하는 경우에는 참조 수를 관리하고, 마지막 사용이 끝난 시점에 해제합니다.

### **GameObject에 라이프타임을 연결한다**

다른 관리 방법으로, GameObject를 라이프타임의 소유자로 삼는 방법이 있습니다.

화면이나 다이얼로그의 루트 GameObject에 전용 Component를 추가하고, 로드한 에셋의 해제용 핸들을 등록합니다. GameObject가 파괴될 때 OnDestroy에서 등록된 핸들을 해제합니다.

CODE START csharp | GameObject에 라이프타임을 연결하는 예시 {{list:asset_scope_component}}  
public sealed class AssetLifetimeScope : MonoBehaviour  
{  
    private readonly List\<IDisposable\> \_handles \=  
        new List\<IDisposable\>();

    private bool \_released;

    public void Add(IDisposable handle)  
    {  
        if (\_released)  
        {  
            handle.Dispose();  
            return;  
        }

        \_handles.Add(handle);  
    }

    private void OnDestroy()  
    {  
        \_released \= true;

        for (var i \= \_handles.Count \- 1; i \>= 0; i--)  
        {  
            \_handles\[i\].Dispose();  
        }

        \_handles.Clear();  
    }  
}  
CODE END

{{list:asset_scope_component}}의 예시에서는 GameObject가 파괴된 후에 비동기 로드가 완료되어 Add가 호출된 경우에도, 추가된 핸들을 즉시 해제합니다.

다만 GameObject를 파괴하는 것만으로 동적으로 로드한 에셋이 자동으로 해제되지는 않습니다. 전용 Component에서 로드 기반에 대응하는 해제 처리를 호출해야 합니다.

관리용 GameObject는 에셋마다 만들지 않고, 하나의 화면이나 다이얼로그 등 하나의 라이프타임 단위당 하나로 통합합니다. 이 방법은 GameObject와 에셋의 수명이 일치하는 경우에 유효하지만, 여러 씬에 걸쳐 사용하는 에셋 등에는 적합하지 않을 수 있습니다. 필요에 따라 라이프타임별 관리 테이블과 구분하여 사용하시기 바랍니다.

### **해제 누락을 확인한다**

Memory Profiler로 화면이나 씬 전환 전후의 스냅숏을 취득하고, 종료된 라이프타임의 에셋이 남아 있지 않은지 비교합니다. 같은 전환을 여러 번 반복하여 에셋이나 로드 핸들이 계속 늘어나지 않는지 확인하는 것도 유효합니다.

계측은 필요한 해제 처리가 완료된 후에 같은 기준 상태끼리 비교하고, 최종적으로는 실기기의 플레이어 빌드에서 확인하시기 바랍니다. 다만 다른 기능에서 참조 중인 에셋이나 캐시로서 의도적으로 보유하는 에셋도 있으므로, 남아 있다는 사실만으로 해제 누락이라고 판단하지 말고 실제 참조 위치까지 확인합니다.

에셋 관리는 개발 후반에 변경하면 많은 로드 처리에 영향을 줍니다. 개발 초기 단계에서 필요한 라이프타임을 정리하고, 구현자가 개별 해제 시점을 의식하지 않아도 프로젝트에 맞는 라이프타임이 적용되는 구조를 설계해 두시기 바랍니다.
