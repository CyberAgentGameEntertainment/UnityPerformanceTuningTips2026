# **原稿**

# **Unity で活用する iOS 標準のパフォーマンス計測機能**

* 名前: 山崎 雅大 / Yamazaki Masahiro  
* 所属: 株式会社アプリボット  
* X: [https://x.com/TEST\_H\_](https://x.com/TEST_H_)  
* GitHub: [https://github.com/mao-test-h](https://github.com/mao-test-h)

# **はじめに**

Unity には Unity Profiler をはじめとした計測機能が備わっていますが、iOS 向けのアプリであれば、それに加えて OS 標準のパフォーマンス計測機能も活用できます。

Apple が提供するこれらの機能は Unity の Development Build フラグに依存せずに利用できるのが特徴です。  
例えば、計測処理をまだ組み込めていない段階や、デバッグ機能を無効にしたリリースビルドに近い構成でざっくりと負荷を確認するほか、テスト担当者の端末からトレース結果だけを受け取るといった使い方ができます。 こうして大まかな負荷を把握し、ボトルネックの候補をある程度絞り込んだ後は、Development Build を有効にしたビルドへ切り替え、Unity Profiler や Instruments で詳細に調査するといった使い分けが可能です。

本記事では、iOS が標準で提供する機能の中から以下の 2 つを紹介し、加えて macOS 27 / iOS 27 から追加された新機能についても解説します。

* Metal Performance HUD  
* Performance Trace

また、これらの機能を Unity から扱えるようにした OSS 兼サンプルリポジトリとして「MetalPerfKit-Unity」{{fn:metalperfkit_unity}} を公開しているので、実装の詳細についてはこちらも併せてご覧ください。

※ 以降の解説では表記を iOS に統一しますが、特に断りがない限り iPadOS も含みます。

## **検証環境**

本記事は以下の環境で動作検証を行っています。

* Unity 6000.0.81f1  
* Xcode 26.6 / 27.0 beta  
* iOS 26.6 / 27.0 beta  
* macOS 27.0 beta

※ 執筆時点では iOS 27、macOS 27、Xcode 27、後述する \`StateReporter\` API はベータ段階です。本記事は 2026 年 8 月時点の情報であり、正式リリースまでに API やコマンド、設定画面、挙動などが変わる可能性があります。

# **Metal Performance HUD**

Metal Performance HUD は、Metal を使用するアプリケーションのパフォーマンス情報をリアルタイムに画面上へオーバーレイ表示したり、ログとして出力したりできるツールです。 Apple のドキュメント {{fn:hud_monitoring_doc}} では、現在の状態を素早く把握し、Xcode や Instruments で詳しくキャプチャする範囲を見つけるための機能として紹介されています。

この機能自体は Metal 4 以前 (iOS 16 世代) から存在していましたが、2025 年の WWDC で発表された Metal 4 (iOS 26 世代) に合わせて大きく強化されました。 Metal 4 以前と Metal 4 以降の表示の違いを {{fig:perf_hud_legacy}} と {{fig:perf_hud_metal4}} に示します。

![][image1]{scale=0.5}

図: Metal 4 以前の Metal Performance HUD {{fig:perf_hud_legacy}}

![][image2]{scale=0.5}

図: Metal 4 以降の Metal Performance HUD {{fig:perf_hud_metal4}}

Metal 4 からの主な強化点は計測項目の拡充のほか、表示位置や不透明度・スケールのカスタマイズ、これらを API から動的に制御できる項目の増加などが挙げられます。

主要なメトリクスを表示した状態の例を {{fig:perf_hud_metrics_all}} に示します。

![][image3]{scale=1.0}

図: 主要なメトリクスを表示した状態の例 {{fig:perf_hud_metrics_all}}

各メトリクスの詳細については、公式ドキュメント「Understanding the Metal Performance HUD metrics」{{fn:hud_metrics_doc}} をご覧ください。

また、筆者も過去に Metal Performance HUD についての解説記事 {{fn:qiita_gpu_measurement}} や技術同人誌「UniTips Vol.12」{{fn:unitips_vol12}} を公開しています。 基本的な使い方から Unity 向けの活用術などを解説しているので、詳細についてはこちらも参考にしてみてください。

以降のセクションでは、Metal 4 (iOS 26 世代) の内容をベースに解説していきます。

## **HUD を開くには**

HUD を表示する方法はいくつかありますが、ここでは「設定アプリから開く方法」と「API で表示を制御する方法」の 2 つを解説します。

### **設定アプリから開く**

iOS デバイスの設定アプリから有効化できます。手順は以下の通りです。

※ 前提として、\`Developer\` メニューが表示されている必要があります。また、OS バージョンによっては、メニュー表記が異なる可能性があります。

1. iOS の「設定」アプリを開く  
2. \`Developer\` メニューを開き、その中にある \`Metal HUD\` を開く ({{fig:settings_developer_menu}}を参照)

![][image4]{scale=0.4}

図: Developer メニューから Metal HUD を開く {{fig:settings_developer_menu}}

\`Metal HUD\` を開くと {{fig:settings_metal_hud}} の画面が表示されるので、この中の \`Show Metal Performance HUD\` を有効化し、対象アプリを再起動すると表示できます。

この画面では、表示する指標の切り替え、不透明度・スケール・表示位置の変更なども行えます。 また、同じセクションにある \`Log Metal Performance\` を有効にすると、パフォーマンス情報を一定間隔でログとして出力できます。詳細は後述の「ログ機能について」で解説します。

![][image5]{scale=0.4}

図: Metal HUD の設定画面 {{fig:settings_metal_hud}}

### **API で表示を制御する**

アプリの実行中に HUD の表示を動的に制御したい場合は、\`CAMetalLayer\` クラスの \`developerHUDProperties\` プロパティに設定値を渡します。 {{list:hud_properties_basic}} では HUD とログ出力を有効化し、表示位置を左上に指定しています。

CODE START swift | developerHUDProperties による HUD の表示制御 {{list:hud_properties_basic}}

myMetalLayer.developerHUDProperties \= \[  
   "mode": "default",      // HUD の ON/OFF  
   "logging": "default",   // ログ機能の ON/OFF  
   "positionX": 0,         // 表示位置 (ピクセル単位の X 座標)  
   "positionY": 0,         // 表示位置 (ピクセル単位の Y 座標)  
\]

CODE END

HUD を非表示にする場合は \`mode\` に \`disabled\` を指定します。 従来はこのプロパティで制御できるのは HUD とログ機能の ON/OFF までであり、位置調整などは行えませんでしたが、Metal 4 からは \`positionX\` / \`positionY\` によるピクセル単位の位置指定が行えるようになりました。

また、この Dictionary には HUD がサポートする環境変数のキーも渡すことができます。 例えば \`MTL\_HUD\_OPACITY\` と \`MTL\_HUD\_SCALE\` を渡すと、不透明度とスケールを 0.0〜1.0 の範囲で調整できます（{{list:hud_properties_env}}）。

CODE START swift | 環境変数キーによる不透明度・スケールの調整 {{list:hud_properties_env}}

myMetalLayer.developerHUDProperties \= \[  
   ...,  
   "MTL\_HUD\_OPACITY": opacity,     // 不透明度 (0.0〜1.0)  
   "MTL\_HUD\_SCALE": scale,         // スケール (0.0〜1.0)  
\]

CODE END

他にも設定可能な項目が多数あるので、詳細については公式ドキュメント「Customizing the Metal Performance HUD」{{fn:hud_customizing_doc}} をご覧ください。

Unity から呼び出す場合は、描画に使われている \`CAMetalLayer\` を iOS ネイティブプラグインで取得し、C\# 側から P/Invoke で呼び出す形になります。 \`CAMetalLayer\` は \`UnityFramework\` 経由で {{list:get_metal_layer}} のように取得できます。

CODE START swift | UnityFramework 経由での CAMetalLayer の取得 {{list:get_metal_layer}}

private func getMetalLayer() \-\> CAMetalLayer? {  
   guard let instance \= UnityFramework.getInstance(),  
         let rootView \= instance.appController().rootView,  
         let metalLayer \= rootView.layer as? CAMetalLayer else {  
       return nil  
   }  
   return metalLayer  
}

CODE END

MetalPerfKit-Unity ではこれらの実装を \`PerformanceHUDSwitcher\` クラスとして公開しており、C\# からは {{list:hud_switcher_csharp}} のように呼び出せます。 なお、ネイティブ API がピクセル単位で位置を指定するのに対し、\`PerformanceHUDSwitcher\` では解像度に依存せず扱えるよう座標を 0.0〜1.0 の相対位置で受け取り、内部で描画領域のサイズに応じたピクセル座標へ変換しています。

CODE START csharp | PerformanceHUDSwitcher の呼び出し例 {{list:hud_switcher_csharp}}

// HUD を画面右上 (x=1.0, y=0.0) に表示する  
// (座標は 0.0〜1.0 の相対位置で指定)  
PerformanceHUDSwitcher.SetPerformanceHUDVisible(true, 1.0f, 0.0f);

CODE END

完全な実装例については、MetalPerfKit-Unity のソースコードをご覧ください。

## **ログ機能について**

前述の \`Log Metal Performance\` や API の \`logging\` を有効にすると、アプリの実行中に 1 秒間隔でパフォーマンス情報がログとして出力されます。 ログは \`metal-HUD:\` から始まる {{list:hud_log_format}} のフォーマットで出力され、Xcode のコンソールや macOS の Console.app から確認できます。

CODE START text | metal-HUD ログの出力フォーマット {{list:hud_log_format}}

metal-HUD: \<first-frame-number-integer\>,\<graphics-memory-usage-float\>,\<process-memory-usage-float\>,\<first-frame-present-interval-float\>,\<first-frame-gpu-time-float\>,...\<last-frame-present-interval-float\>,\<last-frame-gpu-time-float\>

CODE END

先頭から順に「サンプリングを開始したフレーム番号」「グラフィックスメモリ使用量」「プロセス全体のメモリ使用量」が記録され、それ以降は、サンプリングされた各フレームの Frame Present Interval と GPU Time のペアが繰り返し記録されます。 実際の出力例を {{fig:perf_hud_log_sample}} に示します。

![][image6]{scale=1.0}

図: Metal Performance HUD のログ出力例 {{fig:perf_hud_log_sample}}

カンマ区切りのデータとして集計すれば、フレーム間隔の乱れや GPU Time、メモリ使用量の推移を後から分析できます。 また、これらのログが Unified Logging に残っている範囲であれば、\`OSLogStore\` API を用いて「過去 N 秒分のログを取得してファイルに保存する」といった実装も可能です。 MetalPerfKit-Unity では \`PerformanceLogger\` クラスとしてこの機能を提供しています（{{list:fetch_performance_logs}}）。

CODE START csharp | PerformanceLogger によるログの取得と保存 {{list:fetch_performance_logs}}

// 過去 60 秒分のログを取得してファイルに保存する  
PerformanceLogger.FetchPerformanceLogs(60, FileUtility.GenerateFetchLoggingFilePath());

CODE END

# **Performance Trace**

Performance Trace は iOS 26 から追加された機能で、Mac に接続することなく、iOS デバイス単体でパフォーマンスに関するデータを収集できます。 収集したトレースファイルは Mac に転送し、Instruments や後述する \`metalperftrace\` コマンドで分析できます。

デバイス単体で完結するため、長時間のプレイなど Mac に接続した状態では再現しにくい状況を記録できるほか、テスト担当者など他人の端末で収集したトレースファイルを共有してもらい、手元で分析するといった使い方も可能です。

本章では、その中でも WWDC 2026 で機能強化とともに解説された Lookback Collection をメインに紹介します。

## **使い方**

Performance Trace は \`Developer\` メニューから有効化する必要があります。 手順は以下の通りです。

1. iOS の「設定」アプリを開く  
2. \`Developer\` メニューを開き、その中にある \`Performance Trace\` を開く  
3. \`Enable Performance Trace\` を有効化 (有効/無効の切り替えにはデバイスの再起動が必要)  
4. 任意の \`Tracing Mode\` を選択 (詳しくは後述)

\`Developer\` メニュー内の \`Performance Trace\` と、その設定画面をそれぞれ {{fig:trace_developer_menu}} と {{fig:trace_enable}} に示します。

![][image7]{scale=0.4}

図: Developer メニュー内の Performance Trace {{fig:trace_developer_menu}}

![][image8]{scale=0.4}

図: Enable Performance Trace の設定画面 {{fig:trace_enable}}

有効化が完了したら、次にトレースの開始・停止やトレースデータの収集などを行うために、コントロールセンターに \`Performance Trace\` を追加します。 こちらはコントロールセンターを開き、\`Developer\` セクションから追加することができます。追加手順と追加後の状態を {{fig:trace_add_control}} と {{fig:trace_control}} に示します。

![][image9]{scale=0.4}

図: コントロールセンターへの Performance Trace の追加 {{fig:trace_add_control}}

![][image10]{scale=0.4}

図: Performance Trace コントロールを追加した状態 {{fig:trace_control}}

## **Tracing Mode について**

\`Tracing Mode\` には {{table:tracing_modes}} に示す 4 種類 (System Activity は Summary / Detailed の 2 種類) があり、コントロールセンターの \`Performance Trace\` コントロールを長押しすることでも切り替えられます。

表: Tracing Mode の種類 {{table:tracing_modes}}

| モード | 概要 |
| :---- | :---- |
| Power Profiler | 電力消費への影響を記録する |
| System Activity (Summary / Detailed) | システムの動作状況を記録する |
| Lookback Collection | システムが常時記録しているデータを過去に遡って収集する |

Power Profiler と System Activity はコントロールセンターの \`Performance Trace\` コントロールをタップして記録を開始し、再度タップして停止します。 一方、Lookback Collection は記録操作が不要で、タップした時点から過去に遡ってデータを収集します。詳細は後述します。

収集されたトレースファイルは Performance Trace 画面にある \`Available Trace Files\` に一覧表示され、共有ボタンから AirDrop などで Mac や他の開発者へ転送できます。 共有されたアーカイブを展開すると、Instruments で開ける \`.atrc\` ファイルが得られます。 なお、トレースファイルはデバイスのストレージ残量が少なくなると自動的に削除されるほか、スワイプ操作で手動削除も可能です。

各モードについての補足は以下の通りです。

### **Power Profiler**

Power Profiler は、アプリによる CPU / GPU / ディスプレイ / ネットワークといった各サブシステムの利用が、デバイスの電力消費に与える影響 (Power Impact) を計測するモードです。 本記事では詳細を割愛しますが、公式ドキュメント「Measuring your app’s power use with Power Profiler」{{fn:power_profiler_doc}} と WWDC 2025 のセッション「Profile and optimize power usage in your app」{{fn:wwdc2025_power}} で詳しく解説されています。

### **System Activity**

System Activity は、「Summary」と「Detailed」の 2 種類が用意されているトレースモードです。  
筆者が確認した範囲では、執筆時点 (2026 年 8 月) において、Apple のドキュメントや WWDC セッションに System Activity の詳しい仕様や、Summary と Detailed の違いに関する説明は見当たりませんでした。 そのため、本記事では各モードの詳細には踏み込まず、実際のトレースで確認できた情報のみを紹介します。

参考として、iOS 27 を搭載したデバイスで System Activity (Summary) のトレースを記録し、後述する \`metalperftrace\` コマンドで分析した結果を {{list:overview_system_activity}} に示します。

CODE START text | System Activity (Summary) トレースの分析結果 {{list:overview_system_activity}}

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

このトレースでは、プロセスごとのメモリ使用量や CPU 時間、ディスク I/O に加えて、\`CAMetalLayer\` 単位の FPS や Frame Time、CPU Begin-to-Present、On-GPU Time などの統計を確認できました。 なお、\`\[States\]\` の項目にはアプリの状態遷移が記録されていますが、こちらは次章で解説する \`StateReporter\` API によるものなので、詳しくは後述します。

### **Lookback Collection**

Lookback Collection は、システムが常時記録しているパフォーマンスデータを、後から過去に遡って収集するモードです。  
筆者が確認した限り、Lookback Collection は iOS 26 の時点ですでに \`Tracing Mode\` の選択肢として存在していましたが、WWDC 2025 や公式ドキュメントでは解説されていませんでした。その後、WWDC 2026 で機能が強化され、その内容を解説するセッション動画「Find and fix performance issues in your Metal games」{{fn:wwdc2026_metal_games}} が公開されました。

講演によると、ゲームの実行中は、システムがパフォーマンスメトリクス (CPU、GPU、FPS、メモリなど) を常時記録し、そのデータを数日分保存しています。 Lookback Collection はこの仕組みを利用しているため、テスターによる長時間のプレイセッションであっても、遡ってデータを取得できるのが特徴です。 この仕組みの概要を {{fig:wwdc_lookback_metrics}} に示します。

![][image11]{scale=1.0}

図: システムによるパフォーマンスメトリクスの常時記録（WWDC 2026 セッションより） {{fig:wwdc_lookback_metrics}}

使い方は、通常通りプレイテストを行った後にコントロールセンターの \`Performance Trace\` コントロールをタップするだけです。 タップした時点から設定した時間分だけ遡ってトレースの収集・処理が始まり、完了すると通知が届いて \`Available Trace Files\` に追加されます。

実際に iOS 26 を搭載したデバイスで約 3 時間分の Lookback Collection トレースを収集し、Instruments で開いた例を {{fig:lookback_process_list}} に示します。 遡った期間中に起動していた複数のアプリ (プロセス) が一覧表示され、それぞれの FPS などの推移を俯瞰できます。

![][image12]{scale=1.0}

図: Lookback Collection のトレースを Instruments で開いた例 {{fig:lookback_process_list}}

さらにプロセスを展開すると、{{fig:lookback_frame_metrics}} のように \`CAMetalLayer\` 単位で FPS や GPU Usage %、Frame Interval、CPU / GPU 時間といったフレームメトリクスの推移を確認できます。

![][image13]{scale=1.0}

図: CAMetalLayer 単位のフレームメトリクスの推移 {{fig:lookback_frame_metrics}}

※ 手元の iOS 27 環境では、集計中に \`Error processing Performance Trace\` と表示され、正常に動作しませんでした。こちらの不具合については、iOS / iPadOS 27 の正式リリース時点でも、リリースノートの \`Control Center -> Known Issues\` に、「Power Profiler や Lookback のトレースを収集すると、トレースが生成されない場合がある」という既知の問題 (184729556) が記載されています。{{fn:ios_ipados_27_release_notes}}

# **macOS 27 / iOS 27 から追加された新機能**

前章で紹介した通り、ゲームの実行中はシステムが Metal パフォーマンスメトリクスを常時記録・保存しています。 WWDC 2026 ではこの仕組みを軸とした新機能として、トレースにアプリ固有の情報を付与する \`StateReporter\` API と、トレースを CLI から分析できる \`metalperftrace\` コマンドが発表されました。

## **StateReporter API**

トレースデータでフレームレートが落ちた時刻を特定できても、Metal パフォーマンスメトリクスだけでは、そのときゲームが何をしていたかまでは分かりません。 「ゲームのどのシーンを開いているのか」「オブジェクト数はどれくらいか」「グラフィックス設定は何か」といったアプリ固有の情報は、Metal パフォーマンスメトリクスには自動的に含まれないためです。

iOS 27 で追加された \`StateReporting\` フレームワーク {{fn:statereporting_doc}} の \`StateReporter\` API {{fn:statereporter_doc}} を用いると、アプリの状態をシステムに報告し、トレースデータにアプリ固有のコンテキストを付与できます。

報告した状態は、\`metalperftrace\` では状態別の集計に、Instruments では Points of Interest の表示に利用されます。 前章で掲載した System Activity のトレースを Instruments で開いた例を {{fig:instruments_state_reporter}} に示します。報告した状態が Points of Interest のトラックとして表示されています。

![][image14]{scale=1.0}

図: 報告した状態が Points of Interest として表示される例 {{fig:instruments_state_reporter}}

\`StateReporter\` は {{table:state_reporter_elements}} に示す 4 つの要素で構成されます。

表: StateReporter を構成する 4 つの要素 {{table:state_reporter_elements}}

| 要素 | 概要 | 例 |
| :---- | :---- | :---- |
| Domain | 報告対象とする機能領域。逆 DNS 形式の文字列で、ドメインごとに同時にアクティブな状態は 1 つ | \`com.mygame.level\` |
| Label | 現在の状態を示す短い文字列 | \`Level 1\` |
| Stable metadata | 状態の存続期間中は不変のメタデータ。Label と組み合わせて状態を一意に識別する | \`{id: 1001}\` |
| Volatile metadata | 状態の中で変化しうる揮発性のメタデータ。更新しても状態遷移は発生しない | \`{health: 20}\` |

ドメインは複数定義でき、例えば「レベル進行」「グラフィックス設定」「ネットワーク状態」をそれぞれ独立したドメインとして報告し、組み合わせて分析できます。

Swift からは {{list:state_reporter_swift}} のように利用します。

CODE START swift | Swift からの StateReporter の利用例 {{list:state_reporter_swift}}

import StateReporting  
// ドメインに対応する Reporter を取得  
let reporter \= StateReporter.reporter(for: "com.mygame.gameplay")  
// 状態遷移を報告  
reporter.reportTransition(to: "Playing")  
// 状態を終了する場合は nil を渡す  
reporter.reportTransition(to: nil)

CODE END

### **Unity から利用する**

\`StateReporting\` は Swift / Objective-C 向けの API であるため、Unity から利用するにはネイティブプラグインが必要です。 MetalPerfKit-Unity ではこれをラップした \`StateReporter\` クラスをプレビュー機能として公開しており、C\# から {{list:state_reporter_csharp}} のように利用できます。

CODE START csharp | C\# からの StateReporter の利用例 {{list:state_reporter_csharp}}

using MetalPerfKit;  
// ドメインに対応する Reporter を取得  
var reporter \= StateReporter.ReporterForDomain($"{Application.identifier}.screen");  
// 状態遷移を報告 (揮発性メタデータ付き)  
reporter.ReportTransitionToStateLabel(  
   "Main",  
   volatileMetadata: new Dictionary\<string, StateReporterMetadataValue\>  
   {  
       \["CubeCount"\] \= cubeCount,  
   });  
// 状態は維持したまま、揮発性メタデータのみを更新  
reporter.ReportVolatileMetadataUpdate(  
   new Dictionary\<string, StateReporterMetadataValue\>  
   {  
       \["CubeCount"\] \= cubeCount,  
   });

CODE END

また、リポジトリの \`Assets/\_Example/2\_MetalPerformanceMetrics\` には、画面遷移 (Title / Main / Settings) とグラフィックス品質 (URP の 3 段階) をそれぞれ独立したドメインとして報告するサンプルを用意しています。 前章で掲載した System Activity のトレース内の \`\[States\]\` は、このサンプルで記録したものです。

## **metalperftrace**

\`metalperftrace\` は macOS 27 から追加された CLI ツールで、トレースファイルの分析のほか、macOS 向けアプリのパフォーマンスメトリクスの収集を行えます。 収集機能は macOS 上で動作するアプリが対象となるため本記事では扱いませんが、iOS 端末で収集したトレースファイルの分析にはこちらのツールを活用できます。

### **トレースファイルの分析について**

トレースファイルの分析には \`overview\` サブコマンドを使用します（{{list:metalperftrace_overview}}）。

CODE START bash | overview サブコマンドによるトレースファイルの分析 {{list:metalperftrace_overview}}

\# 収集済みのトレースファイルを分析  
metalperftrace overview MyGameTrace.atrc

CODE END

出力内容は前章で紹介した通りです。 さらに \`--include-state-transitions\` を渡すと、\`StateReporter\` で記録した状態遷移の詳細も確認できます。 {{list:overview_state_transitions}} は前述のサンプルアプリのトレースを分析した例で、各グラフィックス品質への変更履歴が、その状態での滞在時間と割合とともに出力されています。

CODE START text | 状態遷移の詳細を含めた分析結果 {{list:overview_state_transitions}}

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

※ 出力は一部を抜粋・整形して掲載しています。実際には同じ状態も時系列に沿って繰り返し出力され、それぞれに開始・終了のタイムスタンプが付きます。

また、\`--aggregate\` を渡すと、状態ごとのメトリクス集計が行えます。\`--domain com.mygame.graphics\` を指定すると特定のドメインに絞り込め、さらに \`--state-label "High"\` を併用すると特定の状態まで絞り込めます。 「グラフィックス設定が High のときだけ平均 FPS が落ちている」といった、アプリの状態とパフォーマンスの相関をコマンド 1 つで確認できます。

また、これらの出力は \`--json\` を渡すことで JSON 形式でも取得できます。 講演でも「構造化された出力はスクリプトによるリグレッションテストや、AI エージェントによるパフォーマンス問題の自動トリアージに活用できる」と案内されています。Apple の「Metal Developer Tools」ページ {{fn:metal_developer_tools}} でも、「Hello, agentic debugging」と題して、エージェントと組み合わせたワークフローが紹介されています。

脚注: {{fn:metalperfkit_unity}} MetalPerfKit-Unity [https://github.com/mao-test-h/MetalPerfKit-Unity](https://github.com/mao-test-h/MetalPerfKit-Unity)

脚注: {{fn:hud_monitoring_doc}} Monitoring your Metal app’s graphics performance [https://developer.apple.com/documentation/xcode/monitoring-your-metal-apps-graphics-performance](https://developer.apple.com/documentation/xcode/monitoring-your-metal-apps-graphics-performance)

脚注: {{fn:hud_metrics_doc}} Understanding the Metal Performance HUD metrics [https://developer.apple.com/documentation/xcode/understanding-metal-performance-hud-metrics](https://developer.apple.com/documentation/xcode/understanding-metal-performance-hud-metrics)

脚注: {{fn:qiita_gpu_measurement}} 【Unity】モバイルアプリ上で GPU 負荷を計測する [https://qiita.com/mao\_/items/3b531d17cacb86036b7f](https://qiita.com/mao_/items/3b531d17cacb86036b7f)

脚注: {{fn:unitips_vol12}} UniTips Vol.12（「第2章: Metal 4 から強化された Metal Performance HUD の Unity 活用術」にて解説） [https://techbookfest.org/product/8mHpXz8LJCKAsLxENJKwaz?productVariantID=ewK5GtNTjkz4k8heM4BrxU](https://techbookfest.org/product/8mHpXz8LJCKAsLxENJKwaz?productVariantID=ewK5GtNTjkz4k8heM4BrxU)

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