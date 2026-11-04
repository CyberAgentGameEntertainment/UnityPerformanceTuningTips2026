# **원고**

# **Unity에서 활용하는 iOS 표준 퍼포먼스 계측 기능**

* 이름: Yamazaki Masahiro  
* 소속: 주식회사 Applibot  
* X: [https://x.com/TEST\_H\_](https://x.com/TEST_H_)  
* GitHub: [https://github.com/mao-test-h](https://github.com/mao-test-h)

# **들어가며**

Unity에는 Unity Profiler를 비롯한 계측 기능이 갖춰져 있지만, iOS용 앱이라면 이에 더해 OS 표준 퍼포먼스 계측 기능도 활용할 수 있습니다.

Apple이 제공하는 이러한 기능은 Unity의 Development Build 플래그에 의존하지 않고 사용할 수 있다는 점이 특징입니다.  
예를 들어 계측 처리를 아직 넣지 못한 단계나 디버그 기능을 비활성화한 릴리스 빌드에 가까운 구성에서 대략적인 부하를 확인하거나, 테스트 담당자의 기기에서 트레이스 결과만 전달받는 식으로 사용할 수 있습니다. 이렇게 대략적인 부하를 파악해 병목 후보를 어느 정도 좁힌 뒤에는, Development Build를 활성화한 빌드로 전환하여 Unity Profiler나 Instruments로 상세히 조사하는 식으로 구분하여 사용할 수 있습니다.

이 글에서는 iOS가 표준으로 제공하는 기능 중 다음 2가지를 소개하고, 더불어 macOS 27 / iOS 27에서 추가된 새로운 기능도 설명합니다.

* Metal Performance HUD  
* Performance Trace

또한 이러한 기능을 Unity에서 다룰 수 있도록 한 OSS 겸 샘플 리포지토리로 「MetalPerfKit-Unity」{{fn:metalperfkit_unity}}를 공개하고 있으므로, 구현의 상세 내용은 이쪽도 함께 참고하시기 바랍니다.

※ 이후 설명에서는 표기를 iOS로 통일하지만, 별도로 명시하지 않는 한 iPadOS도 포함합니다.

## **검증 환경**

이 글은 다음 환경에서 동작을 검증했습니다.

* Unity 6000.0.81f1  
* Xcode 26.6 / 27.0 beta  
* iOS 26.6 / 27.0 beta  
* macOS 27.0 beta

※ 집필 시점에서 iOS 27, macOS 27, Xcode 27, 후술하는 \`StateReporter\` API는 베타 단계입니다. 이 글은 2026년 8월 시점의 정보이며, 정식 릴리스까지 API, 명령어, 설정 화면, 동작 등이 바뀔 수 있습니다.

# **Metal Performance HUD**

Metal Performance HUD는 Metal을 사용하는 애플리케이션의 퍼포먼스 정보를 실시간으로 화면에 오버레이 표시하거나 로그로 출력할 수 있는 도구입니다. Apple 문서 {{fn:hud_monitoring_doc}}에서는 현재 상태를 빠르게 파악하고, Xcode나 Instruments로 자세히 캡처할 범위를 찾기 위한 기능으로 소개하고 있습니다.

이 기능 자체는 Metal 4 이전(iOS 16 세대)부터 존재했지만, 2025년 WWDC에서 발표된 Metal 4(iOS 26 세대)에 맞춰 크게 강화되었습니다. Metal 4 이전과 Metal 4 이후의 표시 차이를 {{fig:perf_hud_legacy}}과 {{fig:perf_hud_metal4}}에 나타냅니다.

![][image1]{scale=0.5}

図: Metal 4 이전의 Metal Performance HUD {{fig:perf_hud_legacy}}

![][image2]{scale=0.5}

図: Metal 4 이후의 Metal Performance HUD {{fig:perf_hud_metal4}}

Metal 4의 주요 강화점으로는 계측 항목 확충 외에도 표시 위치, 불투명도, 스케일의 커스터마이즈와 이를 API로 동적으로 제어할 수 있는 항목의 증가 등을 들 수 있습니다.

주요 메트릭을 표시한 상태의 예를 {{fig:perf_hud_metrics_all}}에 나타냅니다.

![][image3]{scale=1.0}

図: 주요 메트릭을 표시한 상태의 예 {{fig:perf_hud_metrics_all}}

각 메트릭의 상세 내용은 공식 문서 「Understanding the Metal Performance HUD metrics」{{fn:hud_metrics_doc}}를 참고하시기 바랍니다.

또한 필자는 과거에 Metal Performance HUD에 관한 해설 글 {{fn:qiita_gpu_measurement}}과 기술 동인지 「UniTips Vol.12」{{fn:unitips_vol12}}도 공개했습니다. 기본적인 사용법부터 Unity용 활용법까지 설명하고 있으므로, 자세한 내용은 이쪽도 참고하시기 바랍니다.

이후 섹션에서는 Metal 4(iOS 26 세대)의 내용을 기준으로 설명합니다.

## **HUD를 여는 방법**

HUD를 표시하는 방법은 여러 가지가 있지만, 여기서는 「설정 앱에서 여는 방법」과 「API로 표시를 제어하는 방법」 2가지를 설명합니다.

### **설정 앱에서 열기**

iOS 기기의 설정 앱에서 활성화할 수 있습니다. 절차는 다음과 같습니다.

※ 전제로 \`Developer\` 메뉴가 표시되어 있어야 합니다. 또한 OS 버전에 따라 메뉴 표기가 다를 수 있습니다.

1. iOS의 「설정」 앱을 엽니다  
2. \`Developer\` 메뉴를 열고, 그 안에 있는 \`Metal HUD\`을 엽니다({{fig:settings_developer_menu}} 참조)

![][image4]{scale=0.4}

図: Developer 메뉴에서 Metal HUD 열기 {{fig:settings_developer_menu}}

\`Metal HUD\`을 열면 {{fig:settings_metal_hud}} 화면이 표시되므로, 이 안의 \`Show Metal Performance HUD\`를 활성화하고 대상 앱을 재시작하면 표시할 수 있습니다.

이 화면에서는 표시할 지표 전환, 불투명도, 스케일, 표시 위치 변경 등도 할 수 있습니다. 또한 같은 섹션에 있는 \`Log Metal Performance\`을 활성화하면 퍼포먼스 정보를 일정 간격으로 로그로 출력할 수 있습니다. 자세한 내용은 후술하는 「로그 기능」에서 설명합니다.

![][image5]{scale=0.4}

図: Metal HUD 설정 화면 {{fig:settings_metal_hud}}

### **API로 표시 제어하기**

앱 실행 중에 HUD 표시를 동적으로 제어하려면 \`CAMetalLayer\` 클래스의 \`developerHUDProperties\` 프로퍼티에 설정값을 전달합니다. {{list:hud_properties_basic}}에서는 HUD와 로그 출력을 활성화하고 표시 위치를 왼쪽 위로 지정했습니다.

CODE START swift | developerHUDProperties에 의한 HUD 표시 제어 {{list:hud_properties_basic}}

myMetalLayer.developerHUDProperties \= \[  
   "mode": "default",      // HUD ON/OFF  
   "logging": "default",   // 로그 기능 ON/OFF  
   "positionX": 0,         // 표시 위치(픽셀 단위의 X 좌표)  
   "positionY": 0,         // 표시 위치(픽셀 단위의 Y 좌표)  
\]

CODE END

HUD를 숨기려면 \`mode\`에 \`disabled\`을 지정합니다. 기존에는 이 프로퍼티로 제어할 수 있는 것이 HUD와 로그 기능의 ON/OFF까지였고 위치 조정 등은 할 수 없었지만, Metal 4부터는 \`positionX\` / \`positionY\`으로 픽셀 단위 위치 지정이 가능해졌습니다.

또한 이 Dictionary에는 HUD가 지원하는 환경 변수 키도 전달할 수 있습니다. 예를 들어 \`MTL\_HUD\_OPACITY\`과 \`MTL\_HUD\_SCALE\`을 전달하면 불투명도와 스케일을 0.0~1.0 범위로 조정할 수 있습니다({{list:hud_properties_env}}).

CODE START swift | 환경 변수 키에 의한 불투명도·스케일 조정 {{list:hud_properties_env}}

myMetalLayer.developerHUDProperties \= \[  
   ...,  
   "MTL\_HUD\_OPACITY": opacity,     // 불투명도(0.0~1.0)  
   "MTL\_HUD\_SCALE": scale,         // 스케일(0.0~1.0)  
\]

CODE END

이 밖에도 설정 가능한 항목이 많으므로, 자세한 내용은 공식 문서 「Customizing the Metal Performance HUD」{{fn:hud_customizing_doc}}를 참고하시기 바랍니다.

Unity에서 호출하는 경우에는 렌더링에 사용되는 \`CAMetalLayer\`을 iOS 네이티브 플러그인으로 가져와 C\# 쪽에서 P/Invoke로 호출하는 형태가 됩니다. \`CAMetalLayer\`은 \`UnityFramework\`를 통해 {{list:get_metal_layer}}처럼 가져올 수 있습니다.

CODE START swift | UnityFramework를 통한 CAMetalLayer 가져오기 {{list:get_metal_layer}}

private func getMetalLayer() \-\> CAMetalLayer? {  
   guard let instance \= UnityFramework.getInstance(),  
         let rootView \= instance.appController().rootView,  
         let metalLayer \= rootView.layer as? CAMetalLayer else {  
       return nil  
   }  
   return metalLayer  
}

CODE END

MetalPerfKit-Unity에서는 이러한 구현을 \`PerformanceHUDSwitcher\` 클래스로 공개하고 있으며, C\#에서는 {{list:hud_switcher_csharp}}처럼 호출할 수 있습니다. 한편 네이티브 API가 픽셀 단위로 위치를 지정하는 것과 달리, \`PerformanceHUDSwitcher\`에서는 해상도에 의존하지 않고 다룰 수 있도록 좌표를 0.0~1.0의 상대 위치로 받아 내부에서 렌더링 영역 크기에 맞는 픽셀 좌표로 변환합니다.

CODE START csharp | PerformanceHUDSwitcher 호출 예 {{list:hud_switcher_csharp}}

// HUD를 화면 오른쪽 위(x=1.0, y=0.0)에 표시한다  
// (좌표는 0.0~1.0의 상대 위치로 지정)  
PerformanceHUDSwitcher.SetPerformanceHUDVisible(true, 1.0f, 0.0f);

CODE END

전체 구현 예는 MetalPerfKit-Unity의 소스 코드를 참고하시기 바랍니다.

## **로그 기능**

앞서 설명한 \`Log Metal Performance\`이나 API의 \`logging\`을 활성화하면 앱 실행 중에 1초 간격으로 퍼포먼스 정보가 로그로 출력됩니다. 로그는 \`metal-HUD:\`로 시작하는 {{list:hud_log_format}} 형식으로 출력되며, Xcode 콘솔이나 macOS의 Console.app에서 확인할 수 있습니다.

CODE START text | metal-HUD 로그의 출력 형식 {{list:hud_log_format}}

metal-HUD: \<first-frame-number-integer\>,\<graphics-memory-usage-float\>,\<process-memory-usage-float\>,\<first-frame-present-interval-float\>,\<first-frame-gpu-time-float\>,...\<last-frame-present-interval-float\>,\<last-frame-gpu-time-float\>

CODE END

앞에서부터 차례로 「샘플링을 시작한 프레임 번호」「그래픽스 메모리 사용량」「프로세스 전체의 메모리 사용량」이 기록되고, 그 이후에는 샘플링된 각 프레임의 Frame Present Interval과 GPU Time 쌍이 반복해서 기록됩니다. 실제 출력 예를 {{fig:perf_hud_log_sample}}에 나타냅니다.

![][image6]{scale=1.0}

図: Metal Performance HUD 로그 출력 예 {{fig:perf_hud_log_sample}}

쉼표로 구분된 데이터로 집계하면 프레임 간격의 흐트러짐, GPU Time, 메모리 사용량의 추이를 나중에 분석할 수 있습니다. 또한 이러한 로그가 Unified Logging에 남아 있는 범위라면 \`OSLogStore\` API를 사용해 「과거 N초 분량의 로그를 가져와 파일로 저장」하는 구현도 가능합니다. MetalPerfKit-Unity에서는 \`PerformanceLogger\` 클래스로 이 기능을 제공합니다({{list:fetch_performance_logs}}).

CODE START csharp | PerformanceLogger에 의한 로그 가져오기와 저장 {{list:fetch_performance_logs}}

// 최근 60초 분량의 로그를 가져와 파일에 저장한다  
PerformanceLogger.FetchPerformanceLogs(60, FileUtility.GenerateFetchLoggingFilePath());

CODE END

# **Performance Trace**

Performance Trace는 iOS 26에서 추가된 기능으로, Mac에 연결하지 않고 iOS 기기 단독으로 퍼포먼스 관련 데이터를 수집할 수 있습니다. 수집한 트레이스 파일은 Mac으로 전송하여 Instruments나 후술하는 \`metalperftrace\` 명령어로 분석할 수 있습니다.

기기 단독으로 완결되므로 장시간 플레이처럼 Mac에 연결한 상태에서는 재현하기 어려운 상황을 기록할 수 있고, 테스트 담당자 등 다른 사람의 기기에서 수집한 트레이스 파일을 공유받아 로컬에서 분석하는 식의 사용도 가능합니다.

이 장에서는 그중에서도 WWDC 2026에서 기능 강화와 함께 소개된 Lookback Collection을 중심으로 소개합니다.

## **사용법**

Performance Trace는 \`Developer\` 메뉴에서 활성화해야 합니다. 절차는 다음과 같습니다.

1. iOS의 「설정」 앱을 엽니다  
2. \`Developer\` 메뉴를 열고, 그 안에 있는 \`Performance Trace\`을 엽니다  
3. \`Enable Performance Trace\`을 활성화합니다(활성/비활성 전환에는 기기 재시작이 필요합니다)  
4. 원하는 \`Tracing Mode\`을 선택합니다(자세한 내용은 후술)

\`Developer\` 메뉴 내의 \`Performance Trace\`과 그 설정 화면을 각각 {{fig:trace_developer_menu}}와 {{fig:trace_enable}}에 나타냅니다.

![][image7]{scale=0.4}

図: Developer 메뉴 내의 Performance Trace {{fig:trace_developer_menu}}

![][image8]{scale=0.4}

図: Enable Performance Trace 설정 화면 {{fig:trace_enable}}

활성화가 끝나면 다음으로 트레이스 시작·중지와 트레이스 데이터 수집 등을 하기 위해 제어 센터에 \`Performance Trace\`을 추가합니다. 제어 센터를 열고 \`Developer\` 섹션에서 추가할 수 있습니다. 추가 절차와 추가 후의 상태를 {{fig:trace_add_control}}와 {{fig:trace_control}}에 나타냅니다.

![][image9]{scale=0.4}

図: 제어 센터에 Performance Trace 추가 {{fig:trace_add_control}}

![][image10]{scale=0.4}

図: Performance Trace 컨트롤을 추가한 상태 {{fig:trace_control}}

## **Tracing Mode**

\`Tracing Mode\`에는 {{table:tracing_modes}}에 나타낸 4종류(System Activity는 Summary / Detailed의 2종류)가 있으며, 제어 센터의 \`Performance Trace\` 컨트롤을 길게 눌러서도 전환할 수 있습니다.

表: Tracing Mode의 종류 {{table:tracing_modes}}

| 모드 | 개요 |
| :---- | :---- |
| Power Profiler | 전력 소비에 미치는 영향을 기록한다 |
| System Activity (Summary / Detailed) | 시스템의 동작 상황을 기록한다 |
| Lookback Collection | 시스템이 상시 기록하고 있는 데이터를 과거로 거슬러 올라가 수집한다 |

Power Profiler와 System Activity는 제어 센터의 \`Performance Trace\` 컨트롤을 탭하여 기록을 시작하고, 다시 탭하여 중지합니다. 한편 Lookback Collection은 기록 조작이 필요 없으며, 탭한 시점부터 과거로 거슬러 올라가 데이터를 수집합니다. 자세한 내용은 후술합니다.

수집된 트레이스 파일은 Performance Trace 화면의 \`Available Trace Files\`에 목록으로 표시되며, 공유 버튼으로 AirDrop 등을 통해 Mac이나 다른 개발자에게 전송할 수 있습니다. 공유받은 아카이브를 풀면 Instruments에서 열 수 있는 \`.atrc\` 파일을 얻을 수 있습니다. 또한 트레이스 파일은 기기의 저장 공간이 부족해지면 자동으로 삭제되며, 스와이프 조작으로 수동 삭제도 가능합니다.

각 모드에 대한 보충 설명은 다음과 같습니다.

### **Power Profiler**

Power Profiler는 앱의 CPU / GPU / 디스플레이 / 네트워크 등 각 서브시스템 사용이 기기의 전력 소비에 미치는 영향(Power Impact)을 계측하는 모드입니다. 이 글에서는 상세 내용을 생략하지만, 공식 문서 「Measuring your app’s power use with Power Profiler」{{fn:power_profiler_doc}}와 WWDC 2025 세션 「Profile and optimize power usage in your app」{{fn:wwdc2025_power}}에서 자세히 설명하고 있습니다.

### **System Activity**

System Activity는 「Summary」와 「Detailed」 2종류가 준비된 트레이스 모드입니다.  
필자가 확인한 범위에서는 집필 시점(2026년 8월)에 Apple 문서나 WWDC 세션에서 System Activity의 자세한 사양이나 Summary와 Detailed의 차이에 관한 설명을 찾지 못했습니다. 따라서 이 글에서는 각 모드의 상세 내용에는 들어가지 않고, 실제 트레이스에서 확인한 정보만 소개합니다.

참고로 iOS 27이 탑재된 기기에서 System Activity(Summary) 트레이스를 기록하고 후술하는 \`metalperftrace\` 명령어로 분석한 결과를 {{list:overview_system_activity}}에 나타냅니다.

CODE START text | System Activity(Summary) 트레이스 분석 결과 {{list:overview_system_activity}}

$ metalperftrace overview system\_activity\_summary\_ios27.atrc  
\[MetalPerfKit-Unity pid:1974\]  
   Mem: 799.0 MiB (799.0 Peak, 616.8 Metal, 2577.0 Headroom)  
   Total CPU Time: 12.679s (20.47% Sys, 79.53% User)  
   Instructions: 43.33B (60.88% P, 39.12% E)  
   Cycles: 21.04B (38.97% P, 61.03% E)  
   Disk Reads / Writes: 9.13 / 0.08 MiB (Logical Write 0.07)  
   \[States\]  
     com.DefaultCompany.MetalPerfKit-Unity.graphics: 3 transitions, last known state: URP-HighFidelity  
     com.DefaultCompany.MetalPerfKit-Unity.screen: 11 transitions, last known state: Title  
 UnityView (UnityDefaultViewController) (1290x2796)  Interval 30.037s  Active 30.037s  
   44.5 FPS  1202 Frames  1202 Detached  11 Skipped  
   Frame Time           avg:  22.47ms  min:  16.67  max: 420.93  stddev:  30.68  p95:  50.01  p99: 133.36  
   CPU Begin-to-Present avg:  17.24ms  min:   1.14  max: 794.63  stddev:  31.76  p95:  31.28  p99: 111.73  
   On-GPU Time          avg:  15.50ms  min:   1.81  max: 424.87  stddev:  37.24  p95:  51.17  p99: 134.48  
   Next Drawable Wait   avg:  14.53ms  min:   0.00  max: 401.43  stddev:  32.11

CODE END

이 트레이스에서는 프로세스별 메모리 사용량, CPU 시간, 디스크 I/O에 더해 \`CAMetalLayer\` 단위의 FPS, Frame Time, CPU Begin-to-Present, On-GPU Time 등의 통계를 확인할 수 있었습니다. 한편 \`\[States\]\` 항목에는 앱의 상태 전환이 기록되어 있는데, 이는 다음 장에서 설명하는 \`StateReporter\` API에 의한 것이므로 자세한 내용은 후술합니다.

### **Lookback Collection**

Lookback Collection은 시스템이 상시 기록하고 있는 퍼포먼스 데이터를 나중에 과거로 거슬러 올라가 수집하는 모드입니다.  
필자가 확인한 바로는 Lookback Collection은 iOS 26 시점에 이미 \`Tracing Mode\`의 선택지로 존재했지만, WWDC 2025나 공식 문서에서는 설명하지 않았습니다. 이후 WWDC 2026에서 기능이 강화되었고, 그 내용을 설명하는 세션 영상 「Find and fix performance issues in your Metal games」{{fn:wwdc2026_metal_games}}가 공개되었습니다.

강연에 따르면 게임 실행 중에 시스템이 퍼포먼스 메트릭(CPU, GPU, FPS, 메모리 등)을 상시 기록하고, 그 데이터를 며칠 분량 저장합니다. Lookback Collection은 이 구조를 이용하므로 테스터의 장시간 플레이 세션이라도 거슬러 올라가 데이터를 가져올 수 있다는 점이 특징입니다. 이 구조의 개요를 {{fig:wwdc_lookback_metrics}}에 나타냅니다.

![][image11]{scale=1.0}

図: 시스템에 의한 퍼포먼스 메트릭 상시 기록(WWDC 2026 세션에서) {{fig:wwdc_lookback_metrics}}

사용법은 평소처럼 플레이 테스트를 한 뒤 제어 센터의 \`Performance Trace\` 컨트롤을 탭하기만 하면 됩니다. 탭한 시점부터 설정한 시간만큼 거슬러 올라가 트레이스 수집·처리가 시작되고, 완료되면 알림이 도착하여 \`Available Trace Files\`에 추가됩니다.

실제로 iOS 26이 탑재된 기기에서 약 3시간 분량의 Lookback Collection 트레이스를 수집하여 Instruments에서 연 예를 {{fig:lookback_process_list}}에 나타냅니다. 거슬러 올라간 기간 중에 실행되었던 여러 앱(프로세스)이 목록으로 표시되며, 각각의 FPS 등의 추이를 한눈에 볼 수 있습니다.

![][image12]{scale=1.0}

図: Lookback Collection 트레이스를 Instruments에서 연 예 {{fig:lookback_process_list}}

프로세스를 더 펼치면 {{fig:lookback_frame_metrics}}와 같이 \`CAMetalLayer\` 단위로 FPS, GPU Usage %, Frame Interval, CPU / GPU 시간 같은 프레임 메트릭의 추이를 확인할 수 있습니다.

![][image13]{scale=1.0}

図: CAMetalLayer 단위의 프레임 메트릭 추이 {{fig:lookback_frame_metrics}}

※ 필자의 iOS 27 환경에서는 집계 중에 \`Error processing Performance Trace\`가 표시되며 정상적으로 동작하지 않았습니다. 이 문제에 대해서는 iOS / iPadOS 27 정식 릴리스 시점에도 릴리스 노트의 \`Control Center -> Known Issues\`에 'Power Profiler나 Lookback의 트레이스를 수집하면 트레이스가 생성되지 않을 수 있다'는 알려진 문제(184729556)가 기재되어 있습니다. {{fn:ios_ipados_27_release_notes}}

# **macOS 27 / iOS 27에서 추가된 새로운 기능**

앞 장에서 소개한 것처럼 게임 실행 중에 시스템은 Metal 퍼포먼스 메트릭을 상시 기록·저장합니다. WWDC 2026에서는 이 구조를 축으로 한 새로운 기능으로, 트레이스에 앱 고유의 정보를 부여하는 \`StateReporter\` API와 트레이스를 CLI에서 분석할 수 있는 \`metalperftrace\` 명령어가 발표되었습니다.

## **StateReporter API**

트레이스 데이터에서 프레임 레이트가 떨어진 시각을 특정할 수 있더라도, Metal 퍼포먼스 메트릭만으로는 그때 게임이 무엇을 하고 있었는지까지는 알 수 없습니다. 「게임의 어느 씬을 열고 있는가」「오브젝트 수는 어느 정도인가」「그래픽스 설정은 무엇인가」와 같은 앱 고유의 정보는 Metal 퍼포먼스 메트릭에 자동으로 포함되지 않기 때문입니다.

iOS 27에서 추가된 \`StateReporting\` 프레임워크 {{fn:statereporting_doc}}의 \`StateReporter\` API {{fn:statereporter_doc}}를 사용하면 앱의 상태를 시스템에 보고하여 트레이스 데이터에 앱 고유의 컨텍스트를 부여할 수 있습니다.

보고한 상태는 \`metalperftrace\`에서는 상태별 집계에, Instruments에서는 Points of Interest 표시에 사용됩니다. 앞 장에서 실은 System Activity 트레이스를 Instruments에서 연 예를 {{fig:instruments_state_reporter}}에 나타냅니다. 보고한 상태가 Points of Interest 트랙으로 표시되어 있습니다.

![][image14]{scale=1.0}

図: 보고한 상태가 Points of Interest로 표시되는 예 {{fig:instruments_state_reporter}}

\`StateReporter\`는 {{table:state_reporter_elements}}에 나타낸 4개 요소로 구성됩니다.

表: StateReporter를 구성하는 4개 요소 {{table:state_reporter_elements}}

| 요소 | 개요 | 예 |
| :---- | :---- | :---- |
| Domain | 보고 대상으로 하는 기능 영역. 역 DNS 형식의 문자열이며, 도메인마다 동시에 활성화되는 상태는 1개 | \`com.mygame.level\` |
| Label | 현재 상태를 나타내는 짧은 문자열 | \`Level 1\` |
| Stable metadata | 상태가 유지되는 동안 변하지 않는 메타데이터. Label과 조합하여 상태를 고유하게 식별한다 | \`{id: 1001}\` |
| Volatile metadata | 상태 안에서 변할 수 있는 휘발성 메타데이터. 갱신해도 상태 전환은 발생하지 않는다 | \`{health: 20}\` |

도메인은 여러 개 정의할 수 있으며, 예를 들어 「레벨 진행」「그래픽스 설정」「네트워크 상태」를 각각 독립된 도메인으로 보고하고 조합하여 분석할 수 있습니다.

Swift에서는 {{list:state_reporter_swift}}처럼 사용합니다.

CODE START swift | Swift에서의 StateReporter 사용 예 {{list:state_reporter_swift}}

import StateReporting  
// 도메인에 대응하는 Reporter를 가져온다  
let reporter \= StateReporter.reporter(for: "com.mygame.gameplay")  
// 상태 전이를 보고한다  
reporter.reportTransition(to: "Playing")  
// 상태를 종료하려면 nil을 전달한다  
reporter.reportTransition(to: nil)

CODE END

### **Unity에서 사용하기**

\`StateReporting\`는 Swift / Objective-C용 API이므로 Unity에서 사용하려면 네이티브 플러그인이 필요합니다. MetalPerfKit-Unity에서는 이를 래핑한 \`StateReporter\` 클래스를 프리뷰 기능으로 공개하고 있으며, C\#에서 {{list:state_reporter_csharp}}처럼 사용할 수 있습니다.

CODE START csharp | C\#에서의 StateReporter 사용 예 {{list:state_reporter_csharp}}

using MetalPerfKit;  
// 도메인에 대응하는 Reporter를 가져온다  
var reporter \= StateReporter.ReporterForDomain($"{Application.identifier}.screen");  
// 상태 전이를 보고한다(휘발성 메타데이터 포함)  
reporter.ReportTransitionToStateLabel(  
   "Main",  
   volatileMetadata: new Dictionary\<string, StateReporterMetadataValue\>  
   {  
       \["CubeCount"\] \= cubeCount,  
   });  
// 상태는 유지한 채 휘발성 메타데이터만 갱신한다  
reporter.ReportVolatileMetadataUpdate(  
   new Dictionary\<string, StateReporterMetadataValue\>  
   {  
       \["CubeCount"\] \= cubeCount,  
   });

CODE END

또한 리포지토리의 \`Assets/\_Example/2\_MetalPerformanceMetrics\`에는 화면 전환(Title / Main / Settings)과 그래픽스 품질(URP 3단계)을 각각 독립된 도메인으로 보고하는 샘플을 마련해 두었습니다. 앞 장에서 실은 System Activity 트레이스 내의 \`\[States\]\`은 이 샘플로 기록한 것입니다.

## **metalperftrace**

\`metalperftrace\`는 macOS 27에서 추가된 CLI 도구로, 트레이스 파일 분석 외에 macOS용 앱의 퍼포먼스 메트릭 수집도 할 수 있습니다. 수집 기능은 macOS에서 동작하는 앱이 대상이므로 이 글에서는 다루지 않지만, iOS 기기에서 수집한 트레이스 파일의 분석에는 이 도구를 활용할 수 있습니다.

### **트레이스 파일 분석**

트레이스 파일 분석에는 \`overview\` 서브커맨드를 사용합니다({{list:metalperftrace_overview}}).

CODE START bash | overview 서브커맨드에 의한 트레이스 파일 분석 {{list:metalperftrace_overview}}

\# 수집된 트레이스 파일을 분석한다  
metalperftrace overview MyGameTrace.atrc

CODE END

출력 내용은 앞 장에서 소개한 대로입니다. 여기에 \`--include-state-transitions\`를 전달하면 \`StateReporter\`로 기록한 상태 전환의 상세 내용도 확인할 수 있습니다. {{list:overview_state_transitions}}는 앞서 언급한 샘플 앱의 트레이스를 분석한 예로, 각 그래픽스 품질로의 변경 이력이 해당 상태에서의 체류 시간 및 비율과 함께 출력되어 있습니다.

CODE START text | 상태 전환의 상세 내용을 포함한 분석 결과 {{list:overview_state_transitions}}

$ metalperftrace overview \--include-state-transitions system\_activity\_summary\_ios27.atrc  
\[MetalPerfKit-Unity pid:1974\]  
   ...  
   \[States\]  
     com.DefaultCompany.MetalPerfKit-Unity.graphics  
       URP-Balanced  (32.75%, 9.904s)  
         (2026-08-16T14:22:33.025Z → 2026-08-16T14:22:42.930Z)  
       URP-Performant  (29.43%, 8.900s)  
         ...  
       URP-HighFidelity  (37.82%, 11.437s)  
         ...  
     com.DefaultCompany.MetalPerfKit-Unity.screen  
       Title  (5.07%, 1.535s)  
         ...  
       Main  (21.09%, 6.379s)  
         ...  
       Title  (2.22%, 0.671s)  
         ...  
       Settings  (6.28%, 1.899s)  
         ...

CODE END

※ 출력은 일부를 발췌·정리하여 실었습니다. 실제로는 같은 상태도 시계열에 따라 반복해서 출력되며, 각각에 시작·종료 타임스탬프가 붙습니다.

또한 \`--aggregate\`를 전달하면 상태별 메트릭 집계를 할 수 있습니다. \`--domain com.mygame.graphics\`을 지정하면 특정 도메인으로 좁힐 수 있고, \`--state-label "High"\`를 함께 사용하면 특정 상태까지 좁힐 수 있습니다. 「그래픽스 설정이 High일 때만 평균 FPS가 떨어진다」와 같은 앱 상태와 퍼포먼스의 상관관계를 명령어 하나로 확인할 수 있습니다.

또한 이러한 출력은 \`--json\`를 전달하여 JSON 형식으로도 얻을 수 있습니다. 강연에서도 「구조화된 출력은 스크립트에 의한 리그레션 테스트나 AI 에이전트에 의한 퍼포먼스 문제의 자동 트리아지에 활용할 수 있다」고 안내했습니다. Apple의 「Metal Developer Tools」 페이지 {{fn:metal_developer_tools}}에서도 「Hello, agentic debugging」이라는 제목으로 에이전트와 조합한 워크플로를 소개하고 있습니다.

脚注: {{fn:metalperfkit_unity}} MetalPerfKit-Unity [https://github.com/mao-test-h/MetalPerfKit-Unity](https://github.com/mao-test-h/MetalPerfKit-Unity)

脚注: {{fn:hud_monitoring_doc}} Monitoring your Metal app’s graphics performance [https://developer.apple.com/documentation/xcode/monitoring-your-metal-apps-graphics-performance](https://developer.apple.com/documentation/xcode/monitoring-your-metal-apps-graphics-performance)

脚注: {{fn:hud_metrics_doc}} Understanding the Metal Performance HUD metrics [https://developer.apple.com/documentation/xcode/understanding-metal-performance-hud-metrics](https://developer.apple.com/documentation/xcode/understanding-metal-performance-hud-metrics)

脚注: {{fn:qiita_gpu_measurement}} 【Unity】모바일 앱에서 GPU 부하를 계측하기 [https://qiita.com/mao\_/items/3b531d17cacb86036b7f](https://qiita.com/mao_/items/3b531d17cacb86036b7f)

脚注: {{fn:unitips_vol12}} UniTips Vol.12(「제2장: Metal 4에서 강화된 Metal Performance HUD의 Unity 활용법」에서 설명) [https://techbookfest.org/product/8mHpXz8LJCKAsLxENJKwaz?productVariantID=ewK5GtNTjkz4k8heM4BrxU](https://techbookfest.org/product/8mHpXz8LJCKAsLxENJKwaz?productVariantID=ewK5GtNTjkz4k8heM4BrxU)

脚注: {{fn:hud_customizing_doc}} Customizing the Metal Performance HUD [https://developer.apple.com/documentation/xcode/customizing-metal-performance-hud](https://developer.apple.com/documentation/xcode/customizing-metal-performance-hud)

脚注: {{fn:power_profiler_doc}} Measuring your app’s power use with Power Profiler [https://developer.apple.com/documentation/xcode/measuring-your-app-s-power-use-with-power-profiler](https://developer.apple.com/documentation/xcode/measuring-your-app-s-power-use-with-power-profiler)

脚注: {{fn:wwdc2025_power}} WWDC 2025 \- Profile and optimize power usage in your app [https://developer.apple.com/videos/play/wwdc2025/226/](https://developer.apple.com/videos/play/wwdc2025/226/)

脚注: {{fn:wwdc2026_metal_games}} WWDC 2026 \- Find and fix performance issues in your Metal games [https://developer.apple.com/videos/play/wwdc2026/388/](https://developer.apple.com/videos/play/wwdc2026/388/)

脚注: {{fn:ios_ipados_27_release_notes}} iOS & iPadOS 27 Release Notes [https://developer.apple.com/documentation/ios-ipados-release-notes/ios-ipados-27-release-notes](https://developer.apple.com/documentation/ios-ipados-release-notes/ios-ipados-27-release-notes)

脚注: {{fn:statereporting_doc}} Getting started with StateReporting [https://developer.apple.com/documentation/StateReporting/getting-started-with-statereporting](https://developer.apple.com/documentation/StateReporting/getting-started-with-statereporting)

脚注: {{fn:statereporter_doc}} StateReporter [https://developer.apple.com/documentation/statereporting/statereporter](https://developer.apple.com/documentation/statereporting/statereporter)

脚注: {{fn:metal_developer_tools}} Metal Developer Tools [https://developer.apple.com/metal/tools/](https://developer.apple.com/metal/tools/)

[image1]: assets/yamazaki-masahiro-1/media/image1.jpg

[image2]: assets/yamazaki-masahiro-1/media/image2.jpg

[image3]: assets/yamazaki-masahiro-1/media/image3.jpg

[image4]: assets/yamazaki-masahiro-1/media/image4.jpg

[image5]: assets/yamazaki-masahiro-1/media/image5.jpg

[image6]: assets/yamazaki-masahiro-1/media/image6.png

[image7]: assets/yamazaki-masahiro-1/media/image7.jpg

[image8]: assets/yamazaki-masahiro-1/media/image8.jpg

[image9]: assets/yamazaki-masahiro-1/media/image9.jpg

[image10]: assets/yamazaki-masahiro-1/media/image10.jpg

[image11]: assets/yamazaki-masahiro-1/media/image11.png

[image12]: assets/yamazaki-masahiro-1/media/image12.png

[image13]: assets/yamazaki-masahiro-1/media/image13.png

[image14]: assets/yamazaki-masahiro-1/media/image14.png