=[nonum] 2026年のスクリプティング・アニメーション動向

==[nonum] スクリプティングの動向

Unity 6世代ではスクリプティングに関する状況はそれほど変化していません。ただしUnity 7では@<b>{CoreCLR}と@<b>{ECS for All}という、C# コードのパフォーマンスチューニングに関する前提を大きく変える機能の追加が控えており、Unity 6世代でも一部それに向けたAPIの変更が行われています。この章ではUnity 6世代のAPIの紹介とともに、Unity 7で予定されている変更なども一部紹介します。なお、Unity 7に関する情報は執筆時現在のものであり、実際のリリースとは異なっている可能性があることをご了承ください。

=== 非同期処理

@<b>{Awaitable}はUnity 2023で登場したUnity標準のクラスで、従来は@<b>{UniTask}などの外部ライブラリを使って行っていたような効率的なasync / await処理をUnity標準クラスのみで記述できるようになりました。ただしAwaitableはUniTaskと比較すると機能セットが少ないので、既にUniTaskを利用している場合はそのままUniTaskを継続利用してもよいでしょう。外部パッケージに依存できない状況下ではTaskやValueTask, コルーチン の代わりにAwaitableを使用することで性能の向上が期待できます。また、第2部5章では非同期処理で問題になりやすいCancellationTokenの管理について解説しています。

=== .NETランタイム

Unity 6時点の .NETランタイムはこれまで通りMonoとIL2CPPが存在し、.NET Standard 2.1のAPIセットとC# 9が利用可能です。Unity 7ではMonoが廃止され、エディタと一部のデスクトッププレイヤーで@<b>{CoreCLR}の導入が予定されています。CoreCLRは .NET本家（.NET Core / .NET）で利用されているランタイム実装であり、パフォーマンスにおいてMonoより先行しています。モバイルやコンソールプラットフォームでは当面CoreCLRは導入されず、引き続きIL2CPPが第一級の選択肢となりますが、エディタはCoreCLRに移行するほか、実行時に利用できる標準APIの追加や実装の最適化など一部で恩恵があります。

==== CoreCLRのJIT

CoreCLRではC# コードをネイティブコードに変換するJITに多数の最適化が加えられています。ほとんどの場面ではMonoからCoreCLRに移行するだけでC# コードの実行速度は向上するでしょう。Unityから公開されたベンチマーク結果によると、GC利用のないワークロードではCoreCLRはMonoと比較して実行速度が2 〜 3倍に上昇し、IL2CPPやBurstに匹敵する性能が得られることが示されています@<fn>{fn1}。これまで、特にMonoとIL2CPPのどちらかを選択できるデスクトッププラットフォームではパフォーマンス観点でIL2CPPが最適な選択肢でしたが、CoreCLR登場以降はこの前提も変化するかもしれません。

==== .NET 10のBCL

これまで .NET BCL@<fn>{fn2}はMonoに付属する実装が利用されており、.NET Standard 2.1相当のAPIセットに制限されていましたが、CoreCLRの導入に伴い .NET 10相当のBCLに更新される見込みです。利用できるAPIが大きく増えるほか、内部実装がより洗練され、性能の改善が期待できます。新しいBCLはIL2CPPでも利用可能になる予定で、CoreCLRが導入されないモバイルやコンソールプラットフォームにおいても性能面と機能面において恩恵があります。

==== CoreCLRのGC / メモリ管理

CoreCLRはGarbage Collector (GC)の実装もMono / IL2CPPと異なります。これまでUnityのMonoとIL2CPPでは@<b>{Boehm GC} (Boehm-Demers-Weiser GC, bdwgc)と呼ばれるGC実装が利用されていましたが、CoreCLRのGCはBoehm GCとは大きく異なる特徴を持っています@<fn>{fn3}。

まず、CoreCLR GCは@<b>{世代別GC (generational GC)}です。世代別GCは確保されたオブジェクトを「世代」と呼ばれるグループごとに管理します。新たに確保されたオブジェクトはまずgen0として扱われます。gen0に対するGCは比較的頻繁に行われますが、gen0 GCを生き延びたオブジェクトは次の世代gen1に移動し、gen0よりも低い頻度でGCが実行されるようになります。これは「新たに確保されたオブジェクトのほとんどが短命で、ある程度生き残ったオブジェクトはその後も長く生き残る場合が多い」という一般的な観測に基づいており、若い世代に対して頻繁に実行されるGCにかかる時間を短くする目的があります。CoreCLR GCでのメモリ最適化では、確保されたオブジェクトがどの世代に属しているか、どの世代に対するGCがいつ実行されているかなどが重要なポイントになるでしょう。

また、CoreCLR GCは@<b>{コンパクション}を行います。Boehm GCは一度確保されたオブジェクトを移動させないため、オブジェクトの確保と解放が繰り返されると次第にヒープが断片化し、実際に必要なサイズよりもヒープが大きくなりやすいという問題がありました。CoreCLR GCのコンパクションでは断片化したヒープ上でオブジェクトを移動させ、オブジェクトの間の隙間を埋めます。これによってヒープの拡大が抑えられます。

なお、IL2CPPでは引き続きBoehm GCが利用されるため、これまでと同様のメモリ最適化戦略が有効です@<fn>{fn4}。

GCのほかに、CoreCLRではJITによるメモリ確保の最適化も積極的に行われています。例えば@<b>{Object Stack Allocation}はメソッド内で利用が完結するような非常に短命なオブジェクトをヒープではなくスタック上に確保するJITの機能です。特にループの内側などホットパスで大量に確保される短命なオブジェクトなどは、MonoやIL2CPPほど問題を引き起こさなくなるかもしれません。

==== C# 14サポート

Unity 6時点で公式にサポートされているのはC# 9ですが、Unity 7ではC# 14が利用可能になる予定です。近年のC# リリースではパフォーマンス改善のための新機能も多数追加されており、パフォーマンスチューニング観点で知っておくべき言語機能も増えることになります。

==== Unity 6世代に先行導入されている機能

CoreCLRの正式導入はUnity 7で行われる予定ですが、一部のプラットフォームではUnity 6系で実験的なCoreCLRの導入が始まっています。執筆時現在最新のUnity 6.7 alpha (6000.7.0a6)ではWindows, macOS, LinuxのスタンドアロンプレイヤーでCoreCLRが利用可能です。ただしAPIセットはまだ .NET Standard 2.1に制限されています。同じくUnity 6.7 alphaでiOSプレイヤー(IL2CPP)では .NET 10相当のBCL実装が利用可能になっていますが、こちらも .NET Standard 2.1に制限されています。現段階ではUnity 7でのCoreCLR移行に向けたパフォーマンスと互換性の検証にのみ利用できると考えるべきでしょう。

=== DOTS

Unity 6ではDOTS (Data-Oriented Technology Stack)機能群が引き続き有効です。LLVMを活用してC# コードを高度に最適化されたネイティブコードに変換する@<b>{Burst}, コードを安全にマルチスレッド化する@<b>{C# Job System}, それらを活用してMonoBehaviourよりも高速にゲームの状態を更新する仕組みである@<b>{Entity Component System (ECS)}が利用できます。

Unity 7では@<b>{ECS for All}と題した機能群によってECSとGameObjectの連携が強化される見込みです。これまでECSでコンポーネントを格納するためのコンテナであるEntityはGameObjectとは互換性がなく、ECSの状態はGameObjectから完全に独立して管理されていました。Unity 7ではGameObjectがEntityとしても機能するようになり、GameObjectに対してECSのコンポーネントをアタッチできるようになります。これまでよりもECSとGameObjectを混在・連携させやすくなることで、ボトルネックになりやすいMonoBehaviourのUpdate処理群をECSに移動しやすくなり、メインスレッド負荷の軽減とフレームレートの向上に役立つでしょう。

#@# この節の最後の1行だけが次のページにあふれるため、このページを1行延ばす。
//embed[latex]{
\enlargethispage{\baselineskip}
//}

==[nonum] アニメーションの動向

Unity 6世代ではアニメーションのパフォーマンスにいくつか改善が加えられています。Unity 6.3でAnimatorとAnimationPlayableGraphに@<b>{ResetControllerState}メソッドが追加され、Animatorを容易にプーリングできるようになりました。詳しくは第2部2章で解説しています。Unity 6.5では@<b>{AnimationEventInfo}構造体が追加され、Animation Eventを受け取る際のAnimationEventクラスによるヒープメモリ確保を回避できるようになりました。

//footnote[fn1][Path to CoreCLR | GDC 2026 - Unity - YouTube @<href>{https://youtu.be/_t6xVfrmEWU?si=edILni6YnObyWkoj&t=218}]
//footnote[fn2][Base Class Library。System名前空間などの標準クラスライブラリ実装のこと]
//footnote[fn3][Fundamentals of garbage collection - .NET | Microsoft Learn - @<href>{https://learn.microsoft.com/en-us/dotnet/standard/garbage-collection/fundamentals}]
//footnote[fn4][Understanding Memory Management in Unity - Unity Discussion - @<href>{https://discussions.unity.com/t/understanding-memory-management-in-unity/1700107}]
