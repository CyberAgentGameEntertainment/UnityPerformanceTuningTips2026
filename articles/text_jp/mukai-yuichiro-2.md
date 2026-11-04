# **Unity Managed Code Stripping 実践ガイド**

向井 祐一郎 / Mukai Yuichiro / 所属: 株式会社アプリボット  
yucchiy\_  / blog.yucchiy.com / yucchiy  
\---

アプリサイズの削減手段として、Unityには未使用のコードをビルド時に取り除くManaged Code Strippingという機能について紹介します。  
設定1つで有効にできる手軽な機能ですが、削減を強めると「エディター上では動くのに、ビルドしたアプリだけで特定の機能が壊れる」という問題を引き起こすことがあります。  
本章では、Managed Code Strippingの仕組みと使い方、コードが必要以上に削減されてしまう典型的なパターンとその対処方法などを紹介します。

検証環境は以下の通りです。

*  Unity 6000.3.19f1  
* Addressables 4.0.1  
* macOS Sequoia 15.7.3

先に結論を書くと、Managed Code Strippingは設定をHighにして終わりにできる最適化ではありません。

静的解析から見えない依存を明示し、ビルドしたアプリのサイズと動作を検証し続ける運用とあわせて、初めて安全に使える機能です。

# **Managed Code Stripping とは何か**

## **機能の概要**

Managed Code Strippingは、ビルド時に未使用のマネージドコード（C\#コード）を検出し、ビルドしたアプリから取り除くUnity標準の機能です。

シーンやスクリプトから利用されていないクラスやメソッドが除去されることで、アプリのコードサイズを削減できます。

削減の対象はユーザーコードだけではなく、UnityEngineのモジュールや.NETのクラスライブラリ、プロジェクトに含まれるパッケージやプラグインのアセンブリも対象になるため、自分では手を入れづらいライブラリ内部の未使用コードまで削減することができます。

とくにIL2CPPビルドでは、マネージドコードが削減されることでその後のIL2CPPで生成されるC++コードも減るため、アプリサイズやビルド時間の短縮にもつながります。

なお、似た名前の設定に Strip Engine Code がありますが、こちらはUnityエンジンのネイティブコードを対象とした削減設定であり、マネージドコードを対象とするManaged Code Strippingとは別の機能です。

### **\#\#\# Managed Stripping Level**

どの程度積極的にコードを削るかは、Managed Stripping Levelという設定で制御します。

Edit \> Project Settings \> Player \> Other Settings \> Optimization の「Managed Stripping Level」から設定でき、下記のレベルを選択できます。

表: Managed Stripping Levelの選択肢 {{table:stripping_levels}}

| Level | 概要 |
| :---- | :---- |
| Disabled | ストリッピングを行わない。Monoビルドでのみ選択できる |
| Minimal | UnityEngineと.NETのクラスライブラリのみを対象に削減し、ユーザーコードは削除しない。IL2CPPビルドの既定値 |
| Low | ユーザーコードも解析対象に含めるが、保守的に削減する |
| Medium | 全アセンブリを対象に、サイズと安全性のバランスを取って削減する |
| High | 全アセンブリを最も積極的に削減する |

MonoビルドではDisabledが既定値のため、何も設定しなければストリッピングは行われません。

一方、IL2CPPビルドではMinimalが既定値であり、Disabledは選択できないため、程度の差はあれ必ずストリッピングが行われます。

なお、LowはUnity 6.3のドキュメントで将来的な廃止が予告されており、利用は推奨されていません。

参考: \[Managed code stripping\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping.html) / \[Configure managed code stripping\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-configure.html)

## **基本的な動作原理**

Managed Code Strippingは、\*\*UnityLinker\*\*というUnity内製のツールにより実現されています。

UnityLinkerはアプリのビルドパイプラインの中で、C\#コードのコンパイルが完了した後に実行され、コンパイル済みのアセンブリ（DLL）から未使用コードを取り除きます。

IL2CPPビルドでは、このストリッピングを経たアセンブリが、その後のC++コード変換のための入力になります。

UnityLinkerは、次の三段階で未使用コードを判定します。

1. 解析の起点となるコード（\*\*root\*\*）を決める  
2. rootから静的に参照されている型やメンバーを再帰的にたどり、使用中の印を付ける（\*\*mark\*\*）  
3. 最後まで印が付かなかったコードを、到達不能とみなしてアセンブリから取り除く

rootとして扱われる代表的なものは下記の通りです。

* ビルドに含まれるシーンやアセットから参照されている型  
* MonoBehaviourやScriptableObjectを継承した型  
* \[RuntimeInitializeOnLoadMethod\]などの属性が付いたメソッド  
* link.xmlや\[Preserve\]属性で明示的に保持を指定したコード（後述の対処方法で扱います）

ここで重要なのは、コードを残すかどうかが「実行時に実際に使われるか」ではなく、「ビルド時の静的解析でrootから到達できるか」で判断される点です。

静的解析とは、コードを実行せずに、コンパイル済みのコードに現れる参照だけを機械的にたどる解析のことです。

例えば new による型の生成やメソッドの直接呼び出しは、コンパイルされたコードに参照先の型やメンバーの情報がそのまま残るため、UnityLinkerがたどれます。

一方で、コード上に参照が現れない形で呼び出されているコードは、実行時に使われるかどうかにかかわらず到達不能とみなされます。

そのため、静的解析から見えない参照しか持たないコードは、実行時には必要であっても未使用と判定されて削除されます。

また、markは型単位ではなくメンバー単位で行われるため、残った型でも使われていないメソッドは個別に削除されます。

実際にどのようなコードが削除されてしまうのかは、次節で具体的なコードと計測結果とともに確認します。

### **何が削られたかをMono.Cecilで確認する**

本章では、この後の節で「どの型やメソッドがビルドに残ったか」を繰り返し確認します。

その確認には、.NETのアセンブリを解析するライブラリである\*\*Mono.Cecil\*\*を用いています。

ビルド出力に含まれるアセンブリ（macOS向けMonoビルドなら「アプリ名.app/Contents/Resources/Data/Managed/Assembly-CSharp.dll」）を読み、型とメソッドの一覧をファイルに書き出します。

CODE START csharp | 型とメソッド一覧の出力 {{list:cecil_dump}}  
// アセンブリを読み、型とメソッド（.ctor 含む）の一覧を書き出す  
using var assembly \= AssemblyDefinition.ReadAssembly(dllPath);  
var lines \= assembly.MainModule.Types  
    .Where(type \=\> type.Name \!= "\<Module\>")  
    .SelectMany(type \=\> new\[\] { type.FullName }  
        .Concat(type.Methods.Select(method \=\> $"  {method.Name}")));  
File.WriteAllLines(outputPath, lines);  
CODE END

メソッドの一覧にはコンストラクタ（.ctor）も含まれるため、後述する「型は残っているのにコンストラクタだけが消える」症状の確認にも使えます。

Mono.CecilはUnity Package Manager経由でインストールして利用できます。エディター用のAssembly Definitionで overrideReferences を有効にして precompiledReferences に Mono.Cecil.dll を追加すると、エディターコードから参照できます。

Levelを変えた2つのビルドでこの一覧を取って差分を見れば、どの型が消え、どのメソッドがメンバー単位で削られたかを特定できます。

ビルド後には、ストリップ後のアセンブリが中間生成物としてLibrary配下に残ることがあります。

今回の環境では、Library/Bee/artifacts/MacStandalonePlayerBuildProgram/ManagedStripped/ にビルド出力と同一内容のDLLが残っており、ビルド成果物を展開しなくてもこちらを解析対象にできます。

このパスにストリップ後のアセンブリが保存されることは公式ドキュメントには明記されてないため、見つからない場合はビルド出力側を解析します。

以降の節で「型とメソッドを確認した」と書いている箇所は、基本的にはこの方法による確認です。ストリッピング前後のDLLを比較すれば、削減の内訳まで確認できます。

参考: \[Unity linker marking rules\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-marking-rules.html)

## **なぜコードを削りたいのか**

Managed Code Strippingを使う主目的は、アプリサイズの削減です。

副次的には、未使用コードがメモリ上に乗らないため、ランタイムでのメモリ使用量の削減も期待できます。

どの程度の効果があるかを、終了処理のスクリプトを1つ置いただけのシンプルなシーンをビルドし、Managed Stripping LevelのMinimalとHighを比較します。

下記に計測結果を示します（{{table:size_comparison}}）。

表: シンプルなシーンでのMinimalとHighの比較 {{table:size_comparison}}

| 指標 | Minimal | High | 削減 |
| :---- | :---- | :---- | :---- |
| マネージドアセンブリの合計サイズ | 7,702,016 bytes | 7,188,480 bytes | 約6.7% |
| アセンブリ（DLL）の数 | 52 | 51 | 1個 |
| アプリ全体のサイズ | 89,069,307 bytes | 88,548,299 bytes | 約0.6% |

小さなサンプルプロジェクトでも、マネージドコードは6.7%減りました。

内訳を見ると、どこからも使われていないSystem.Runtime.Serialization.dllが丸ごと削除され、mscorlib.dllが約272KB減り、シーンから参照されないコードを多く含むAssembly-CSharp.dllも22.5%減っています。

マネージドコードの削減量は、プロジェクトが参照するライブラリの量と、その中で実際に使われていないコードの割合で決まります。

大規模なプロジェクトでは多くのライブラリを導入する傾向にありますが、実際にライブラリの中でも使われない機能が多くなることがよくあり、それによってコード削減の恩恵を受けやすいでしょう。

IL2CPPビルドでは、削減されたマネージドコードの分だけC++への変換とコンパイルの対象も減るため、ネイティブバイナリのサイズとビルド時間にも効果が波及します。

# **Managed Code Stripping を適用する上での注意点**

前節で見たとおり、Managed Code Strippingの判断はビルド時の静的解析に依存します。

そのため、静的解析から見えない参照しか持たないコードは、実行時には必要であっても削除されてしまいます。

この問題が厄介なのは、症状がビルドしたアプリでしか現れないことです。

エディターでの実行や、ストリッピングが無効な既定のMonoビルドでは正常に動くため、開発中には気づきにくく、Stripping Levelを上げた途端に特定の機能だけが壊れる、という形で不具合が表面化します。

この節では、コードが必要以上に削られてしまう典型例の2つのパターンを取り上げます。

ひとつはリフレクションを介したコード呼び出し、もうひとつはUnityのエントリーポイントから参照されないコードです。

## **リフレクションによるコード呼び出しが消える**

前節で説明した「静的解析でrootから到達できるか」の判断を、具体的なコードで確認します。

参照のされ方が異なる3つのクラスを用意します（{{list:probe_targets}}）。

CODE START csharp | 参照のされ方が異なる3つのクラス {{list:probe_targets}}  
// new で直接生成される  
public class DirectlyUsedService  
{  
    public string Describe() \=\> "DirectlyUsedService is alive";  
}

// typeof で型だけ参照される  
public class TypeofOnlyService  
{  
    public string Describe() \=\> "TypeofOnlyService is alive";  
}

// リフレクションを介してのみ生成される  
public class ReflectionOnlyService  
{  
    public string Describe() \=\> "ReflectionOnlyService is alive";  
}  
CODE END

シーンに置いたMonoBehaviourから、この3つを次のように使います（{{list:stripping_probe}}）。

CODE START csharp | 3つのクラスを使うMonoBehaviour {{list:stripping_probe}}  
public class StrippingProbe : MonoBehaviour  
{  
    private static readonly string ReflectionTypeName \= "ReflectionOnlyService";  
    private static readonly string TypeofTypeName \= "TypeofOnlyService";  
    private static readonly string TypeofMethodName \= "Describe";

    private void Start()  
    {  
        // 静的な参照: 参照先の型とメソッドの情報がコンパイル済みコードに残る  
        var direct \= new DirectlyUsedService();  
        Debug.Log($"direct: {direct.Describe()}");

        // 静的な参照が型だけ: typeof は型を参照するが、メンバーへの静的参照はない  
        Debug.Log($"typeof: type is referenced: {typeof(TypeofOnlyService).Name}");

        // メンバーが残ったかは、静的解析から見えない経路で型を取り直して確認する  
        var typeofType \= Type.GetType(TypeofTypeName);  
        var describeMethod \= typeofType?.GetMethod(TypeofMethodName);  
        if (describeMethod \== null)  
        {  
            Debug.Log($"typeof: type {typeofType?.Name} exists but Describe is stripped");  
        }  
        else  
        {  
            Debug.Log($"typeof: {describeMethod.Invoke(Activator.CreateInstance(typeofType), null)}");  
        }

        // 静的解析から見えない参照: リフレクションを介して生成する  
        var type \= Type.GetType(ReflectionTypeName);  
        if (type \== null)  
        {  
            Debug.Log($"reflection: Type.GetType(\\"{ReflectionTypeName}\\") returned null");  
        }  
        else  
        {  
            var instance \= Activator.CreateInstance(type);  
            Debug.Log($"reflection: {type.GetMethod("Describe")?.Invoke(instance, null)}");  
        }  
    }  
}  
CODE END

このサンプルを、Managed Stripping LevelをMinimalとHighに変えてビルドして実行すると、結果は次のようになります。

CODE START text  
\# Minimal  
direct: DirectlyUsedService is alive  
typeof: type is referenced: TypeofOnlyService  
typeof: TypeofOnlyService is alive  
reflection: ReflectionOnlyService is alive

\# High  
direct: DirectlyUsedService is alive  
typeof: type is referenced: TypeofOnlyService  
typeof: type TypeofOnlyService exists but Describe is stripped  
reflection: Type.GetType("ReflectionOnlyService") returned null  
CODE END

Highでは、typeofのケースは型は取得できるのにGetMethodがnullを返し、リフレクションのケースはType.GetType自体がnullを返します。

まとめると下記のような挙動になります（{{table:probe_results}}）。

表: MinimalとHighでの残り方 {{table:probe_results}}

| 対象 | Minimal | High |
| :---- | :---- | :---- |
| StrippingProbe | 残る | 残る |
| DirectlyUsedService（newで参照） | 残る | 型もメソッドも残る |
| TypeofOnlyService（typeofで型だけ参照） | 残る | 型は残るがDescribeは削除される |
| ReflectionOnlyService（リフレクションのみ） | 残る | 型ごと削除される |

Minimalはユーザーコードを削除しないため、3つともそのまま残ります。

Highでは、参照のされ方の違いがそのまま結果に現れます。

DirectlyUsedServiceはnewで型を参照し、Describeも直接呼び出しているため、型もメソッドも残ります。

typeofで参照されるTypeofOnlyServiceは型としては残りますが、どこからも呼ばれていないDescribeはメンバー単位で削除されています。

そして、静的な参照がまったくないReflectionOnlyServiceは、型ごと削除されています。

なお、この実験で型名やメソッド名の文字列をすべてstatic readonlyフィールド経由で読んでいるのは、意図的な作りです。

UnityLinkerはリフレクションの典型的な使い方を一部理解しており、Type.GetType("ReflectionOnlyService") と定数文字列を直接渡した場合はHighでも型が保持されることを確認しています。

また、typeofで得たTypeに対して直接GetMethodを呼んだ場合は、メソッド名が定数でなくても、その型のpublicメソッドが一括で保持されました。

一方で、フィールド経由で渡した文字列まではUnityLinkerは追跡しません。

型名やメンバー名が設定ファイル、シリアライズされたデータ、サーバー応答などビルド時に確定しない経路から来る実際の状況を、フィールド読み出しで再現しています。

フィールドをconstにすると使用箇所へ定数文字列として埋め込まれるため、定数を直接渡した場合と同じ扱いになります。

### DIコンテナーにおける型解決が壊れる

ここまでの例は説明のために単純化していますが、同じ構造の問題は実際のライブラリで起こります。代表的なのがDIコンテナーです。

多くのDIコンテナーは、インターフェイスと実装型の対応を登録しておき、解決時にリフレクションでコンストラクタを呼び出してインスタンスを生成します。

この動きを模した最小のコンテナーで確認します。実際のDIコンテナーと同じく、Registerで登録しResolveで解決するインターフェイスにします（{{list:mini_container}}）。

CODE START csharp | 最小のDIコンテナー {{list:mini_container}}  
public sealed class MiniContainer  
{  
    private readonly Dictionary\<Type, Type\> \_registrations \= new Dictionary\<Type, Type\>();

    // 実装型を型引数で登録する。TImplementation は typeof 相当の静的な参照になる  
    public void Register\<TInterface, TImplementation\>() where TImplementation : TInterface  
    {  
        \_registrations\[typeof(TInterface)\] \= typeof(TImplementation);  
    }

    // 登録された実装型を、リフレクションでインスタンス化して返す  
    public TInterface Resolve\<TInterface\>()  
    {  
        var implementationType \= \_registrations\[typeof(TInterface)\];  
        return (TInterface)Activator.CreateInstance(implementationType);  
    }  
}  
CODE END

登録する実装型は、コンテナー経由でのみ使われる想定で、どこからも静的に参照しません（{{list:notification_service}}）。

CODE START csharp | 登録するインターフェイスと実装 {{list:notification_service}}  
public interface INotificationService  
{  
    string Notify();  
}

// コンテナー経由でのみ使われ、どこからも静的に参照されない実装  
public sealed class NotificationService : INotificationService  
{  
    public string Notify() \=\> "NotificationService.Notify succeeded";  
}  
CODE END

シーンに置いたMonoBehaviourから、登録と解決を行います（{{list:di_probe}}）。

CODE START csharp | 登録と解決を行うMonoBehaviour {{list:di_probe}}  
public sealed class DiContainerProbe : MonoBehaviour  
{  
    private void Start()  
    {  
        try  
        {  
            var container \= new MiniContainer();  
            container.Register\<INotificationService, NotificationService\>();  
            Debug.Log($"\[DiContainerProbe\] explicit: {container.Resolve\<INotificationService\>().Notify()}");  
        }  
        catch (Exception exception)  
        {  
            Debug.Log($"\[DiContainerProbe\] explicit failed: {exception.GetType().Name}: {exception.Message}");  
        }  
    }  
}  
CODE END

これを先ほどと同じ条件でビルドすると、結果は次のようになります。

CODE START text  
\# Minimal  
\[DiContainerProbe\] explicit: NotificationService.Notify succeeded

\# High  
\[DiContainerProbe\] explicit failed: MissingMethodException: Default constructor not found for type NotificationService  
CODE END

Highでは、Register\<INotificationService, NotificationService\>()の型引数が静的な参照になるため、NotificationServiceの型そのものは残ります。

また、インターフェイス経由で静的に呼ばれているNotifyも残ります。

しかし、リフレクションでしか呼ばれないデフォルトコンストラクタは静的な参照を持たないため、メンバー単位で削除されます。

その結果、Activator.CreateInstanceがコンストラクタを見つけられず、MissingMethodExceptionで解決に失敗します。

型は登録してあるのに、ビルドしたアプリでだけ解決に失敗するという不具合の原因です。

実際のDIコンテナーでも、構造は同じです。

コンストラクタの解決がリフレクション頼みである以上、明示的に登録した型でも暗黙のデフォルトコンストラクタは削除の候補になります。

アセンブリから実装型をスキャンして自動登録する方式であれば、静的な参照を持たない実装型が型ごと削除され、登録自体が失敗します。

## **Unity エントリーポイントから参照がないコードが消える**

もうひとつのパターンは、シーンからもスクリプトからも参照されず、AssetBundleなどの外部コンテンツを介してのみ必要になるコードです。

前節で見たとおり、rootの供給源はビルドに含まれるシーンやアセットです。

AssetBundleとして別にビルドされるコンテンツはアプリ本体のビルドに含まれないため、そのコンテンツが参照する型はrootとして扱われません。

したがって、コンテンツ側だけが必要とする型は、アプリのコードに静的な参照がなければ、実行時に読み込まれるかどうかにかかわらず未使用と判定されます。

このパターンを、Timelineアセットで再現します。

TimelineアセットをAssetBundleにだけ含め、アプリ側はUnity.Timelineを静的に参照せず、アセット名で読み込んでPlayableDirectorから再生します（{{list:timeline_probe}}）。

CODE START csharp | AssetBundleからTimelineアセットを再生するMonoBehaviour {{list:timeline_probe}}  
public class TimelineAssetBundleProbe : MonoBehaviour  
{  
    private const string BundleDirectoryName \= "TimelineBundles";  
    private const string BundleName \= "timeline.bundle";  
    private const string TimelineAssetName \= "TimelineOnly";

    private void Start()  
    {  
        var bundlePath \= Path.Combine(Application.streamingAssetsPath, BundleDirectoryName, BundleName);  
        var bundle \= AssetBundle.LoadFromFile(bundlePath);  
        if (bundle \== null)  
        {  
            Debug.Log("timeline bundle: failed to load asset bundle");  
            return;  
        }

        var asset \= bundle.LoadAsset(TimelineAssetName);  
        if (asset \== null)  
        {  
            Debug.Log($"timeline bundle: asset {TimelineAssetName} failed to load");  
            bundle.Unload(true);  
            return;  
        }

        Debug.Log($"timeline bundle: loaded {asset.GetType().AssemblyQualifiedName}");  
        var playableAsset \= asset as PlayableAsset;  
        if (playableAsset \== null)  
        {  
            Debug.Log($"timeline bundle: asset {TimelineAssetName} is not playable");  
            bundle.Unload(true);  
            return;  
        }

        var director \= gameObject.AddComponent\<PlayableDirector\>();  
        director.playableAsset \= playableAsset;  
        director.Play();  
        Debug.Log($"timeline bundle: play state={director.state}, graph valid={director.playableGraph.IsValid()}");  
        bundle.Unload(false);  
    }  
}  
CODE END

AssetBundleの作成後に元のTimelineアセットをプロジェクトから削除してからアプリをビルドすることで、アプリとは別にコンテンツを更新する方式を模しています。

Managed Stripping LevelをMinimalとHighに変えてビルドします。どちらのレベルでも同じ結果になります。

CODE START text  
\# Minimal / High とも  
The referenced script (UnityEngine.Timeline.TimelineAsset) on this Behaviour is missing\!  
timeline bundle: asset TimelineOnly failed to load  
CODE END

また、ビルドしたアプリのManagedフォルダーを確認すると、どちらのビルドでもUnity.Timeline.dll自体が存在しませんでした。

注意したいのは、MinimalとHighで結果が変わらないことです。

このパターンへの対処はStripping Levelの調整ではなく、必要な型やアセンブリの保持を明示することです。

その手段は次節で扱い、Addressablesを使う場合の仕組みは応用編で扱います。

参考: \[How code stripping affects content\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-content.html) / \[タイムラインアセットが消える事例（Qiita, 2021）\](https://qiita.com/KyoheiOkawa/items/0cf9cc09679eababec75)

# **コードを削りすぎる問題への対処方法**

## **基本的な対処方法**

必要なコードが削られるのを防ぐ最も基本的な手段は、UnityLinkerに「このコードは使われる」と明示的に伝えることです。

Unityはそのための手段として、link.xmlファイルと\[Preserve\]属性を提供しています。

どちらで指定したコードも解析のrootとして扱われ、静的な参照がなくても削除されなくなります。

### **link.xml**

link.xmlは、保持したいアセンブリや型、メンバーを列挙するXMLファイルです。

プロジェクトのAssets配下に置くと、UnityLinkerがビルド時に読み取ります。

前節のサンプルで型ごと削除されたReflectionOnlyServiceは、次のlink.xmlで保持できます（{{list:linkxml_reflection}}）。

CODE START xml | ReflectionOnlyServiceを保持するlink.xml {{list:linkxml_reflection}}  
\<linker\>  
  \<assembly fullname="Assembly-CSharp"\>  
    \<type fullname="ReflectionOnlyService" preserve="all" /\>  
  \</assembly\>  
\</linker\>  
CODE END

このlink.xmlをAssets直下に置いてHighでビルドし直すと、実行結果が次のように変わりました。

CODE START text  
\# High（link.xml なし）  
reflection: Type.GetType("ReflectionOnlyService") returned null

\# High（link.xml あり）  
reflection: ReflectionOnlyService is alive  
CODE END

ビルドに含まれるアセンブリを確認しても、ReflectionOnlyServiceがDescribeごと保持されていることが確認できます。

link.xmlは保持対象をコードの外側で指定できるため、自分では編集できないDLLやパッケージの型にも使えます。

保持範囲はアセンブリ単位、型単位、メソッドやフィールドなどのメンバー単位で指定できます。

なお、アセンブリ全体をpreserve="all"で保持する書き方は、手軽ですがそのアセンブリに対するストリッピングの効果を打ち消します。

問題の切り分けには使えますが、恒久的な対応では必要な型・メンバーまで絞りこめる方が良いです。

### **\[Preserve\] 属性**

自分で編集できるコードであれば、UnityEngine.Scripting名前空間の\[Preserve\]属性を対象に直接付けられます。

注意したいのは、\[Preserve\]は付けた場所によって保持範囲が変わることです。

この違いを、型に付けたクラスと、メソッドにだけ付けたクラスの2つで確認します（{{list:type_preserved}}、{{list:method_preserved}}）。

CODE START csharp | 型にPreserveを付けた場合 {{list:type_preserved}}  
using UnityEngine.Scripting;

// 型に \[Preserve\] を付ける。メソッドには付けない  
\[Preserve\]  
public sealed class TypePreservedService  
{  
    public string Describe() \=\> "TypePreservedService.Describe is alive";  
}  
CODE END

CODE START csharp | メソッドにだけPreserveを付けた場合 {{list:method_preserved}}  
using UnityEngine.Scripting;

// 型には付けず、メソッドにだけ \[Preserve\] を付ける  
public sealed class MethodPreservedService  
{  
    \[Preserve\]  
    public string Describe() \=\> "MethodPreservedService.Describe is alive";  
}  
CODE END

この2つを、静的な参照を持たない形でリフレクションから生成して呼び出すと、Highでの結果は次のようになりました（{{table:preserve_results}}）。

表: Preserveを付けた位置とHighでの保持範囲 {{table:preserve_results}}

| 対象 | 型 | デフォルトコンストラクタ | Describe |
| :---- | :---- | :---- | :---- |
| TypePreservedService | 残る | 残る | 削除される |
| MethodPreservedService | 残る | 削除される | 残る |

型に付けた\[Preserve\]は、型とデフォルトコンストラクタを保持しますが、属性を付けていないDescribeは保持しません。

メソッドに付けた\[Preserve\]は、その型とメソッドを保持しますが、今度はデフォルトコンストラクタが保持されず、Activator.CreateInstanceによる生成がMissingMethodExceptionで失敗します。

リフレクションで生成して呼び出すところまで動かすには、型とメソッドの両方に\[Preserve\]を付ける必要があります。

「\[Preserve\]を付けたのに動かない」という場合は、まず属性を付けた位置と保持範囲の対応を疑います。

参考: \[Preserving code using annotations\](https://docs.unity3d.com/6000.3/Documentation/Manual/managed-code-stripping-preserving.html)

## **応用編**

###  **Preserve 以外の属性の活用**

\[Preserve\]が「この要素を残す」という点の指定なのに対し、UnityEngine.Scripting名前空間には「このインターフェイスの実装をすべて残す」のように、パターンで保持を宣言する属性があります。

「利用者が実装した型をリフレクションで探して呼び出す」といったフレームワークによくある構造では、実装型を1つずつ\[Preserve\]で指定する代わりに、インターフェイスや基底クラスに属性を付けることで対応できます。

* \[RequireImplementorsAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireImplementorsAttribute.html)：インターフェイスに付けると、そのインターフェイスを実装する型を保持する](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireImplementorsAttribute.html\)：インターフェイスに付けると、そのインターフェイスを実装する型を保持する)  
* \[RequireDerivedAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireDerivedAttribute.html)：クラスに付けると、その型を継承する型を保持する](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequireDerivedAttribute.html\)：クラスに付けると、その型を継承する型を保持する)  
* \[RequiredMemberAttribute\]([https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredMemberAttribute.html)：メンバーに付けると、その型が残る場合にそのメンバーも保持する](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredMemberAttribute.html\)：メンバーに付けると、その型が残る場合にそのメンバーも保持する)  
* \[RequiredInterfaceAttribute\](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Scripting.RequiredInterfaceAttribute.html)：型に付けると、指定したインターフェイスの実装情報を保持する

代表として\[RequireImplementors\]の挙動を確認します。

インターフェイスに属性を付け、実装はどこからも静的に参照しません（{{list:require_implementors}}）。

CODE START csharp | RequireImplementorsを付けたインターフェイスと実装 {{list:require_implementors}}  
// インターフェイスに付けると、実装型が保持の対象になる  
\[RequireImplementors\]  
public interface IRequiredContract  
{  
}

// どこからも静的に参照されない実装  
public sealed class RequiredContractImpl : IRequiredContract  
{  
}  
CODE END

Highでビルドすると、RequiredContractImplはデフォルトコンストラクタごと保持されました。

静的な参照を持たない実装型が、インターフェイス側の宣言だけで残ります。

\[RequireDerived\]を付けた基底クラスの派生型でも、同様に保持を確認できました。

### **IUnityLinkerProcessor でビルド時に link.xml を生成する**

保持する対象のコードが決まっている場合は、link.xmlに対象のクラスやメンバーを指定するだけで十分です。

一方で、ビルドターゲットごとに保持したい型が違う、ビルドに含めるコンテンツによって必要な型が変わる、といった状況では、link.xmlを手で更新し続けるのは現実的ではありません。

\*\*IUnityLinkerProcessor\*\*を使うと、ビルド時に動的にlink.xmlを生成してUnityLinkerへ渡せます。

IUnityLinkerProcessorはエディター用のインターフェイスで、実装クラスをエディターアセンブリに置くとビルド時に自動で呼ばれます。

GenerateAdditionalLinkXmlFileが返したパスのlink.xmlが、UnityLinkerの入力に追加されます。

最小の実装は次のとおりです（{{list:linker_processor}}）。

CODE START csharp | link.xmlを生成する最小のIUnityLinkerProcessor実装 {{list:linker_processor}}  
using System.IO;  
using UnityEditor.Build;  
using UnityEditor.Build.Reporting;  
using UnityEditor.UnityLinker;  
using UnityEngine;

public sealed class PreserveListLinkerProcessor : IUnityLinkerProcessor  
{  
    private const string LinkXmlContent \= @"\<linker\>  
  \<assembly fullname=""Assembly-CSharp""\>  
    \<type fullname=""GeneratedPreserveService"" preserve=""all"" /\>  
  \</assembly\>  
\</linker\>  
";

    public int callbackOrder \=\> 0;

    // 返したパスの link.xml が UnityLinker の入力に追加される  
    public string GenerateAdditionalLinkXmlFile(BuildReport report, UnityLinkerBuildPipelineData data)  
    {  
        var path \= Path.Combine(Path.GetTempPath(), "managed-stripping-sample-generated-link.xml");  
        File.WriteAllText(path, LinkXmlContent);  
        Debug.Log("\[PreserveListLinkerProcessor\] generated 1 assembly / 1 type.");  
        return path;  
    }  
}  
CODE END

この例では説明のために生成内容を固定していますが、実際にはこの部分を、設定アセットの読み取りやコンテンツの依存解析など、ビルド時の入力から組み立てる処理に置き換えます。

静的な参照を持たないGeneratedPreserveServiceを用意してHighでビルドすると、Assets配下にlink.xmlを置いていなくても、型・コンストラクタ・メソッドが保持されることが確認できました。

CODE START text  
\[GeneratedLinkXmlProbe\] type=present  
\[GeneratedLinkXmlProbe\] ctor=present, method=GeneratedPreserveService.Describe is alive  
CODE END

参考: \[IUnityLinkerProcessor\](https://docs.unity3d.com/6000.3/Documentation/ScriptReference/Build.IUnityLinkerProcessor.html)

###  **Addressables が生成する link.xml**

Addressablesでは、前節で見た「コンテンツ側だけが必要とする型が消える」問題に、コンテンツビルドの成果物としてlink.xmlを自動生成することで対応しています。

実際に何が生成されるかを確認します。コンテンツは最小構成にし、Cubeのメッシュを表示するだけのprefab（GameObjectにMeshFilterとMeshRendererを付けたもの）を1つ作って、Addressablesのグループに登録します。

コンテンツビルドを実行すると、link.xmlが生成されます。

Addressables 4.0.1では、生成先はLibrary/com.unity.addressables/aa/\<platform\>/AddressablesLink/link.xmlです。

今回生成された内容は次のとおりでした（{{list:addressables_linkxml}}）。

CODE START xml | Addressablesが生成したlink.xml {{list:addressables_linkxml}}  
\<linker\>  
  \<assembly fullname="Unity.Addressables, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null" preserve="all"\>  
    \<type fullname="UnityEngine.AddressableAssets.Addressables" preserve="all" /\>  
  \</assembly\>  
  \<assembly fullname="Unity.ResourceManager, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null" preserve="all"\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.AssetBundleProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.BundledAssetProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.IAssetBundleResource" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.InstanceProvider" preserve="all" /\>  
    \<type fullname="UnityEngine.ResourceManagement.ResourceProviders.SceneProvider" preserve="all" /\>  
  \</assembly\>  
  \<assembly fullname="UnityEngine.CoreModule, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null"\>  
    \<type fullname="UnityEngine.GameObject" preserve="all" /\>  
    \<type fullname="UnityEngine.MeshFilter" preserve="all" /\>  
    \<type fullname="UnityEngine.MeshRenderer" preserve="all" /\>  
    \<type fullname="UnityEngine.Transform" preserve="all" /\>  
  \</assembly\>  
\</linker\>  
CODE END

内容は2種類に分かれています。

ひとつはAddressablesのランタイム（Unity.AddressablesとUnity.ResourceManager）のprovider型で、グループ構成に応じて必要になるものです。

もうひとつは、登録したコンテンツが参照する型です。今回はprefabを構成するGameObject、Transform、MeshFilter、MeshRendererが、そのまま保持対象に挙がっています。

Addressablesがコンテンツ側の依存を解析し、アプリのコードから参照がない型でも保持対象に含めていることが分かります。

参考: \[Addressables Build Artifacts\](https://docs.unity3d.com/ja/Packages/com.unity.addressables@1.20/manual/BuildArtifacts.html)（link.xmlの記述は1.20のマニュアルにあります）

### **リフレクションでのコード呼び出しを Source Generator による静的呼び出しに置き換える**

ここまでの対処方法は、どれも必要なコードが削られないように保持を指定するものでした。

これに対して、静的解析から見えない依存を生んでいるリフレクション自体を、\*\*Source Generator\*\*によるコード生成で置き換える方法もあります。

Source Generatorは、C\#コンパイラーがコンパイル時にソースコードを解析し、追加のソースコードを生成する仕組みです。

型の探索と生成をコンパイル時に済ませてしまえば、依存は最初から静的な参照として表現され、保持指定そのものが不要になります。

例として、コマンド名の文字列を受け取り、対応するハンドラーを実行するディスパッチ処理を考えます。

ハンドラーは属性でコマンド名を宣言します。これを実行時にリフレクションで作るなら、属性の付いた型をアセンブリから探して生成することになる構造です（{{list:command_handlers}}）。

CODE START csharp | 属性でコマンド名を宣言するハンドラー {{list:command_handlers}}  
\[AttributeUsage(AttributeTargets.Class)\]  
public sealed class CommandHandlerAttribute : Attribute  
{  
    public CommandHandlerAttribute(string command)  
    {  
        Command \= command;  
    }

    public string Command { get; }  
}

public interface ICommandHandler  
{  
    string Execute();  
}

// ハンドラーはどこからも静的に参照されない  
\[CommandHandler("greet")\]  
public sealed class GreetCommandHandler : ICommandHandler  
{  
    public string Execute() \=\> "GreetCommandHandler.Execute succeeded";  
}

\[CommandHandler("farewell")\]  
public sealed class FarewellCommandHandler : ICommandHandler  
{  
    public string Execute() \=\> "FarewellCommandHandler.Execute succeeded";  
}  
CODE END

2つのハンドラーはどこからも静的に参照されないため、実行時スキャンの作りのままHighでビルドすれば、前節で見たとおり型ごと削除されます。

このコマンド名に対するハンドラーの解決を、Source Generatorを用いて実現します。

\[CommandHandler\]が付いたクラスをコンパイル時に収集し、コマンド文字列から具象型を引くディスパッチャーを生成するジェネレーターを用意します。

ジェネレーターがやることは2段階で、前半（ForAttributeWithMetadataName）が属性付きクラスからのコマンド名と型名の収集、後半（GenerateDispatcher）が集めたペアのテンプレートへの流し込みです。

以下にジェネレーターのコード全文を示します（{{list:dispatcher_generator}}）。

CODE START csharp | ディスパッチャーを生成するSource Generator {{list:dispatcher_generator}}  
\#nullable enable  
using System.Collections.Immutable;  
using System.Linq;  
using System.Text;  
using Microsoft.CodeAnalysis;  
using Microsoft.CodeAnalysis.CSharp;  
using Microsoft.CodeAnalysis.Text;  
namespace StrippingSample.SourceGenerators  
{  
    \[Generator\]  
    public sealed class CommandDispatcherGenerator : IIncrementalGenerator  
    {  
        public void Initialize(IncrementalGeneratorInitializationContext context)  
        {  
            // \[CommandHandler\] が付いたクラスから、コマンド名と型名のペアを収集する  
            var handlers \= context.SyntaxProvider  
                .ForAttributeWithMetadataName(  
                    "CommandHandlerAttribute",  
                    static (\_, \_) \=\> true,  
                    static (syntaxContext, \_) \=\> (  
                        Command: (string)syntaxContext.Attributes\[0\].ConstructorArguments\[0\].Value\!,  
                        TypeName: ((INamedTypeSymbol)syntaxContext.TargetSymbol)  
                            .ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat)))  
                .Collect();  
            // 収集したペアからディスパッチャーを生成する  
            context.RegisterSourceOutput(handlers, static (sourceContext, commandHandlers) \=\>  
            {  
                if (\!commandHandlers.IsDefaultOrEmpty)  
                {  
                    sourceContext.AddSource(  
                        "CommandDispatcher.g.cs",  
                        SourceText.From(GenerateDispatcher(commandHandlers), Encoding.UTF8));  
                }  
            });  
        }  
        private static string GenerateDispatcher(  
            ImmutableArray\<(string Command, string TypeName)\> handlers)  
        {  
            // 出力を決定的にするため、コマンド名で並べ替えてから流し込む  
            var cases \= string.Join("\\n", handlers  
                .Distinct()  
                .OrderBy(static handler \=\> handler.Command, System.StringComparer.Ordinal)  
                .Select(static handler \=\> $$"""  
                            case {{SymbolDisplay.FormatLiteral(handler.Command, true)}}:  
                                return new {{handler.TypeName}}().Execute();  
                """));  
            return $$"""  
                // \<auto-generated/\>  
                public static class CommandDispatcher  
                {  
                    public static string Execute(string command)  
                    {  
                        switch (command)  
                        {  
                {{cases}}  
                            default:  
                                throw new global::System.ArgumentException($"Unknown command: {command}");  
                        }  
                    }  
                }  
                """;  
        }  
    }  
}  
CODE END

コマンド名で並べ替えているのは、同じ入力から同じコードを生成し、ビルドのたびに出力が揺れないようにするためです。

このジェネレーターにより、コンパイル時に下記のようなコードが生成されます（{{list:generated_dispatcher}}）。

CODE START csharp | 生成されるCommandDispatcher {{list:generated_dispatcher}}  
// \<auto-generated/\>  
public static class CommandDispatcher  
{  
    public static string Execute(string command)  
    {  
        switch (command)  
        {  
            case "farewell":  
                return new global::FarewellCommandHandler().Execute();  
            case "greet":  
                return new global::GreetCommandHandler().Execute();  
            default:  
                throw new global::System.ArgumentException($"Unknown command: {command}");  
        }  
    }  
}  
CODE END

実行時に文字列から型を探していた処理が、文字列と具象型の対応表に変わりました。

構築（new）とExecuteの呼び出しがどちらも生成コードに現れるため、型・コンストラクタ・メソッドへの静的な参照が生成コードの中で完結します。

利用側はコマンド文字列を渡すだけで、ハンドラーの具象型を一切参照しません。

リフレクションの実験と同じく、コマンド文字列はstatic readonlyフィールド経由で渡し、UnityLinkerが追跡できない入力のままにしています（{{list:dispatch_probe}}）。

CODE START csharp | 生成されたディスパッチャーの利用側 {{list:dispatch_probe}}  
// コマンド文字列はフィールド経由で渡し、UnityLinker が追跡できない入力にする  
private static readonly string\[\] Commands \= { "farewell", "greet" };

private static void RunGeneratedDispatcher()  
{  
    foreach (var command in Commands)  
    {  
        var result \= CommandDispatcher.Execute(command);  
        Debug.Log($"\[SourceGeneratorProbe\] dispatch command={command}, result={result}");  
    }  
}  
CODE END

これをHighでビルドして実行すると、2つのコマンドの実行に成功することが確認できました。

CODE START text  
\[SourceGeneratorProbe\] dispatch command=farewell, result=FarewellCommandHandler.Execute succeeded  
\[SourceGeneratorProbe\] dispatch command=greet, result=GreetCommandHandler.Execute succeeded  
CODE END

文字列そのものはビルド時に追跡できない入力のままですが、文字列から型への対応が実行時の探索ではなく生成コードのswitchになったため、UnityLinkerからは手書きコードと変わらない通常の静的な参照に見えます。

Mono.Cecilによる確認でも、2つのハンドラーの型・コンストラクタ・ExecuteとCommandDispatcher.Executeが保持されていました。

この置き換えの利点は、ストリッピングへの耐性だけではありません。

\- 依存がUnityLinkerから見えるため、link.xmlや属性による保持指定の保守が不要になる  
\- 実行時のリフレクション探索がなくなり、起動コストと実行時失敗の可能性が減る  
\- 対象の収集がコンパイル時に行われるため、登録漏れや重複をコンパイル時に診断できる

実際のライブラリでも、このような実装を採用しているケースがあります。

シリアライザーでは、MessagePack for C\#がIL2CPP環境向けにコード生成を長く提供しており、現在はSource Generatorベースの生成に移行しています。

DIコンテナーでは、VContainerがSourceGeneratorで注入コードを生成して実行時リフレクションを減らす選択肢を提供しており、前節で見た「コンストラクタが消えて解決に失敗する」症状への対応をSource Generatorによって解決しています。

UnityプロジェクトへのSource Generatorの導入手順（netstandard2.0のDLLをRoslynAnalyzerラベル付きで配置する構成）は、Unityマニュアルを参照してください。

参考: \[Create and use a source generator\](https://docs.unity3d.com/6000.3/Documentation/Manual/create-source-generator.html) / \[MessagePack for C\#\](https://github.com/MessagePack-CSharp/MessagePack-CSharp) / \[MemoryPack\](https://github.com/Cysharp/MemoryPack) / \[VContainer\](https://github.com/hadashiA/VContainer)

# **まとめ**

Managed Code Strippingは、Stripping Levelを上げて終わりにできる最適化ではありません。

本章で見てきたとおり、リフレクションや外部コンテンツを介した依存は静的解析から見えず、実行時には必要なコードでもビルドから消えます。

しかも症状はビルドしたアプリでしか現れないため、見えない依存を明示することと、ビルドしたアプリで検証し続けることを、セットで運用する必要があります。

保持を明示する手段は、対象と状況で使い分けます（{{table:approach_selection}}）。

表: 対処方法の使い分け {{table:approach_selection}}

| 方法 | 適する状況 |
| :---- | :---- |
| \[Preserve\] | 自分で編集できるコードの少数の要素を残す |
| link.xml | 編集できないDLLを含む、決まった対象を残す |
| Require系属性 | フレームワーク側で「実装や派生を残す」契約を宣言する |
| IUnityLinkerProcessor | ビルドターゲットやコンテンツによって保持対象が変わる |
| Source Generator | リフレクションによる探索自体を静的な参照に置き換える |

あわせて、ストリッピングを効きやすくするための方針を挙げます。

* 問題になるケースをのぞき、\[Preserve\]やlink.xmlを過剰に利用しない  
* リフレクションでの型の探索と生成を減らし、明示登録やSource Generatorによる静的な参照への切り替えを検討する  
* コード保持の指定は、できる限り型・メンバー単位まで絞る。アセンブリ全体のpreserve="all"は問題の切り分けに留める  
* アプリと別にビルドするコンテンツだけが使う型の保持は、手作業ではなくAddressablesのlink.xmlを利用したりIUnityLinkerProcessorによる生成といった仕組みを活用する  
* Stripping Levelを上げたビルドを開発中から動かして検証する
