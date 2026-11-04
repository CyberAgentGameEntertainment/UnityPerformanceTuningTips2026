* GraphicsStateCollection을 이용한 셰이더 워밍업  
* Amano Takuro/주식회사 사이버에이전트 SGE 코어 기술본부

# 

셰이더가 처음 렌더링에 사용될 때 런타임에 컴파일 처리가 실행되어, 셰이더 컴파일 스터터라고 불리는 프레임 저하를 일으킬 수 있습니다. 이 장에서는 모바일 환경(Metal·Vulkan)을 대상으로 GraphicsStateCollection을 이용한 셰이더 워밍업의 기본적인 사용법부터 도입 후에도 스터터가 남는 실제 사례와 그 대처까지 소개합니다. 또한 이 장의 내용은 Unity 6.3 LTS(6000.3)를 대상으로 합니다.

# **GraphicsStateCollection 개요**

GraphicsStateCollection(GSC)은 Unity 6에서 사용할 수 있게 된 API입니다{{fn:gsc_docs}}. 게임 플레이 중에 사용되는 셰이더 베리언트와 그래픽스 상태(렌더링 시의 렌더링 설정)의 조합을 기록하고, 이를 한꺼번에 사전 컴파일, 즉 워밍업하는 구조입니다. Metal·Vulkan·D3D12 등 모던 그래픽스 API용 ShaderVariantCollection의 대체 수단으로 자리매김하고 있습니다. 다만 Unity 6000.3 시점에는 \`UnityEngine.Experimental.Rendering\` 네임스페이스에 속한 Experimental API이므로 향후 버전에서 사양이 변경될 수 있습니다.  
![][image1]  
図: GSC에 기록되는 정보의 예 {{fig:gsc_recorded_info}}

그림은 GSC에 기록된 정보를 스크립트로 가져와 EditorWindow에 표시한 예입니다. 기록 내용을 가져오는 방법은 이 장의 마지막({{list:gsc_variant_list}})에서 소개합니다.

셰이더는 그대로는 GPU에서 실행할 수 없으며 GPU의 머신 명령으로 컴파일해야 합니다. 그런데 머신 명령은 GPU 기종이나 드라이버에 따라 다르므로 모든 기기을 대상으로 사전 컴파일한 상태로 배포할 수는 없습니다. 따라서 셰이더는 다음 흐름을 거쳐 GPU에서 실행됩니다.

1\. 개발자가 HLSL 등의 셰이더 언어로 셰이더(에셋)를 작성한다  
2\. 빌드 시 Unity가 셰이더를 그래픽스 API별 중간 표현으로 컴파일하여 앱이나 AssetBundle에 저장한다. Vulkan에서는 SPIR-V, Metal에서는 Metal Shading Language(MSL)가 이 중간 표현에 해당한다{{fn:gsc_spirv_msl}}  
3\. 앱 실행 중 셰이더가 처음 렌더링에 사용될 때, 저장된 중간 표현으로부터 셰이더 프로그램을 생성한다  
4\. 이어서 셰이더 프로그램과 렌더링 설정을 조합하여 GPU 드라이버가 GPU의 머신 명령으로 컴파일한다

3·4단계의 런타임 컴파일을 첫 렌더링 이전에 한꺼번에 끝내 두는 것이 셰이더 워밍업의 기본 개념입니다.

또한 하나의 셰이더에서는 패스(렌더링 공정)별로, 그리고 라이트의 영향이나 포그 유무 같은 조건(셰이더 키워드)의 조합별로 여러 실행 프로그램이 생성됩니다. 이렇게 생성되는 단위를 셰이더 베리언트라고 부릅니다.

## **Pipeline State Object**

모던 그래픽스 API에서 렌더링 시 컴파일되는 단위를 Pipeline State Object(PSO)라고 부릅니다. PSO는 셰이더 베리언트와 렌더링 설정(버텍스 레이아웃, 컬러 포맷이나 MSAA 등의 렌더 패스 어태치먼트, 블렌드 스테이트, 뎁스 테스트, 스텐실 스테이트)을 하나로 묶어 GPU 드라이버가 GPU용 머신 명령으로 컴파일한 오브젝트입니다. 생성된 PSO는 기기에 캐시되며 동일한 렌더링 설정이라면 재사용됩니다. 즉 같은 셰이더 베리언트라도 렌더링 설정이 다르면 별개의 PSO로 컴파일됩니다.

## **셰이더 컴파일 스터터의 발생 원인**

스터터의 원인은 런타임에 실행되는 2단계 컴파일 처리입니다. GPU가 셰이더 베리언트를 처음 렌더링할 때 Main Thread에서의 셰이더 프로그램 생성(3단계)과 Render Thread에서의 GPU 드라이버에 의한 PSO 생성(4단계, 머신 명령으로의 컴파일)이 발생합니다.

Unity Profiler에서 전자는 Main Thread의 \`Shader.CreateGPUProgram\`, 후자는 Render Thread의 \`GpuProgramMetal.GetCachedPipeline\`(iOS) 또는 \`CreateGraphicsPipelineImpl\`(Android)로 관찰할 수 있습니다. 특히 Metal(iOS)·Vulkan(Android)에서는 PSO 생성 비용이 커서 첫 렌더링 시 스터터의 주된 원인이 됩니다. GSC를 사용한 워밍업은 이 PSO 생성을 게임 시작 직후 등 원하는 시점에 끝내 두는 수단입니다.
  
기존 ShaderVariantCollection은 "어떤 셰이더 베리언트를 컴파일할지"는 관리할 수 있지만 렌더링 설정은 가지고 있지 않습니다. 따라서 ShaderVariantCollection의 워밍업은 3단계의 셰이더 프로그램 생성까지만 수행하며, 실제 렌더링 시에는 GPU 드라이버가 다시 PSO를 생성하므로 스터터가 남습니다.  

![][image2]![][image3]  
図: Main Thread와 Render Thread에서의 컴파일 발생 예 {{fig:gsc_profiler_stutter}}

그림은 Vulkan(Android)에서 컴파일이 발생한 프레임을 Profiler의 Timeline 뷰로 표시한 예입니다. 위쪽은 Main Thread의 \`Shader.CreateGPUProgram\`를 선택한 모습으로, 대상 셰이더 이름·패스 이름·키워드를 확인할 수 있습니다. 아래쪽은 Render Thread의 \`CreateGraphicsPipelineImpl\`를 선택한 모습으로, 이 예에서는 PSO 1회 생성에 수십 밀리초가 걸려 스터터의 주된 원인이 됨을 알 수 있습니다.

# **GSC를 사용한 워밍업 흐름**

GSC를 사용한 워밍업은 "렌더링 설정 수집(트레이스)"과 "워밍업(사전 컴파일)"의 두 단계로 나뉩니다.

## **렌더링 설정 수집**

트레이스의 기본 형태를 {{list:gsc_trace_basic}}에 나타냅니다. 트레이스 중에 렌더링된 오브젝트의 렌더링 설정이 \`GraphicsStateCollection.GraphicsState\`로서 GSC에 기록됩니다. 리스트에서는 하나의 메서드로 묶었지만, 실제로는 \`BeginTrace()\`와 \`EndTrace()\`를 서로 다른 타이밍에 호출하여 여러 프레임에 걸쳐 렌더링을 수집합니다.

CODE START csharp | 렌더링 설정 트레이스 {{list:gsc_trace_basic}}  
private GraphicsStateCollection \_gsc;

void Trace() {  
    // 수집 시작  
    \_gsc \= new GraphicsStateCollection();  
    \_gsc.BeginTrace();

    // 여기서 여러 프레임에 걸쳐 웜업 대상 오브젝트를 렌더링한다  
    // 렌더링된 렌더링 설정이 GraphicsState로 기록된다

    // 수집 종료·저장  
    \_gsc.EndTrace();  
    \_gsc.SaveToFile(path);  
}  
CODE END

생성되는 \`GraphicsStateCollection.GraphicsState\`는 그래픽스 API마다 다릅니다. 트레이스는 빌드한 앱을 실제로 실행하는 디바이스에서 수행합시다.

## **워밍업(사전 컴파일)**

수집한 GSC를 로드하여 워밍업을 실행합니다({{list:gsc_warmup_progressive}}). 이 예에서는 프로젝트에 가져온 GSC를 Addressables 에셋으로 로드합니다.

CODE START csharp | WarmUpProgressively에 의한 분산 워밍업 {{list:gsc_warmup_progressive}}  
async Task WarmUpAsync() {  
    // 셰이더를 포함한 에셋을 로드한 후에 실행한다  
    \_gsc \= await Addressables.LoadAssetAsync\<GraphicsStateCollection\>(address).Task;

    // 단계적으로 WarmUp(Jobs 기반. 매 프레임 n건씩 컴파일)  
    StartCoroutine(WarmUpCoroutine());  
}

IEnumerator WarmUpCoroutine() {  
    JobHandle handle \= default;  
    int requestedCount \= 0;  
    // 오류 시 isWarmedUp이 true가 되지 않는 경우가 있으므로 요청 건수로 종료를 판정한다  
    while (requestedCount \< \_gsc.totalGraphicsStateCount) {  
        handle \= \_gsc.WarmUpProgressively(4, handle);  
        handle.Complete();  
        requestedCount \+= 4;  
        yield return null;  
    }  
}  
CODE END

워밍업을 구현할 때의 포인트는 다음과 같습니다.

\* \`WarmUp(dependency)\`로도 워밍업을 실행할 수 있지만 모든 컴파일을 한꺼번에 수행하므로 스파이크가 발생합니다. \`WarmUpProgressively(count, dependency)\`로 1회당 컴파일 건수를 지정하여 여러 프레임에 분산시킵시다  
\* 셰이더 에셋을 로드한 후에 실행합니다. 셰이더 인스턴스가 존재하지 않으면 해당 베리언트의 워밍업은 건너뜁니다  
\* 완료 여부는 \`isWarmedUp\`나 \`completedWarmupCount \== totalGraphicsStateCount\`로 확인할 수 있습니다  
\* 다만 워밍업 에러가 발생하면 \`isWarmedUp\`가 true가 되지 않는 경우가 있습니다(Unity 6000.3에서 확인). 이 때문에 리스트에서는 요청한 건수의 합계와 \`totalGraphicsStateCount\`을 비교하여 종료를 판정함으로써 무한 루프를 피하고 있습니다

# **수집 시나리오 구현 예**

앱 시작 시 \`BeginTrace()\`를 호출하고 수동으로 플레이하여 렌더링 설정을 수집하는 방법도 있습니다. 그러나 앱 규모에 따라서는 버전 업데이트 때마다 수동으로 빠짐없이 수행하기가 어렵습니다. 그래서 수집 전용 씬에서 에셋을 순차적으로 렌더링하는 시나리오를 기계적으로 실행하여 수집하는 방법을 소개합니다. 구현 예에는 Addressables와 UniTask{{fn:gsc_unitask}}를 사용합니다. 이를 사용하지 않는 경우에도 에셋의 비동기 로드와 1프레임 렌더링 대기를 자신의 프로젝트 구조로 대체하면 같은 흐름을 구현할 수 있습니다. 전체 흐름은 다음과 같습니다.

1\. 필요한 에셋을 다운로드한다(Built-in 에셋만으로 완결되는 경우에는 불필요)  
2\. 수집에 필요한 에셋의 Address를 Addressables의 콘텐츠 카탈로그(\`IResourceLocator\`)에서 가져온다  
3\. Quality Settings를 전환한다(Quality Settings에서 셰이더가 변하지 않는 경우에는 전환 불필요)  
4\. 수집을 시작한다(\`BeginTrace()\`)  
5\. 씬을 additive 로드하고, 수집 대상 에셋을 순차적으로 로드하여 렌더링한다(모델 렌더링·\`ParticleSystem\` 재생·Timeline 재생)  
6\. 수집을 완료한다(\`EndTrace()\`)  
7\. 불필요한 셰이더를 GSC에서 삭제한다  
8\. GSC를 저장한다(\`SaveToFile(path)\`)  
9\. 씬을 언로드하고 Quality Settings를 전환하면서 3~8을 반복한다

콘텐츠 카탈로그에서 렌더링 설정에 영향을 주는 에셋(씬), 캐릭터나 프롭 등의 모델 에셋, 이펙트 에셋, Timeline 에셋을 가져와 렌더링 설정을 전환하면서 순차적으로 렌더링함으로써 포괄적으로 수집합니다. Quality Settings마다 수집하는 이유는 Quality Level에 따라 MSAA나 HDR 등 PSO에 영향을 주는 렌더링 설정이 바뀌기 때문입니다.

처리의 골격을 {{list:gsc_scenario_skeleton}}에 나타냅니다. Quality Settings마다 트레이스를 반복하고 Quality Level 이름을 붙여 별도 파일로 저장합니다. 실행 시에는 현재 Quality Level에 대응하는 파일을 선택하여 로드하고 워밍업합니다.

CODE START csharp | 수집 시나리오의 골격 {{list:gsc_scenario_skeleton}}  
private GraphicsStateCollection \_gsc \= new GraphicsStateCollection();

private async UniTaskVoid StartAsync() {  
    // 콘텐츠 카탈로그 취득과 필요한 에셋 다운로드  
    // Prefix로 각종 Address 목록을 사전에 취득한다  
    await AddressableSetupAsync();

    var qualitySettingNames \= QualitySettings.names;  
    for (var i \= 0; i \< qualitySettingNames.Length; i++) {  
        QualitySettings.SetQualityLevel(i);  
        await TraceAsync();  
    }  
}

private async UniTask TraceAsync() {  
    // GSC 수집 시작  
    \_gsc.ClearVariants();  
    \_gsc.BeginTrace();

    // 수집 시나리오를 실행  
    await ExecuteScenarioAsync();

    // GSC 수집 종료  
    \_gsc.EndTrace();

    // 수집 대상이 아닌 셰이더를 GSC에서 제거한다  
    RemoveUnnecessaryVariants();

    var qualityLevelName \= QualitySettings.names\[QualitySettings.GetQualityLevel()\];  
    \_gsc.SaveToFile(\_gscSavePathFormat \+ $"\_{qualityLevelName}");  
}  
CODE END

시나리오 본체에서는 씬마다 모델·이펙트·Timeline을 순차적으로 렌더링합니다. {{list:gsc_scenario_batch}}은 캐릭터 모델을 몇 개씩 배치로 생성·렌더링하는 부분을 발췌한 것입니다. 1프레임 렌더링을 기다리면 해당 프레임의 렌더링 설정이 GSC에 기록됩니다.

CODE START csharp | 모델 에셋의 순차 렌더링(발췌) {{list:gsc_scenario_batch}}  
// 캐릭터 모델을 수집  
for (var i \= 0; i \< \_charaAddressArray.Length; i \+= \_characterNum) {  
    // 캐릭터 모델을 몇 개체씩 생성  
    var batchCount \= Mathf.Min(\_characterNum, \_charaAddressArray.Length \- i);  
    for (var j \= 0; j \< batchCount; j++) {  
        var charaHandle \= Addressables.LoadAssetAsync\<GameObject\>(\_charaAddressArray\[i \+ j\]);  
        var charaPrefab \= await charaHandle.ToUniTask();  
        // 카메라에 들어오는 위치에 균등 배치  
        var pos \= new Vector3((j \- batchCount \* 0.5f) \* 2f, 0f, 5f);  
        \_charaObjectArray\[j\] \= Object.Instantiate(charaPrefab, pos, Quaternion.identity);  
        Addressables.Release(charaHandle);  
    }

    // 1프레임 렌더링 대기  
    await UniTask.NextFrame();

    for (var j \= 0; j \< batchCount; j++) {  
        DestroyImmediate(\_charaObjectArray\[j\]);  
        \_charaObjectArray\[j\] \= null;  
    }  
}  
CODE END

\`ParticleSystem\`도 마찬가지로 몇 개씩 배치로 생성하지만, \`Simulate()\`로 재생 도중의 상태(이 예에서는 0.5초 후)를 동기적으로 재현한 뒤 렌더링한다는 점이 다릅니다({{list:gsc_scenario_particle}}). 실시간 재생을 기다릴 필요가 없으므로 수집 시간을 단축할 수 있습니다.

CODE START csharp | ParticleSystem 수집(발췌) {{list:gsc_scenario_particle}}  
// ParticleSystem을 수집  
for (var i \= 0; i \< \_particlesAddressArray.Length; i \+= \_effectNum) {  
    var batchCount \= Mathf.Min(\_effectNum, \_particlesAddressArray.Length \- i);  
    for (var j \= 0; j \< batchCount; j++) {  
        var particleHandle \= Addressables.LoadAssetAsync\<GameObject\>(\_particlesAddressArray\[i \+ j\]);  
        var particlePrefab \= await particleHandle.ToUniTask();  
        var pos \= new Vector3((j \- batchCount \* 0.5f) \* 3f, 0f, 5f);  
        \_particleObjectArray\[j\] \= Object.Instantiate(particlePrefab, pos, Quaternion.identity);  
        Addressables.Release(particleHandle);

        // \_effectSimulateTime초 후의 상태를 동기 처리로 재현(Play는 불필요)  
        var ps \= \_particleObjectArray\[j\].GetComponentInChildren\<ParticleSystem\>();  
        if (ps \!= null) {  
            ps.Simulate(\_effectSimulateTime, withChildren: true, restart: true);  
        }  
    }

    // 1프레임 렌더링 대기  
    await UniTask.NextFrame();

    for (var j \= 0; j \< batchCount; j++) {  
        DestroyImmediate(\_particleObjectArray\[j\]);  
        \_particleObjectArray\[j\] \= null;  
    }  
}  
CODE END

Timeline을 사용한 연출은 \`PlayableDirector\`에서 실제로 재생하여 수집합니다({{list:gsc_scenario_timeline}}). 재생 시간만큼 기다리므로 대상 Timeline 에셋은 수집에 필요한 것으로 좁히는 것이 좋습니다.

CODE START csharp | Timeline 수집(발췌) {{list:gsc_scenario_timeline}}  
// Timeline을 수집  
foreach (var timelineAddress in \_timelineAddressArray) {  
    var timelineHandle \= Addressables.LoadAssetAsync\<GameObject\>(timelineAddress);  
    var timelinePrefab \= await timelineHandle.ToUniTask();  
    var timelineObj \= Object.Instantiate(timelinePrefab, Vector3.zero, Quaternion.identity);  
    Addressables.Release(timelineHandle);  
    var director \= timelineObj.GetComponent\<PlayableDirector\>();  
    if (director \!= null) {  
        director.Play();  
        await UniTask.Delay((int)(director.duration \* 1000));  
    }  
    DestroyImmediate(timelineObj);  
}  
CODE END

수집이 끝나면 워밍업 대상으로 삼을 셰이더만 남깁니다({{list:gsc_remove_variants}}). 남길 셰이더를 허용 목록(\`\_allowShaderNames\`)으로 지정하고, 대상 외 셰이더와 참조가 끊어진(null) 베리언트를 GSC에서 삭제합니다.

CODE START csharp | 수집 대상 외 베리언트 삭제 {{list:gsc_remove_variants}}  
private void RemoveUnnecessaryVariants() {  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    \_gsc.GetVariants(variants);

    foreach (var variant in variants) {  
        if (variant.shader \== null) {  
            \_gsc.RemoveVariant(null, variant.passId, variant.keywords);  
            continue;  
        }

        if (\_allowShaderNames.Contains(variant.shader.name)) {  
            continue;  
        }

        // 불필요한 셰이더를 GSC에서 제거  
        \_gsc.RemoveVariant(variant.shader, variant.passId, variant.keywords);  
    }  
}  
CODE END

시나리오나 에셋 관리 방법은 프로젝트마다 다르므로 씬 구성과 수집 대상은 자신의 프로젝트에 맞게 조정하십시오.

# **스터터가 남는 실제 사례와 대처**

GSC에 의한 워밍업을 도입했는데도 스터터가 남는 경우가 있습니다. 여기서는 그 실제 사례와 대처를 소개합니다.

## **Shader Stripping으로 인한 베리언트 누락**

워밍업 시 로그에 \`variant not found\` 에러가 발생하고, 런타임에서도 해당 베리언트의 컴파일(Profiler의 \`Shader.CreateGPUProgram\` 등)이 관찰되지 않는 경우입니다.

원인은 GSC에 포함된 셰이더 베리언트가 앱 또는 AssetBundle에 포함되어 있지 않은 것입니다. \*\*Project Settings \> Graphics \> Shader Stripping\*\* 설정에 따라서는 빌드 시 사용되지 않는 베리언트가 제외됩니다. 셰이더나 에셋 변경으로 필요한 베리언트가 바뀌었을 때 발생하기 쉬운 문제입니다.

Shader Stripping 설정을 확인하고, 베리언트에 영향을 주는 변경을 했을 때는 GSC를 다시 수집합시다.

참고로 GSC에 베리언트는 기록되어 있지만 \`GraphicsState\`가 0건인 경우에도 동일한 \`variant not found\` 에러가 발생합니다. 이 경우에는 런타임에서 해당 베리언트의 컴파일이 발생하므로, 런타임 컴파일 발생 여부로 구분할 수 있습니다.

## **메타데이터 불일치로 인한 mismatch 경고**

워밍업 실행 시 콘솔에 다음 경고가 출력되는 경우입니다.

CODE START text  
WarmUp may be inaccurate due to GraphicsDeviceType mismatch.  
WarmUp may be inaccurate due to RuntimePlatform mismatch.  
WarmUp may be inaccurate due to Quality Level mismatch.  
CODE END

원인은 GSC 파일에 저장된 \`graphicsDeviceType\` / \`runtimePlatform\` / \`qualityLevelName\` 메타데이터가 실행 환경과 일치하지 않는 것입니다. 주요 발생 케이스는 다음 두 가지입니다.

\* 다른 플랫폼에서 수집한 GSC를 그대로 사용하고 있다(예: iOS 실기기에서 수집한 GSC를 Android 빌드에서 사용)  
\* Editor 스크립트로 GSC를 편집·저장할 때 메타데이터가 Editor의 Platform 설정으로 덮어써졌다

대처는 발생 케이스에 따라 다릅니다. 다른 플랫폼에서 수집한 GSC는 메타데이터를 고쳐 써도 내용물의 \`GraphicsState\`가 실행 환경과 일치하지 않으므로 해당 플랫폼에서 다시 수집합니다. Editor의 Platform 설정으로 덮어써진 경우에는 메타데이터를 실기기의 값으로 수정합니다. 또한 \`qualityLevelName\`의 불일치는 PSO에 영향을 주는 렌더링 설정(MSAA나 HDR 등)이나 셰이더가 Quality Level에 따라 변하지 않는 경우에 한해 무시할 수 있습니다. 그 경우에는 {{list:gsc_quality_fix}}처럼 워밍업 직전에 값을 고쳐 써서 경고를 피할 수 있습니다.

CODE START csharp | qualityLevelName 재작성으로 경고 회피 {{list:gsc_quality_fix}}  
// GSC 로드  
var gsc \= LoadGsc();  
// qualityLevelName을 실행 환경에 맞게 변경  
gsc.qualityLevelName \= QualitySettings.names\[QualitySettings.GetQualityLevel()\];  
// 웜업 실행  
CODE END

## **iOS(Metal)에서 AssetBundle 중복 패킹으로 인한 이중 인스턴스**

iOS(Metal)에서만 발생하는 케이스입니다. 워밍업 시 로그에 에러가 없고 Profiler의 Main Thread에서도 \`Shader.CreateGPUProgram\`는 발생하지 않는데, Render Thread에서는 \`GpuProgramMetal.GetCachedPipeline\`가 발생합니다.

원인은 AssetBundle 중복 패킹으로 인한 셰이더의 이중 인스턴스화입니다. 동일한 셰이더가 여러 AssetBundle에 중복되어 패키징되면 Unity는 메모리상에 별개의 셰이더 인스턴스를 생성합니다. 워밍업은 GSC가 참조하는 셰이더 인스턴스에 대해서만 PSO를 생성하므로, 같은 셰이더라도 별개 인스턴스가 된 셰이더는 워밍업되지 않습니다. 또한 Vulkan에서는 이 문제가 발생하지 않습니다. PSO 캐시 관리가 Metal과 Vulkan에서 다르기 때문으로 생각됩니다.

발생하는 구성의 예는 다음과 같습니다. 동일한 셰이더 X가 의존 관계로 인해 두 번들 모두에 패키징되어 있습니다.

{{enlargepage}}

CODE START text  
\- 에셋  
  \- 셰이더 X  
  \- 머티리얼 A(셰이더 X 사용)  
  \- 머티리얼 B(셰이더 X 사용)  
  \- GraphicsStateCollection

\- AssetBundle 구성  
  \- 번들 A  
    \- GraphicsStateCollection  
    \- 머티리얼 A(셰이더 X 사용)  
    \- 셰이더 X ← 의존 관계에 의한 패킹  
  \- 번들 B  
    \- 머티리얼 B(셰이더 X 사용)  
    \- 셰이더 X ← 의존 관계에 의한 패킹  
(셰이더 X에는 주소가 부여되지 않은 상태)  
CODE END

이 구성에서 번들 A와 번들 B를 로드하면 다음 흐름으로 머티리얼 B를 렌더링할 때 스터터가 발생합니다.

CODE START text  
\- 런타임 흐름  
  1\. 번들 A와 번들 B를 로드  
     \- 번들 A의 셰이더 X(instanceId=100)  
     \- 번들 B의 셰이더 X(instanceId=200)  
  2\. GraphicsStateCollection의 웜업을 실행  
     \- GSC가 참조하는 셰이더 X(instanceId=100)로 웜업이 수행된다  
  3\. 머티리얼 A를 렌더링  
     \- 셰이더 X(instanceId=100)의 PSO 캐시는 생성되어 있으므로 스터터가 발생하지 않는다  
  4\. 머티리얼 B를 렌더링  
     \- 셰이더 X(instanceId=200)의 PSO 캐시는 없으므로  
       Render Thread에서 PSO 생성이 실행되어 스터터가 발생한다  
CODE END

대처는 셰이더를 단일 번들(셰이더 전용 그룹)로 모으고 콘텐츠 번들로의 암시적 패키징을 배제하는 것입니다. 단일 번들로 집약하기 어려운 경우에는 번들마다 GSC를 준비합니다. 중복된 셰이더는 {{list:gsc_duplicate_check}} 같은 코드로 확인할 수 있습니다. Addressables를 사용 중이라면 \*\*Addressables Report\*\* 창에서도 셰이더 중복을 확인할 수 있습니다.

CODE START csharp | 셰이더 인스턴스 중복 확인 {{list:gsc_duplicate_check}}  
using System.Linq;

// AssetBundle 로드 후에 실행하여 인스턴스 중복을 확인한다  
var all \= Resources.FindObjectsOfTypeAll\<Shader\>();  
var duplicates \= all.GroupBy(s \=\> s.name).Where(g \=\> g.Count() \> 1);  
foreach (var g in duplicates)  
    foreach (var s in g)  
        Debug.Log($"Duplicate: {s.name} | InstanceID: {s.GetInstanceID()}");  
CODE END

## **iOS(Metal)에서 SubPass를 사용하는 셰이더의 워밍업 실패**

지금까지는 프로젝트 설정이나 번들 설계의 실수로 발생하는 문제였습니다. 다음으로 GSC의 사양상 발생하는 문제를 소개합니다.

하나의 렌더 패스를 여러 렌더링 단계로 분할하고, 이전 단계의 렌더링 결과를 메인 메모리에 다시 기록하지 않고 다음 단계에서 참조할 수 있게 하는 구조를 SubPass라고 부릅니다. 모바일 GPU에서는 이 일련의 단계를 칩 내부의 고속 메모리 영역(타일 메모리)에서 완결할 수 있어 메모리 대역폭을 절약할 수 있습니다. SubPass는 Metal·Vulkan에서 Native Render Pass로 실행됩니다. iOS(Metal)에서만 워밍업을 수행했고 로그에도 에러가 없는데 게임 플레이 중에 PSO 컴파일이 발생하는 경우가 있습니다. 원인은 iOS(Metal)의 워밍업 처리가 이 SubPass(Native Render Pass) 내에서 렌더링되는 셰이더에 대해 의도대로 동작하지 않는 것입니다. 이전 SubPass의 출력을 읽어 들이는 input attachments의 유무와 관계없이 발생합니다. 또한 워밍업의 내부 처리는 비공개 API이므로 여기서는 발생 조건 소개에 그칩니다.

영향을 받는 조건은 다음과 같습니다.

\* 타깃이 iOS / Metal(Apple A/M 칩 실기기)  
\* URP의 렌더링 처리 기반(RenderGraph) 설정인 \`RenderGraph.nativeRenderPassesEnabled\`가 활성화되어 있다(Unity 6000.5 이후에는 이 설정이 폐지되어 항상 활성화)  
\* SubPass(Native Render Pass) 내에서 렌더링하는 셰이더를 사용하고 있다

쓰기 전용 SubPass(URP Deferred의 GBuffer 패스 등)에서도, input attachments를 가진 SubPass(동 Deferred Lighting 패스 등)에서도 동일하게 발생합니다. 영향을 받는 SubPass의 기술 예로서 input attachments를 설정하는 케이스를 {{list:gsc_subpass_input}}에 나타냅니다.

CODE START csharp | SubPass에 input attachments를 설정하는 케이스 {{list:gsc_subpass_input}}  
// ScriptableRenderContext.BeginRenderPass로 시작한  
// 렌더 패스 내에서 SubPass를 설정하는 예  
var inputAttachments \= new NativeArray\<int\>(new\[\] { 0, 1 }, Allocator.Temp);  
context.BeginSubPass(  
    colors,  
    inputAttachments,   // 이전 SubPass의 출력 바인드  
                        // Metal에서는 framebuffer fetch로 변환되지만  
                        // GSC 웜업 시의 PSO 생성에는 반영되지 않는다  
    false  
);  
CODE END

Unity 6000.3에서는 근본적인 대처가 어려우므로 SubPass 사용을 재검토하거나 프리렌더링으로 플레이 중 스터터를 회피합니다. 프리렌더링은 대상 셰이더를 사용하는 오브젝트를 로딩 화면 중 등에 한 번 실제로 렌더링하여 PSO 캐시를 만들어 두는 기법입니다. 이 장의 수집 시나리오와 같은 순차 렌더링을 워밍업 대신 게임 내에서 수행하는 방식입니다.

## **스터터가 남는 경우의 진단 포인트**

지금까지 살펴본 것처럼 GSC를 사용한 셰이더 워밍업은 프로젝트의 에셋 관리 설계(Addressables 구성·셰이더 키워드 정의·번들 설계)와 밀접하게 관련되므로 도입 후 디버깅이 필요한 경우가 많습니다. 워밍업 시 로그 확인과 Profiler를 사용한 반복 계측을 통해 워밍업 누락이 없는지 확인합시다. 특히 다음 사항을 파악해 두면 진단이 빨라집니다.

\* 워밍업 시 로그에 워밍업 에러나 \`variant not found\`가 출력되지 않았는가  
\* Profiler에서 Main Thread의 \`Shader.CreateGPUProgram\`, Render Thread의 \`GpuProgramMetal.GetCachedPipeline\`(iOS) / \`CreateGraphicsPipelineImpl\`(Android)가 발생하지 않는가  
\* Shader Stripping 설정은 적절한가  
\* AssetBundle 패키징이 의도대로 되어 있는가

GSC에 기록된 베리언트를 확인하려면 다음 절에서 소개하는 베리언트 목록 출력({{list:gsc_variant_list}})을 이용할 수 있습니다.

# **GSC 파일 조작 예**

마지막으로 GSC 파일 조작 예를 두 가지 소개합니다. 모두 Editor 스크립트에서 실행하는 것을 전제로 하며, 수집한 GSC의 내용 확인이나 여러 차례 수집한 결과의 통합에 이용할 수 있습니다.

## **베리언트 목록과 State 수 확인**

{{list:gsc_variant_list}}은 GSC 파일을 로드하여 메타 정보와 기록된 베리언트 목록을 출력하는 예입니다.

CODE START csharp | 베리언트 목록과 State 수 확인 {{list:gsc_variant_list}}  
void PrintVariantInfo() {  
    // GSC 로드  
    var gsc \= new GraphicsStateCollection();  
    if (\!gsc.LoadFromFile(path))  
        return;

    // 베리언트 취득  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    gsc.GetVariants(variants);

    // GSC의 메타 정보를 출력  
    Debug.Log($"\[GSC\] Variants={variants.Count} TotalStates={gsc.totalGraphicsStateCount} "  
            \+ $"Platform={gsc.runtimePlatform} Device={gsc.graphicsDeviceType} Quality={gsc.qualityLevelName}");

    // 베리언트의 상세를 출력  
    foreach (var v in variants) {  
        int stateCount \= gsc.GetGraphicsStateCountForVariant(v.shader, v.passId, v.keywords);  
        string kws \= v.keywords.Length \> 0  
            ? string.Join(" ", v.keywords.Select(k \=\> k.name))  
            : "\<no keywords\>";  
        Debug.Log($"  {v.shader?.name} | states={stateCount} | {kws}");  
    }  
}  
CODE END

## **GSC 병합**

{{list:gsc_merge}}은 두 GSC를 병합하는 예입니다. 여러 번에 나누어 수집한 결과를 하나로 통합할 수 있습니다.

CODE START csharp | 두 GSC 병합 {{list:gsc_merge}}  
void MergeGsc() {  
    // 두 GSC를 로드  
    var src \= new GraphicsStateCollection();  
    var dst \= new GraphicsStateCollection();  
    if (\!src.LoadFromFile(srcPath) || \!dst.LoadFromFile(dstPath))  
        return;

    // 베리언트를 취득  
    var variants \= new List\<GraphicsStateCollection.ShaderVariant\>();  
    var states \= new List\<GraphicsStateCollection.GraphicsState\>();  
    src.GetVariants(variants);

    // src의 GSC에서 dst의 GSC로 추가  
    foreach (var v in variants) {  
        // 베리언트에 포함된 GraphicsState를 취득  
        src.GetGraphicsStatesForVariant(v.shader, v.passId, v.keywords, states);  
        foreach (var s in states) {  
            // State를 추가  
            dst.AddGraphicsStateForVariant(v.shader, v.passId, v.keywords, s);  
        }  
    }

    dst.SaveToFile(dstPath);  
}  
CODE END

AssetBundle 구성에 따라서는 수집한 \`GraphicsState\`를 다른 GSC로 이동·통합하는 대응이 필요한 경우가 있습니다. 이중 인스턴스 대처에서 소개한, 번들마다 GSC를 준비하는 경우가 대표적입니다. 그때는 이 절의 조작 예를 조합하여 활용하십시오.

[image1]: assets/amano-takuro-1/media/image1.png

[image2]: assets/amano-takuro-1/media/image2.png

[image3]: assets/amano-takuro-1/media/image3.png