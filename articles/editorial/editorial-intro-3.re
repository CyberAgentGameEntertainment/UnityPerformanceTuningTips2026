=[nonum] 2026年のアセット・設定・ビルド動向

パフォーマンスチューニングは、自分たちのコードやアセットを直すことだけではありません。同じコードとアセットでも、エンジンのバージョンや設定ひとつで、ビルド時間、アプリ容量、メモリ使用量、実行速度は変わります。エンジン側ですでに解決された問題を自前の仕組みで抱え続けていないか、新しく追加された設定を知らないまま既定値で使っていないか。それを確かめるためにも、エンジン自体の変化を追っておくことはチューニングの前提になります。

Unityにとっても、パフォーマンスはエンジンとして優先度の高い領域です。Unity 6世代の各リリースのWhat's Newには、新機能と並んで、ビルド時間の短縮、プレイヤーの容量削減、メモリ管理の改善が毎回挙がっています。第3部で扱うアセット・設定・ビルドは、その変化がとくに大きい領域です。

執筆時点の2026年9月では、長期サポート版の最新はUnity 6.3 LTS、正式版の最新はUnity 6.6です。Unity 6.7はLTSとなる予定のバージョンとしてベータ版が公開されており@<fn>{fn1}、Unity 7は2026年12月に初期のベータ版、2027年第1四半期に正式版のリリースが予定されています@<fn>{fn2}。以降では、Unity 6.0から6.7までの変化をリリースの順にたどり、同じバージョンの中では効果の大きいものから取り上げます。第3部の各章で詳しく扱う変化は要点だけにとどめ、該当する章を示します。なお、本文で触れるUnityのバージョンや既定値は執筆時点のものであり、以降のリリースで変わる可能性があることをご了承ください。

==[nonum] Unity 6.0

=== iOSのメモリ上限をAPIで設定する

Increased Memory LimitとExtended Virtual Addressingは、アプリが使えるメモリの上限そのものに関わるiOSのエンタイトルメントです。以前のUnityでも、書き出したXcodeプロジェクトのEntitlementsファイルを手で編集するか、ビルド後処理で書き換えれば設定できました。Unity 6000.0.11f1以降では @<tt>{ProjectCapabilityManager} に専用のAPIが追加され、Entitlementsへの追加とXcodeプロジェクトへの反映をまとめて行えます。それぞれの役割と実機での上限の確認方法は、第3部6章「iOSのメモリ上限と仮想アドレス空間」で扱います。

=== Build Profilesで構成ごとに設定を持つ

Unity 6.0で導入されたBuild Profilesは、1つのプラットフォームに対して複数のビルド構成を持てる仕組みです。導入時点から、プロファイルごとにシーンの一覧、Player Settingsの上書き、スクリプトのDefine Symbolsの追加を持てます。Unity 6.1からはGraphics設定とQuality設定をプロファイルごとに上書きできるようになり@<fn>{fn3}、6.3では上書きする項目を選んで追加する形になりました。6.5では @<tt>{BuildProfile.CreateBuildProfile} でスクリプトからプロファイルを作成することもできます。

パフォーマンスの面での利点は、プラットフォームや配信先ごとの設定の違いを、ビルド前に設定を書き換えるスクリプトではなく、プロファイルとして固定しておける点です。たとえばDefine Symbolsを分ければ、デバッグ用の計測やログのコードをリリース用のプロファイルからだけ除外でき、Quality設定を上書きすれば、ローエンド向けの構成ではビルドに含める品質レベルそのものを絞り込めます。第3部3章「端末に応じた描画設定の切り替え」は、実行時に端末の性能に合わせて描画負荷を振り分ける設計を、Quality Levelを切り替える形式とRender Pipeline Assetの値を直接書き換える形式で比べます。Build Profilesは、それをビルドの時点で切り替える手段として組み合わせられます。

=== Managed Stripping LevelのLowが非推奨に

Unity 6.0のマニュアルでは、Managed Stripping LevelのLowが将来の廃止対象とされ、MinimalかMediumを使うよう案内されています@<fn>{fn4}。Lowを使っていたプロジェクトでは、Mediumへ上げてバイナリを小さくするか、Minimalへ下げるかを選ぶことになります。Mediumへ上げるとリフレクションで参照されるコードが削られやすくなり、その対処は第3部4章「Unity Managed Code Stripping 実践ガイド」で扱います。

==[nonum] Unity 6.1

=== 端末ごとにGraphics Jobsのモードを選ぶ

Unity 6.1で追加されたVulkan Device Filter Assetでは、Androidの端末ごとにGraphics Jobsのモードを指定できます。特定のGPUやドライバーでだけ遅くなる、あるいは不安定になる設定を、ほかの端末の設定を変えずに避けられます。

==[nonum] Unity 6.2

=== インポート時に自動生成されるMesh LOD

Unity 6.2で追加されたMesh LODは、モデルのインポート設定でGenerate Mesh LODsを有効にすると、インポート時にLODを自動生成する機能です@<fn>{fn5}。LODを用意していなかったモデルでも、設定ひとつで遠景のポリゴン数を減らし、GPU負荷を下げられます。すでにLOD Groupを使っている場合、ポリゴンを減らす効果は変わらず、差が出るのはメモリです。Mesh LODは頂点を追加せず、すべてのLODが元のメッシュの頂点バッファを共有して、インデックスバッファだけが増えます。LODごとにメッシュ全体とGameObjectを持つLOD Groupより、メモリの増加を小さく抑えられます。

ただし、自動生成されるLODの品質は元のメッシュの形状に左右され、手作業で作ったLODと同じ品質は期待できません。公式ドキュメントでも、生物や岩のような有機的な形状には向く一方、曲率の高い形状や離れたパーツの多いメッシュでは、テクスチャのずれや、樹木の葉が消えるといった崩れが起きる例が示されています。生成されたLODはInspectorで段階ごとに確認し、崩れる段階からはLimit LODsで生成を止めるか、そのメッシュだけLOD Groupに戻すといった判断が必要です。

また、減らせるのはポリゴンだけで、マテリアルは変わりません。Particle System、VFX Graph、Static Batchingでは常にLOD0が使われるため効果がなく、Skinned Mesh Rendererでは、どのLODが選ばれていても変形の計算はLOD0に対して行われます。LODのクロスフェードにはGPU Resident Drawerが必要で、LOD Groupとの併用は推奨されていません。

==[nonum] Unity 6.3

=== シェーダーバリアントを設定で減らす

シェーダーバリアントはキーワードの組み合わせごとにコンパイルされるため、キーワードが増えるほど掛け算で増えていきます。バリアントを減らせば、ビルド時間、ビルド容量、シェーダーの読み込み時間、実行時のメモリがまとめて減ります。

Unity 6.3で追加されたShader Build Settingsでは、シェーダーのソースを書き換えずに、キーワードの種類を @<tt>{shader_feature} や @<tt>{dynamic_branch} に上書きしたり、特定のキーワードのコードをビルドから除外したりできます@<fn>{fn6}。この設定はBuild Profileごとに上書きできるため、プラットフォームごとに不要なバリアントだけを削れます。ビルトインのキーワードを除外すると必要なバリアントまで消えることがあるため、欠落を検出できるStrict Shader Variant Matchingとの併用も欠かせません。ビルドに残ったバリアントの実行時メモリは、2021 LTSから使えるShader Variant Loadingの設定で、読み込むチャンクのサイズと数を制限して抑えられます@<fn>{fn7}。

==[nonum] Unity 6.4

=== エディタに同梱されたProject Auditor

Unity 6.4では、Project Auditorがエディタに同梱されました。Addressablesのバンドル間の重複のような肥大化を機械的に洗い出せます。6.4以降は診断ルールが別パッケージに分かれているため、導入手順がバージョンによって異なります。調査の手順は、第3部2章「運用フェーズのアセット肥大化とProject Auditorによる調査」で扱います。

=== Physicsモジュールを無効にする

Unity 6.4では、組み込みのPhysicsモジュールをPackage Managerから無効にできるようになりました@<fn>{fn8}。3D物理を使わないタイトルでは、その分ビルド容量を減らせます。ただしLocal Volumeを使う場合は有効のままにする必要があります。

==[nonum] Unity 6.5

=== Read/Writeの暗黙の有効化がなくなった

テクスチャやメッシュのRead/Writeを有効にすると、GPUに転送したデータとは別にCPU側にもコピーが残り、その分メモリを消費します。これまでのUnityは、Particle SystemのShapeモジュールやTerrainのディテール、Mesh ColliderがCPUからの読み取りを必要とする場合に、ビルド時にこの設定を暗黙的に有効にしていました。Unity 6.5ではテクスチャ、Unity 6.6ではメッシュについてこの暗黙の有効化がなくなり、必要なアセットで明示的に有効にしていなければ、Inspectorに警告が表示され、ビルドが失敗するようになりました。

公式ドキュメントはこの変更の理由としてビルド時間の短縮を挙げていますが、メモリの面でも意味があります。どのアセットがCPU側にコピーを持つのかがアセットの設定として見えるようになるため、本当にCPUからの読み取りが必要かを1つずつ判断でき、不要なコピーを減らす手がかりになります。

=== AndroidのLTO

Unity 6.5では、Androidにリンク時最適化 (LTO) の設定が追加されました。このトグルが効くのはエンジン側のライブラリだけで、Strip Engine Codeの無効化を前提とするため、実行速度の改善と容量の増加を比べて採用を判断することになります。実測の結果は、第3部5章「UnityのLTO設定とAndroid・iOSでの性能比較」で扱います。

=== mimallocの追加

Unity 6.5では、ネイティブメモリのアロケーターとしてmimallocが追加されました。mimallocは複数のスレッドが同時にメモリを確保・解放できるアロケーターで、6.5では選択制でしたが、Unity 6.6では既定で有効になっています@<fn>{fn9}。ジョブやワーカースレッドでネイティブメモリの確保が重なる場面では、スレッド間の待ちの削減が期待できます。一方で、アロケーターが変わればメモリ使用量や断片化の傾向も変わり、公式には効果の数値も示されていません。6.6へ更新するときは、ネイティブメモリの推移を更新前と比べておくのが安全です。問題があればMemory Settingsから無効にできます。

=== iOSの熱状態に応じた目標フレームレート

Unity 6.5では、iOSの熱状態がSeriousやCriticalになったときの目標フレームレートをPlayer Settingsで指定できるようになりました。発熱時にフレームレートを下げる処理を自前で書かずに、サーマルスロットリングによる急な性能低下を和らげられます。

=== AssetBundleのTypeTreeを分離する

AssetBundleには、シリアライズされたデータの型情報であるTypeTreeが含まれています。TypeTreeがあるおかげで、バンドルをビルドしたときとプレイヤーのUnityバージョンが異なっても読み込めますが、同じ型の情報がバンドルごとに重複して含まれ、容量を押し上げます。Unity 6.5とAddressables 2.9.1以降では、Extract TypeTree Dataを有効にすると、TypeTreeを別ファイルに分けてAssetBundleの容量を減らせます@<fn>{fn10}。配信するバンドルが多いタイトルほど、ダウンロード量と端末のストレージの両方に効きます。ただし、この設定はすでに配信しているすべてのAssetBundleを変更するため、公式には新規プロジェクトでの利用が推奨されています。

==[nonum] Unity 6.6

=== AssetBundleの後継となるContent Directories

Unity 6.6では、AssetBundleの後継となる新しいコンテンツビルドの仕組みとしてContent Directoriesが追加されました@<fn>{fn11}。公式ドキュメントが挙げる利点は、暗黙的な重複排除、読み込み時間の短縮、メモリのオーバーヘッドの削減です。AssetBundleでは、複数のバンドルから参照されるアセットが各バンドルに複製され、容量とメモリを二重に消費することがありました。第3部2章「運用フェーズのアセット肥大化とProject Auditorによる調査」で扱うバンドル間の重複は、Content Directoriesでは自動的に排除されます。アセットの読み込みと解放は @<tt>{Loadable<T>} APIで行い、async/awaitにも対応しています。Addressables 4.0ではContent Directory用のスキーマが追加され、既存のグループを変換できます@<fn>{fn12}。

ただし、Content Directoriesが現時点で対応するのはローカルコンテンツのビルドだけで、リモート配信には対応していません。ベータ版のUnity 6.7でも同じで、リモートのコンテンツへの対応はUnity 7世代で予定されています@<fn>{fn13}。Addressables 4.0でも、リモートの読み込みパスを持つグループを変換すると、スキーマは無効な状態で追加されます。ダウンロードでコンテンツを追加していく多くのモバイルタイトルでは、配信するコンテンツは引き続きAssetBundleで扱い、アプリに同梱するコンテンツから移行を検討することになるでしょう。参照の循環に含まれるアセットの合計が64MBを超えるとビルドが失敗するという制約もあります。

どちらの仕組みを使う場合でも、アセットをいつ読み込み、いつ解放するかという設計はそのまま当てはまります。第3部1章「AssetBundleのライフタイム管理」は、読み込みと解放を基盤層に集約し、読み込んだ時点でアセットの寿命を決めることで、解放漏れや不要なアセットの常駐を構造で防ぎます。

=== シェーダー定数をBuild Profileごとに指定する

Unity 6.6では、シェーダー定数の値をプロジェクト全体またはBuild Profileごとに指定できるようになりました@<fn>{fn14}。プラットフォームごとに値だけが違う処理を、キーワードでバリアントを増やさずに書き分けられます。

=== Build HistoryとBuild Analysis

Unity 6.6では、Build HistoryとBuild Analysisウィンドウが追加され、プレイヤーのビルドレポートが自動で蓄積されるようになりました。ビルドごとの容量の推移を追えるため、肥大化の原因となった変更を特定しやすくなります。ただしAssetBundleのビルドへの対応は限定的です。

==[nonum] Unity 6.7（発表段階）

次の機能は2026年7月のUnite Seoulで発表されたもので、執筆時点ではドキュメントで詳細を確認できない発表段階の情報です@<fn>{fn15}。

=== Neural Texture Compression

Neural Texture Compression (NTC) は、学習済みのモデルを使い、テクスチャをほぼ劣化なく圧縮する技術です。発表では、見た目の劣化なしにテクスチャの実行時メモリを50%以上、ディスク容量を約70%削減した例が示されました。NTCは、Unity 6.7から順次提供が始まるUnity Neuralの一部として紹介されています。

=== Fast Build Profile

Fast Build Profileは、Unity 6.7で追加される予定の新しいビルドプロファイルで、シェーダーのビルド時間を最大90%短縮できるとされています。あわせて、PSOのトレースとウォームアップによって実行時のスタッターを減らす仕組みも紹介されています。

==[nonum] 第3部に向けて

Unity 6.0から6.7までをたどると、エンジン側の変化は、これまで自前のスクリプトや運用で補ってきたことを設定として持てるようにする方向に進んでいます。一方で、これらの変化は既定値のまま待っていれば効果が出るものではありません。mimallocのように更新するだけで挙動が変わるものは、更新前後の計測で確かめる必要があります。Content Directoriesのように、配信の要件によってはまだ移行できないものもあります。アセットをいつ読み込んで解放するか、どこまでコードを削るか、端末ごとに何を切り替えるかといった設計の判断は、エンジンのバージョンが変わっても残ります。

第3部の各章は、その判断に必要な設計と計測を、実際のプロジェクトでの事例とともに扱います。新しいバージョンへの更新を検討するときの判断材料として、役立てていただければ幸いです。

//footnote[fn1][Unity Discussions, Unity 6.7 Beta is now available @<href>{https://discussions.unity.com/t/unity-6-7-beta-is-now-available/1736830}]
//footnote[fn2][Unity, Unity 7 roadmap revealed at Unite Seoul @<href>{https://unity.com/news/unity-7-roadmap-revealed-at-unite-seoul}]
//footnote[fn3][Unity Manual 6000.1, What's new in Unity 6.1 @<href>{https://docs.unity3d.com/6000.1/Documentation/Manual/WhatsNewUnity61.html}]
//footnote[fn4][Unity Manual 6000.0, Configure managed code stripping @<href>{https://docs.unity3d.com/6000.0/Documentation/Manual/managed-code-stripping-configure.html}]
//footnote[fn5][Unity Manual 6000.6, Introduction to Mesh LOD @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/lod/mesh-lod-introduction.html}]
//footnote[fn6][Unity Manual 6000.3, What's new in Unity 6.3 @<href>{https://docs.unity3d.com/6000.3/Documentation/Manual/WhatsNewUnity63.html}]
//footnote[fn7][Unity Blog, Improvements to shader build times and memory usage in 2021 LTS @<href>{https://unity.com/blog/engine-platform/2021-lts-improvements-to-shader-build-times-and-memory-usage}]
//footnote[fn8][Unity Manual 6000.4, What's new in Unity 6.4 @<href>{https://docs.unity3d.com/6000.4/Documentation/Manual/WhatsNewUnity64.html}]
//footnote[fn9][Unity Manual 6000.6, mimalloc @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/performance-mimalloc.html}]
//footnote[fn10][Unity Manual 6000.5, What's new in Unity 6.5 @<href>{https://docs.unity3d.com/6000.5/Documentation/Manual/WhatsNewUnity65.html}]
//footnote[fn11][Unity Manual 6000.6, Introduction to content directories @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/content-directories-introduction.html}]
//footnote[fn12][Addressables 4.0 Manual, Content build systems @<href>{https://docs.unity3d.com/Packages/com.unity.addressables@4.0/manual/content-build-systems.html}]
//footnote[fn13][Unity Blog, Content directories: Beyond the AssetBundle @<href>{https://unity.com/blog/content-directories-beyond-the-assetbundle}]
//footnote[fn14][Unity Manual 6000.6, What's new in Unity 6.6 @<href>{https://docs.unity3d.com/6000.6/Documentation/Manual/WhatsNewUnity66.html}]
//footnote[fn15][Unity Blog, Unite Seoul 2026 keynote recap @<href>{https://unity.com/blog/unite-seoul-keynote-2026-recap}]
