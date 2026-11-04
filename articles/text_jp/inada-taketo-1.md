# **UniTaskとキャンセル処理のライフタイム管理**

名前 / ローマ字：稲田 健人 / Inada Taketo

所属：株式会社サイバーエージェント SGEコア技術本部

Xアカウント / Webサイト / GitHubアカウント（任意）：

UniTaskを用いた非同期処理では、終了条件やキャンセルを適切に管理しないと、完了しない処理や不要な参照が残り続けることがあります。

前作『Unity パフォーマンスチューニング バイブル』では、MonoBehaviourが破棄された後もUniTaskが待機し続ける事例を紹介しました。本節では、CancellationTokenSourceの破棄漏れによって、不要なオブジェクトが参照され続ける事例を紹介します。

なお、CancellationTokenSourceやCreateLinkedTokenSourceはUniTask固有の機能ではなく、.NETのキャンセル処理を構成するAPIです。

本節で扱うコードのうち、using宣言（using var）はC\# 8の構文で、Unity 2020.2以降で利用できます。また、後述するCreateLinkedTokenSourceの内部実装は、Unity Technologiesが公開しているUnity 2022.3向けMono実装（unity-2022.3-mbeブランチ）を参照したもので、Unityのバージョンによって変わる可能性があります。

### **CancellationTokenSourceのDispose**

CancellationTokenは、処理へキャンセル要求を伝えるための値型です。CancellationToken自体はIDisposableを実装していないため、Disposeを呼び出す必要はありません。

ただし、CancellationToken.Registerでコールバックを登録した場合、その戻り値であるCancellationTokenRegistrationはIDisposableです{{fn:cancellation_token_registration_dispose}}。キャンセル前に登録されたコールバックは発行元のCancellationTokenSourceに保持されるため、登録が不要になった時点でDisposeして解除しないと、コールバックが参照するオブジェクトが解放されないことがあります。

一方、CancellationTokenSourceはIDisposableを実装しています。使用を終えたCancellationTokenSourceは、所有者がDisposeを呼び出します。

CODE START csharp | CancellationTokenSourceをDisposeする例 {{list:cts_dispose}}

private async UniTask LoadCancelableAsync()

{

	using var cts \= new CancellationTokenSource();

	

	// キャンセル要求を行う処理は省略

	await LoadAsync(cts.Token);

}

CODE END

{{list:cts_dispose}}では、using宣言によってCancellationTokenSourceを管理しています。非同期処理が正常終了した場合だけでなく、キャンセルや例外によって終了した場合にも、メソッドを抜ける際にcts.Dispose()が自動的に呼び出されます。

CancelとDisposeの役割は異なります。Cancelは処理へキャンセルを要求し、DisposeはCancellationTokenSourceが保持するタイマーやコールバック登録などを解除・解放します。Disposeを呼び出しただけでは、処理にキャンセル要求は通知されません。

なお、Disposeを呼び忘れても、CancellationTokenSourceと関連するオブジェクトがすべて外部から参照されなくなれば、GCの回収対象になります{{fn:managed_memory}}。ただし、長寿命のCancellationTokenSourceへのコールバック登録などによって参照が残る場合があるため、使用を終えた時点でDisposeすることを原則としましょう。

### **CancellationTokenSource.CreateLinkedTokenSource**

CancellationTokenSource.CreateLinkedTokenSourceは、複数の元トークンを監視する新しいCancellationTokenSourceを生成します。元トークンのいずれかがキャンセルされると、生成されたCancellationTokenSourceのTokenがキャンセル状態になります。

元トークン同士がキャンセルされるわけではなく、キャンセルは元トークンからリンク先へ伝播します。

以下では、tokenAとtokenBの発行元となるCancellationTokenSourceを、それぞれA、Bとします。

Unity TechnologiesがGitHubで公開しているUnity 2022.3向けMono実装{{fn:mono_cancellation_token_source}}では、CreateLinkedTokenSourceは元トークンにキャンセルコールバックを登録します。参照関係に関わる部分を簡略化すると、{{list:linked_source_internal}}のようになります。

CODE START csharp | CreateLinkedTokenSourceの内部実装の要点 {{list:linked_source_internal}}

linkedCts.m\_linkingRegistrations\[0\] \=

	tokenA.InternalRegisterWithoutEC(

    	s\_LinkedTokenCancelDelegate,

    	linkedCts);

 

linkedCts.m\_linkingRegistrations\[1\] \=

	tokenB.InternalRegisterWithoutEC(

    	s\_LinkedTokenCancelDelegate,

    	linkedCts);

CODE END

元トークンに登録されたコールバックからlinkedCtsが参照され、linkedCtsもAとBへの登録情報を保持します。

そのため、Bがアプリケーション全体で使用される長寿命のCancellationTokenSourceであり、長期間キャンセルされない場合、linkedCtsをDisposeしないと次の参照が残ることがあります。

長寿命のB → コールバック登録 → linkedCts → Aへの登録情報 → 短命なA

この参照関係だけで必ずメモリリークするわけではありません。すべてのオブジェクトが外部から参照されなくなれば、まとめてGCの回収対象になります。しかし、Bが生存し、リンク用のコールバックが解除されていない間は、linkedCtsはBから参照され続け、そこから短命なAも到達可能なまま残ります。

また、linkedCts.Tokenに登録されたコールバックがほかのオブジェクトを参照している場合、それらも保持されることがあります。

linkedCtsをDisposeすると{{fn:cancellation_token_source_dispose}}、内部でAとBへのCancellationTokenRegistrationが破棄され、元トークンに登録されたコールバックが解除されます。

CreateLinkedTokenSourceの戻り値は、リンクしたトークンを使用する処理のスコープ内で、using宣言などにより管理するとよいでしょう。

CODE START csharp | リンクしたCancellationTokenSourceをDisposeする例 {{list:linked_source_using}}

private async UniTask LoadAsync(

	CancellationToken tokenA,

	CancellationToken tokenB)

{

	using var linkedCts \=

        CancellationTokenSource.CreateLinkedTokenSource(

        	tokenA,

        	tokenB);

 

	await LoadAssetAsync(linkedCts.Token);

}

CODE END

{{list:linked_source_using}}では、LoadAssetAsyncが正常終了、キャンセル、または例外によって終了した後にlinkedCts.Dispose()が呼び出され、元トークンへの登録が解除されます。

リンクされたCancellationTokenSourceがすでにキャンセル状態であっても、ほかの元トークンへの登録が残っている場合があります。CreateLinkedTokenSourceの戻り値は、不要になった時点で必ずDisposeしましょう。

### **IDisposableオブジェクトのDispose漏れをチームで防ぐ**

ここまで、CancellationTokenSourceを例にIDisposableの破棄漏れを見てきました。対策は、オブジェクトの所有者が適切なタイミングでDisposeを呼び出すことを前提としています。しかし、プロジェクトの規模が大きくなると、すべての生成箇所をコードレビューだけで確認し続けることは困難です。特にCreateLinkedTokenSourceは短い記述で生成できるため、戻り値がIDisposableであることを見落としやすくなります。

チームで再発を防ぐには、個人の注意に加えて、破棄が必要なオブジェクトを扱いやすいAPIに寄せること、Analyzerで問題を早期に検知すること、必要に応じてCIで新しい違反を検出することを組み合わせるとよいでしょう。Analyzerだけに責務を集中させず、設計、静的解析、運用のそれぞれから同じ方針を支えると効果的です。

ここで紹介するusingによる破棄とAnalyzerによる検出は、CancellationTokenSourceに限らず、IDisposableを実装するほかのリソースにも応用できます。

本節では、こうした仕組み化のうち、C\#コードを自動的に検査できるRoslyn Analyzerを使った方法を紹介します。

### **Roslyn Analyzerとは**

Roslyn Analyzerは、C\#コードをコンパイル時やIDE上で静的に解析し、問題の可能性がある箇所へ警告や提案を表示する仕組みです。構文、型情報、処理経路などを基にバグの兆候やコーディング規約違反を検出でき、Code Fixを用意すれば修正候補も提示できます。

Analyzerが何を問題として検出するかは診断ルールとして定義され、各ルールにはCA2000のような識別用の診断IDが付けられます。CA2000は.NETが提供する既存ルールで、後述するUTK001とUTK002はプロジェクト独自に追加するルール案です。

Unityプロジェクトでは、AnalyzerのDLLをAssets以下へ配置し、RoslynAnalyzerラベルを付与して導入します{{fn:roslyn_analyzers}}。asmdefを利用して適用対象のアセンブリを限定することもできます。

### **既存のAnalyzerを最初の防波堤にする**

まず検討したいのは、.NETのコード品質規則に含まれるCA2000です{{fn:ca2000}}。CA2000は、ローカルで生成したIDisposableのオブジェクトが、破棄されないままスコープを外れる経路を検出します。CancellationTokenSourceもIDisposableであるため、new CancellationTokenSource()の破棄漏れを検出する最初の防波堤として利用できます。

ただし、利用しているAnalyzerのバージョンや解析設定によって、CreateLinkedTokenSourceのようにオブジェクトを生成して返すメソッドの戻り値やメソッド間の所有権移譲をどこまで追跡できるかは異なります。導入時には、new CancellationTokenSource()とCreateLinkedTokenSourceの両方について、検出される例と検出されない例を小さな検証プロジェクトで確認するとよいでしょう。既存規則で十分に検出できないパターンは、必要に応じて後述する独自規則で補えます。

### **非同期処理向けの独自ルール設計例**

既存AnalyzerでCreateLinkedTokenSourceの戻り値を十分に追跡できない場合は、プロジェクト固有のRoslyn Analyzerで補う方法があります。以下は、独自Analyzerを実装する場合のルール設計例です。説明のために、仮の診断IDとしてUTK001とUTK002を使用します。

UTK001では、new CancellationTokenSource()またはCancellationTokenSource.CreateLinkedTokenSource()で生成されたローカル変数が、using文、using宣言、またはfinally節のDisposeによって保護されているかを検査することを想定します。すべての正常終了経路だけでなく、returnや例外でメソッドを抜ける経路も解析対象とします。

CODE START csharp | UTK001が検出する例 {{list:analyzer_detects_linked_source}}

private async UniTask LoadAsync(

	CancellationToken tokenA,

	CancellationToken tokenB)

{

	var linkedCts \=

        CancellationTokenSource.CreateLinkedTokenSource(

        	tokenA,

        	tokenB);

 

	await LoadAssetAsync(linkedCts.Token);

}

CODE END

{{list:analyzer_detects_linked_source}}では、linkedCtsがDisposeされないままメソッドを抜けます。この規則では、CreateLinkedTokenSourceの呼び出し位置へ警告を出し、using宣言またはtry-finallyで破棄を保証するよう案内することを想定します。

UTK002では、CancellationToken.Registerの戻り値であるCancellationTokenRegistrationが、そのまま破棄されていないかを検査することを想定します。Registerの戻り値を受け取らずに捨てるコードは、登録解除の責務が不明確です。コールバックを非同期処理の間だけ有効にするなら、using宣言で管理するとよいでしょう。フィールドへ保持する場合は、所有者の終了処理でDisposeします。

CODE START csharp | CancellationTokenRegistrationを管理する例 {{list:registration_using}}

private async UniTask WaitAsync(CancellationToken token)

{

	using var registration \= token.Register(OnCanceled);

 

	await WaitForSignalAsync(token);

}

CODE END

{{list:registration_using}}では、WaitForSignalAsyncが終了した時点でregistrationが破棄されます。一方、アプリケーション全体で共有するTokenへ永続的に登録するなど、登録とTokenの寿命を一致させる設計もあります。そのような例まで一律に警告すると誤検知が増えるため、UTK002はまず情報レベルで試し、プロジェクトで頻出する安全なパターンを除外できるようにするとよいでしょう。

フィールドに保持されたCancellationTokenSourceは、ローカル変数よりも所有権の判定が難しくなります。第二段階の規則として、フィールドへ新しいCancellationTokenSourceを代入する前に古い値をDisposeしているか、MonoBehaviourのOnDestroyなど所有者の終了処理でDisposeしているかを検査できます。ただし、別オブジェクトへの所有権移譲を伴う場合があるため、ローカル変数の規則とは分けて導入するとよいでしょう。

### **Code Fixで修正方法を揃える**

Roslyn Analyzerには、検出した問題をIDE上で自動修正するCode Fixも用意できます。たとえば、UTK001で所有権がメソッド内に完結する単純なケースに、varをusing varへ変換するCode Fixを提供すると、安全に機械変換できるパターンの修正方法をチーム内で統一しやすくなります。フィールドへの代入や戻り値による所有権移譲など、破棄タイミングを一意に決められないケースには自動修正を提示せず、設計の確認を促す診断に留めるとよいでしょう。

コードレビューではAnalyzerの結果だけでなく、生成、Cancel、Disposeの担当がコードから読み取れるかも確認するとよいでしょう。Analyzerに警告がないことだけでは、安全性を証明できない点にも注意が必要です。

### **Analyzerだけでは検知できない問題**

静的解析には限界があります。CancellationTokenSourceをコレクションへ格納した場合や、別のオブジェクトへ渡して所有権を移した場合などは、最終的な破棄箇所を正確に判断できないことがあります。誤検知を避けるために解析を弱くすると、今度は検出漏れが増えます。

この領域は、Development Buildやテスト環境での実行時検証によって補う方法があります。プロジェクト共通の生成用の仕組みや、寿命を管理する型を経由してCancellationTokenSourceを生成し、生成数と破棄数を記録すれば、シーン遷移やテスト終了時に未破棄のスコープが残っていないかを確認できます。生成時のスタックトレースも記録する場合は、負荷を考慮して開発環境だけで有効にするとよいでしょう。

必要に応じて、Profilerでメモリの増加を確認する調査、実行時の未破棄検知、Roslyn Analyzerによる編集時の警告を組み合わせるとよいでしょう。問題が発生してから担当者が調査する流れだけでなく、コードを書いた時点で修正方法まで提示できる流れにすると、チームとしての再発防止につながります。

{{pagebreak}}

### **参考資料**

Unity Manual『Roslynアナライザーとソースジェネレーター』

Microsoft Learn『CA2000: スコープを失う前にオブジェクトを破棄』

Unity Technologies『Mono CancellationTokenSource.cs（unity-2022.3-mbeブランチ）』

Microsoft Learn『CancellationTokenSource.Dispose Method』

Microsoft Learn『CancellationTokenRegistration.Dispose Method』

Unity Manual『マネージメモリ』

脚注: {{fn:roslyn_analyzers}}

https://docs.unity3d.com/ja/2022.3/Manual/roslyn-analyzers.html（参照日：2026年9月8日）

脚注: {{fn:ca2000}}

https://learn.microsoft.com/ja-jp/dotnet/fundamentals/code-analysis/quality-rules/ca2000（参照日：2026年9月8日）

脚注: {{fn:mono_cancellation_token_source}}

https://github.com/Unity-Technologies/mono/blob/unity-2022.3-mbe/mcs/class/referencesource/mscorlib/system/threading/CancellationTokenSource.cs（参照日：2026年9月8日）

脚注: {{fn:cancellation_token_source_dispose}}

https://learn.microsoft.com/ja-jp/dotnet/api/system.threading.cancellationtokensource.dispose（参照日：2026年9月8日）

脚注: {{fn:cancellation_token_registration_dispose}}

https://learn.microsoft.com/ja-jp/dotnet/api/system.threading.cancellationtokenregistration.dispose（参照日：2026年9月8日）

脚注: {{fn:managed_memory}}

https://docs.unity3d.com/ja/2022.3/Manual/performance-managed-memory.html（参照日：2026年9月8日）