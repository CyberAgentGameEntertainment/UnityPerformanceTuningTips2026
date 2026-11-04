# **AssetBundleのライフタイム管理**

名前 / ローマ字：稲田 健人 / Inada Taketo  
所属：株式会社サイバーエージェント SGEコア技術本部  
Xアカウント / Webサイト / GitHubアカウント（任意）：

ゲームでは、テクスチャ、メッシュ、オーディオなど、多くのアセットを実行時にロードします。これらを各実装者が個別に管理すると、解放漏れや不要なアセットの常駐が発生しやすくなります。

そのため、AssetBundleやAddressablesなどから動的にロードするアセットは、原則としてロードと解放の窓口をシステム基盤へ集約しましょう。ロード時にアセットのライフタイムを決定し、その終了時に基盤側でまとめて解放できる仕組みにします。

本節でいう「解放」は、ロード基盤が管理する参照や解放用ハンドルを解放することを指します。実際にアセットがメモリからアンロードされるかどうかは、基盤の実装や参照数、依存関係の状態によります。

なお、以降で紹介する分類や実装方法は参考例です。各プロジェクトのゲーム構造に合った方法を選択してください。

### **アセットのライフタイムを分けて管理する**

まず、ゲーム内でアセットが必要となる期間を整理します。たとえば、次のように分類できます。

表: アセットのライフタイムの分類例 {{table:asset_lifetime_types}}

ライフタイム	保持期間の例  
Common	ログアウト、アカウント切り替え、タイトルへの復帰など、共通データをリセットするまで  
Scene	対象のシーンを終了するまで  
View	対象の画面やダイアログを閉じるまで  
Custom	バトルやイベントなど、独自に定めた期間が終了するまで

重要なのは、Common、Scene、Viewといった名前ではなく、それぞれのライフタイムがいつ始まり、いつ終了するのかを明確にすることです。

分類が粗すぎると不要なアセットが長期間残り、細かすぎるとロードと解放の回数や管理コストが増えます。また、寿命の異なるアセットが同じAssetBundleに含まれている場合、AssetBundle単位のアンロードでは一部のアセットだけを選んでアンロードできません。ライフタイムの分類は、AssetBundleの粒度（前作『Unity パフォーマンスチューニング バイブル』第5章「Tuning Practice \- AssetBundle」）とあわせて設計しましょう。

### **ロード時にライフタイムを決定する**

ライフタイムを分類したら、アセットをロードするときに登録先を決定します。

実装例の1つとして、ロード関数の引数にライフタイムを指定する方法があります。

CODE START csharp | ライフタイムを指定してロードする例 {{list:asset_load_with_lifetime}}  
var background \=  
    await AssetLoader.LoadAsync\<GameObject\>(  
        address,  
        AssetLifetime.Scene);  
CODE END

{{list:asset_load_with_lifetime}}では、ロードしたアセットをSceneライフタイムへ登録しています。シーンの終了時には、基盤側が登録されたアセットをまとめて解放します。

別の実装例として、ライフタイムごとにロード関数を分ける方法もあります。

CODE START csharp | ライフタイムごとにロード関数を分ける例 {{list:asset_load_by_lifetime}}  
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

LoadCommonAsync、LoadSceneAsync、LoadViewAsyncのように関数を分けると、呼び出し箇所からライフタイムを判断でき、登録先の指定ミスも防ぎやすくなります。一方、引数で指定する方法は、独自のライフタイムを追加しやすいという特徴があります。

どちらの方法でも重要なのは、ロード時にライフタイムが必ず決まり、その終了時に基盤側で解放できることです。

非同期ロードの完了前にライフタイムが終了する場合に備え、ロード開始時から処理を追跡し、途中終了時にはキャンセルするか、完了後の結果を解放できるようにします。また、同じアセットを複数のライフタイムから使用する場合は、参照数を管理し、最後の利用が終了した時点で解放します。

### **GameObjectにライフタイムを紐づける**

別の管理方法として、GameObjectをライフタイムの所有者にする方法があります。

画面やダイアログのルートGameObjectに専用のComponentを追加し、ロードしたアセットの解放用ハンドルを登録します。GameObjectが破棄されたときに、OnDestroyから登録済みのハンドルを解放します。

CODE START csharp | GameObjectにライフタイムを紐づける例 {{list:asset_scope_component}}  
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

{{list:asset_scope_component}}では、GameObjectの破棄後に非同期ロードが完了してAddが呼ばれた場合も、追加されたハンドルを即座に解放します。

ただし、GameObjectを破棄するだけで、動的にロードしたアセットが自動的に解放されるわけではありません。専用のComponentから、ロード基盤に対応した解放処理を呼び出す必要があります。

管理用のGameObjectはアセットごとに作成せず、1つの画面やダイアログなど、1つのライフタイム単位につき1つにまとめます。この方法はGameObjectとアセットの寿命が一致する場合に有効ですが、複数のシーンをまたいで使用するアセットなどには適さないことがあります。必要に応じて、ライフタイムごとの管理テーブルと使い分けましょう。

### **解放漏れを確認する**

Memory Profilerで画面やシーンの遷移前後のスナップショットを取得し、終了したライフタイムのアセットが残っていないかを比較します。同じ遷移を複数回繰り返し、アセットやロードハンドルが増え続けていないかを確認することも有効です。

計測は、必要な解放処理が完了してから同じ基準状態どうしで比較し、最終的には実機のプレイヤービルドで確認しましょう。ただし、ほかの機能から参照されているアセットや、キャッシュとして意図的に保持しているアセットもあるため、残っていることだけで解放漏れとは判断せず、実際の参照元まで確認します。

アセット管理は、開発後期に変更すると多くのロード処理へ影響します。開発の早い段階で必要なライフタイムを整理し、実装者が個々の解放タイミングを意識しなくても、プロジェクトに合ったライフタイムが適用される仕組みを設計しておきましょう。
