# **기기에 따른 렌더링 설정 전환**

* NAOKI SAKUMA / 주식회사 사이버에이전트 SGE 코어 기술본부

스마트폰 게임은 다양한 기기에서 플레이될 것을 전제로 합니다.  
모든 기기에 동일한 렌더링 설정을 적용하면 저사양 기기에서 크래시, 프레임 드롭, 발열이 발생하는 경우가 있습니다.  
반대로 고사양 기기에서는 기기의 성능을 활용해 더 높은 품질을 구현할 수 있습니다.  
이에 기기에 맞는 적절한 렌더링 설정으로 전환하는 구조를 소개합니다.

# **전환 방법**

게임에서는 저·중·고와 같은 프리셋 방식과 사용자가 값을 직접 조작하는 방식, 이 두 가지가 흔히 쓰입니다.

## **프리셋 방식**

프리셋 단위로 전환하고자 할 때의 방법입니다.  
사용자가 프리셋을 선택하게 하는 경우나 기기에서 자동 판정하는 경우에도 이 방법을 사용합니다.

ProjectSettings의 Quality에서 Add Quality Level을 눌러 프리셋 수만큼 추가합니다.  
이후 각 프리셋에 맞는 RenderPipelineAsset을 어태치합니다.  
필요에 따라 Quality의 다른 항목(Rendering, Textures 등)을 설정할 수도 있습니다.  
![][image1]  
코드에서 QualitySettings.SetQualityLevel을 호출하면 원하는 QualityLevel로 전환할 수 있습니다.

## **값을 직접 조작하는 방식**

사용자가 파라미터를 조작하게 하는 경우에는 이 방법을 사용합니다.  
다음과 같은 코드로 파라미터를 직접 조작할 수 있습니다.

CODE START csharp  
var urpAsset \= (UniversalRenderPipelineAsset)GraphicsSettings.currentRenderPipeline;  
// 해상도 변경  
urpAsset.renderScale \= value;  
// 섀도 캐스케이드 수 변경  
urpAsset.shadowCascadeCount \= value;  
// 그 밖에 섀도 해상도, 추가 라이트 수 등  
CODE END

# **기기에 따른 전환**

기기에 따라 프리셋을 권장하거나 자동으로 설정하고 싶은 경우도 있을 것입니다.  
이때는 SystemInfo 타입이 유용합니다.

특히 deviceModel, graphicsDeviceName, systemMemorySize를 보면 식별하기 쉬운 값을 얻을 수 있습니다.  
다음은 반환되는 값의 예입니다.  
表: 기기별 SystemInfo 반환값의 예{{table:device_system_info}}

| 기종 | deviceModel | graphicsDeviceName | systemMemorySize |
| :---- | :---- | :---- | :---- |
| iPhone 13 | iPhone14,5 | Apple A15 GPU | 약 3800 |
| iPad Pro 11-inch (3세대) | iPad13,4 | Apple M1 GPU | 약 7800 |
| Galaxy S23 | samsung SM-S911B | Adreno (TM) 740 | 약 7500 |
| Pixel 8 | Google Pixel 8 | Mali-G715 | 약 7400 |

iOS는 deviceModel과 graphicsDeviceName의 명명 규칙이 일관되며, 기본적으로 같은 계열 내에서는 A 또는 M 뒤의 숫자가 클수록 사양이 높습니다.  
따라서 둘을 조합하면 어느 정도 구분할 수 있습니다.

Android는 기종마다 반환되는 값이 제각각이어서 iOS처럼 일률적으로 비교하기 어렵습니다.  
따라서  
\* RAM 용량으로 단순하게 구분한다  
\* 마스터 데이터로 분리하여 비교한다  
\* 시작 시 벤치마크 도구를 실행한다

등의 방법으로 구분하는 것이 좋습니다.

# **자체 제작 RendererFeature**

자체 RendererFeature 등을 만들 때도 렌더링 설정을 전환할 수 있는 API를 준비해 두면 편리합니다.  
예를 들어

*  임시 텍스처의 포맷이나 해상도를 낮춘다  
*  Blit 횟수를 줄인다  
*  더 단순한 셰이더로 교체한다

등의 대응을 들 수 있습니다.

[image1]: assets/sakuma-naoki-1/media/image1.png