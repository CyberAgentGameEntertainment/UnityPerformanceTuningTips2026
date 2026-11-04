# **iOSのメモリ上限と仮想アドレス空間**

名前 / ローマ字：キム ミンヒョク / Minhyuk Kim  
所属：株式会社サイバーエージェント SGEコア技術本部

ゲームが使える端末のメモリには当然ながら上限があります。シーン切り替え中にアセットが重なったり、一時的な描画バッファが増えたりすると、この上限が問題になります。

ゲーム内でメモリのピークを減らすことに加え、メモリの上限にも調整の余地があります。Xcodeの「Increased Memory Limit」は、対応端末でその上限を引き上げる設定です。別の設定である「Extended Virtual Addressing」は、プロセスが使える仮想アドレス空間を広げます。名前は似ていますが、対処できる領域が異なります。

本章では、2つの設定の使い分けと実機での効果、App Storeでの運用経験を紹介します。数値は筆者の実機計測によるもので、Appleが保証する端末別上限ではありません。

# **メモリ上限と仮想アドレス空間は別の制約**

アプリがOOM（Out of Memory）になったとき、少なくとも次の2つは分けて考える必要があります。

表: iOSアプリで区別したい2種類のメモリ制約 {{table:memory_constraints}}

| 制約 | 主に見る値 | 代表的な現れ方 |
| :---- | :---- | :---- |
| 物理メモリ側 | `phys_footprint` | 高メモリ使用でシステムに終了される（jetsam） |
| 仮想アドレス空間側 | VM Region Summary、Virtual Size | メモリ確保が失敗し、その後にクラッシュする |

物理メモリは、実際にRAMへ保持する必要があるページに関係します。iOSでは、アプリが使える物理メモリ量は端末の搭載RAMそのものではなく、その時点のシステム状態やアプリに適用された上限にも左右されます。

Appleの`os_proc_available_memory()`は、現在のアプリがメモリ上限に達するまでに割り当てられる量を返します。Appleの説明では、この戻り値は「現在のメモリ上限からアプリのmemory footprintを引いた値」に対応し、端末の空き物理メモリと同じとは限りません。また、アプリのライフサイクル中にも上限は変わり得るため、値はキャッシュせず参考情報として扱う必要があります{{fn:os_proc_available_memory}}。

仮想アドレス空間は、メモリやファイルをどのアドレスに対応付けるかを管理する範囲です。たとえば、大きなファイルを`mmap`しても、その全ページがただちにRAMへ読み込まれるとは限りません。しかし、その範囲を置くための仮想アドレスは必要です。逆に、物理メモリのfootprintがまだ低く、`os_proc_available_memory()`にも余裕があるのに、新しいアドレス範囲を確保できず`malloc`などが失敗することがあります。

したがって、{{table:memory_constraints}}の2種類の制約を区別するためには、jetsam reportが残っているのか、通常のクラッシュレポートで割り当て失敗が起点になっているのかを確認する必要があります。
`os_proc_available_memory()`が大きな値を返していても、仮想アドレス空間に同じだけ余裕があるとは限りません。

# **2つのEntitlementの役割**

## **Increased Memory Limit**

Increased Memory Limitのキーは`com.apple.developer.kernel.increased-memory-limit`です。Appleは、対応する端末でデフォルトのアプリメモリ上限を超えることにより、アプリの中核機能が改善し得ることをシステムへ伝えるEntitlementと説明しています{{fn:increased_memory_limit}}。XcodeのCapability一覧でも、{{fig:xcode_increased_memory_limit}}のように「対応端末で」という条件付きの説明が表示されます。

![][image1]{scale=0.8}
図: XcodeのCapability一覧に表示されるIncreased Memory Limit {{fig:xcode_increased_memory_limit}}

つまり、指定すれば全端末で一定量が加算されるわけではありません。Appleのドキュメントにも、増加した上限を利用できるのは一部の端末だけであり、追加メモリを利用できない場合でもアプリが正しく動作するように求める記述があります。

### **Increased Memory Limitの実機計測**

表: Increased Memory Limitの有無によるOOM時のメモリ計測結果（iPhone） {{table:increased_memory_observations}}

| 搭載RAM | 無効時 | 有効時 |
| :---- | ----: | ----: |
| 3GB | 約1,850MB | 約1,850MB |
| 4GB | 約2,050MB | 約2,290MB |
| 6GB | 約3GB | 約4GB |

4GBと6GBのiPhoneではメモリがそれぞれ増えた一方、3GBのiPhoneでは有効と無効の差がありませんでした。
また、iPadではそもそもOOM時のメモリがiPhoneとは異なるため、有効時の結果も異なる可能性が高いです。
もし対象ユーザーの端末割合に3GBのiPhoneが多い場合、Increased Memory Limitを付けるだけで救済できると見込むべきではありません。

## **Extended Virtual Addressing**

Extended Virtual Addressingのキーは`com.apple.developer.kernel.extended-virtual-addressing`です。Appleは、より大きなアドレス空間へアクセスできることを示すBoolean値と説明しています。例としてゲームを挙げています{{fn:extended_virtual_addressing}}。

![][image2]{scale=0.8}
図: XcodeのCapability一覧に表示されるExtended Virtual Addressing {{fig:xcode_extended_virtual_addressing}}

この設定はRAMを増やしません。物理メモリ上限に近づいてjetsamされるアプリに対して、Extended Virtual Addressingだけを付けても、同じ量のRAMを使い続けられる保証はありません。効果が期待できるのは、大きなメモリマップ、断片化したアドレス領域、大きな固定予約などにより、物理メモリを使い切る前に仮想アドレス空間が先に窮屈になっている場合です。

有効にすること自体のコストはほぼありません。ページテーブルは実際にマップした範囲にだけ作られるため、上限を広げても増えません{{fn:xnu_pmap}}。考えるべきは、効果が出る端末があるかどうかです。Increased Memory Limitを付けた時点でアドレス空間も広がるので、両方を付けても`max_address`が変わらない端末があります。

一方、{{table:increased_memory_observations}}の3GB端末のようにIncreased Memory Limitの効果が現れない端末では、Extended Virtual Addressingがアドレス空間を広げる唯一の手段になります。Increased Memory Limitは端末が対応していなければ署名が正しくても無効になりますが、Extended Virtual Addressingにこの条件はありません。

### **仮想アドレス空間の上限値**

Extended Virtual Addressingが広げるのは、footprintの上限とは別のアドレス空間の上限です。この上限は、`pmap_max_64bit_offset()`が搭載メモリに応じて返す{{table:va_ceilings}}のいずれかの値で決まります{{fn:xnu_pmap}}。

表: arm64ユーザプロセスのアドレス空間上限 {{table:va_ceilings}}

| 条件 | 定数 | 上限 |
| :---- | :---- | ----: |
| `max_mem` > 3GB | `ARM64_MAX_OFFSET_DEVICE_LARGE` | 17.375GB |
| `max_mem` 1GB超〜3GB | `ARM64_MAX_OFFSET_DEVICE_SMALL` | 13.375GB |
| `max_mem` 1GB以下 | `ARM64_MIN_MAX_ADDRESS` | 12.5GB |
| entitlementsを含むいずれかの手段で拡張 | `MACH_VM_JUMBO_ADDRESS` | 63GB |

`max_mem`はカーネルが認識する物理メモリで、カタログ上のRAM容量より小さくなります。
いずれもカーネルのコンパイル時定数なので、OSのバージョンごとに変わる可能性があります。

## **App Storeでの運用実績**

これらのEntitlementsの採用にあたって気になるのが、App Storeの審査に影響を与えないかという点です。筆者が把握する複数の運用中タイトルには、Increased Memory Limit、Extended Virtual Addressingのいずれも、設定を有効にしてApp Storeの審査を通過し、配信している実績があります。

Apple自身も、ゲーム開発者向けの「Bring your high-end game to iPhone 15 Pro」{{fn:apple_game_memory}}で、利用可能なメモリを増やすためにIncreased Memory Limitを有効にするよう案内しています。この2つは製品での採用を検討できる公開機能です。個別のアプリ全体の審査結果を保証するものではありませんが、「メモリ上限に関わる設定だから、審査で説明が求められたり、審査で落ちるのではないか」という心配で候補から外す必要はありません。

# **App IDと開発者アカウントを確認する**

## **ワイルドカードのプロファイルは使えない**

これら2つのEntitlementは、ワイルドカード形式のApp IDでは有効にできません。
`com.example.*`のように複数アプリをまとめて署名している構成の場合は、
ゲームのBundle IDと一致するExplicit App IDに切り替える必要があります。

このまま署名しようとすると、XcodeのSigning & Capabilitiesに{{fig:xcode_wildcard_profile_error}}のエラーが出ます。Capability名とEntitlementのキーが別々に報告される点に注目してください。プロファイル側がCapabilityを含んでいないことと、キーに対応する許可がないことの両方が指摘されています。

![][image3]
図: ワイルドカードのProvisioning Profileで署名しようとしたときのエラー {{fig:xcode_wildcard_profile_error}}

Apple DeveloperのCertificates, Identifiers & Profilesで対象のApp IDを選び、{{fig:appid_increased_memory_limit}}のように必要なCapabilityを有効にします。Appleも、App IDに設定したCapabilityがアプリの利用できる機能の許可リストになり、Explicit App IDが必要な機能はワイルドカードでは選べないと説明しています{{fn:explicit_app_id}}。

![][image4]
図: App IDの設定でIncreased Memory Limitを有効にする {{fig:appid_increased_memory_limit}}

手動署名では、Capabilityを変更したあとにProvisioning Profileを再生成し、ビルドで使うものを更新します。自動署名では、正しいTeamとBundle IDで更新されたプロファイルが選ばれることを確認します。古いプロファイルのまま`.entitlements`へキーだけ追加しても、要求する権限と署名側の許可が一致しません。Unity側の設定を調べる前に、まずここを揃えます。

## **Extended Virtual Addressingには有料メンバーシップが必要**

Extended Virtual Addressingは、無料のApple AccountをXcodeのPersonal Teamとして使う構成では有効にできません。
AppleのCapability対応表で対象となっているのは
Apple Developer ProgramとApple Developer Enterprise Programのメンバーシップで、
無料のApple Accountは含まれていません{{fn:capability_membership}}。

一方、Increased Memory LimitはPersonal Teamでも有効にできます。

# **Unityのビルド後処理で自動設定する**

Entitlementsファイルに入る最小の内容は{{list:memory_entitlements}}です。両方を同時に有効にする例です。

CODE START xml | メモリ関連Entitlementの最小構成 {{list:memory_entitlements}}
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.developer.kernel.increased-memory-limit</key>
    <true/>
    <key>com.apple.developer.kernel.extended-virtual-addressing</key>
    <true/>
</dict>
</plist>
CODE END

XcodeのSigning & Capabilitiesから手で追加することもできますが、書き出しのたびに付け直すことになり、設定の抜けたビルドが混ざりやすくなります。Unityではビルド後処理から、Entitlementsファイルと、それを参照する`CODE_SIGN_ENTITLEMENTS`をまとめて更新できます。処理が走るのは署名前のビルド時で、アプリの起動後にC#から権限を取得するわけではありません。

## **Unity 6000.0.11f1以降では専用APIを使う**

Unity 6000.0.11f1で、`ProjectCapabilityManager`に次のメソッドが追加されました{{fn:unity_memory_api}}。

* `AddIncreasedMemoryLimit()`
* `AddExtendedVirtualAddressing()`

これらを使えば、EntitlementsへのBoolean値の追加とXcodeプロジェクトへの反映をまとめて行えます。

対応する`PBXCapabilityType`の定義も用意されています。ただし、これはC#のenumではなく、Capabilityの要件を表すクラスです。`PBXProject.AddCapability()`へこの値を渡すことと、Entitlementsへ必要なキーを追加することは同じではありません。そのため基本的には両方を処理する`ProjectCapabilityManager`を使います。

以下は、`ProjectCapabilityManager`を使いCapabilityを追加するサンプルです。

CODE START csharp | Unity 6000.0.11f1以降でメモリCapabilityを追加するビルド後処理 {{list:unity_memory_postprocess}}
#if UNITY_IOS
using UnityEditor;
using UnityEditor.Callbacks;
using UnityEditor.iOS.Xcode;

static class IosMemoryCapabilities
{
    [PostProcessBuild]
    static void OnPostprocessBuild(BuildTarget target, string buildPath)
    {
        if (target != BuildTarget.iOS) return;

        string pbxPath = PBXProject.GetPBXProjectPath(buildPath);
        var pbx = new PBXProject();
        pbx.ReadFromFile(pbxPath);
        string mainTarget = pbx.GetUnityMainTargetGuid();

        string entitlements = pbx.GetBuildPropertyForAnyConfig(
            mainTarget, "CODE_SIGN_ENTITLEMENTS");
        if (string.IsNullOrEmpty(entitlements))
            entitlements = "Unity-iPhone.entitlements";

        var capabilities = new ProjectCapabilityManager(
            pbxPath, entitlements, targetGuid: mainTarget);
        capabilities.AddIncreasedMemoryLimit();
        capabilities.AddExtendedVirtualAddressing();
        capabilities.WriteToFile();
    }
}
#endif
CODE END

`UnityEditor.iOS.Xcode`はiOS Build Supportに含まれるため、`#if UNITY_IOS`で囲みます。そうしないと、このモジュールが入っていない環境でEditor全体のコンパイルが通らなくなりますので注意してください。

このサンプルは、全ビルド構成で共通のEntitlementsファイルを使い、そのパスをXcodeプロジェクトからの相対パスで指定する構成を対象にしています。DebugとReleaseでファイルを分けている場合や、パスにビルド変数を含めている場合は、各構成の実際のファイルへ反映する処理に合わせてください。

最後の保存は`capabilities.WriteToFile()`で行います。ほかのビルド後処理でXcodeプロジェクトを編集する場合、
`pbx`を保存し直すと、Capability Managerが保存した変更を上書きする可能性があるため、
更新後のファイルを読み直してから変更するよう気をつけてください。

## **専用APIがないバージョンではplistを直接更新する**

専用APIがないEditorで同じ処理を行うには、{{list:unity_memory_postprocess}}の`ProjectCapabilityManager`を生成してから保存するまでの5行を、{{list:unity_memory_legacy}}に置き換えます。

CODE START csharp | 専用Capability APIがないUnityでの代替処理 {{list:unity_memory_legacy}}
string fullPath = System.IO.Path.Combine(buildPath, entitlements);
System.IO.Directory.CreateDirectory(
    System.IO.Path.GetDirectoryName(fullPath));
var plist = new PlistDocument();
if (System.IO.File.Exists(fullPath))
    plist.ReadFromFile(fullPath);

plist.root.SetBoolean(
    "com.apple.developer.kernel.increased-memory-limit", true);
plist.root.SetBoolean(
    "com.apple.developer.kernel.extended-virtual-addressing", true);
plist.WriteToFile(fullPath);

if (string.IsNullOrEmpty(pbx.FindFileGuidByProjectPath(entitlements)))
    pbx.AddFile(entitlements, entitlements);
pbx.SetBuildProperty(mainTarget, "CODE_SIGN_ENTITLEMENTS", entitlements);
pbx.WriteToFile(pbxPath);
CODE END

既存ファイルを読み込んでから2つのキーを追加することで、Push通知などのEntitlementを消さずに更新します。変更するのは`.entitlements`であり、`Info.plist`ではありません。また、ファイルは`CODE_SIGN_ENTITLEMENTS`から参照させて初めて署名に使われます。

C#コードを追加しただけでアカウント側の権限まで有効になるわけではありません。
いずれの方法でもApple Developer側のApp IDやProvisioning Profileは別途設定が必要です。

# **適用を2段階で確認する**

設定が反映されているかは、署名にキーが入っているかと、実機でカーネルが反映したかの2段階に分けて確認します。

## **署名に含まれているかを確認する**

手作業でもビルド後処理でも、{{list:memory_entitlements}}のファイルがプロジェクト内にできただけでは、実機へ入ったアプリに反映された証拠にはなりません。Entitlementは最終的にコード署名へ埋め込まれます。書き出した`.ipa`はzip形式なので、展開して`Payload`内のアプリ本体に対して{{list:codesign_entitlements}}を実行します。

CODE START bash | 署名済みアプリに埋め込まれたEntitlementの確認 {{list:codesign_entitlements}}
unzip -q YourApp.ipa -d ipa
codesign --display --entitlements :- ipa/Payload/YourApp.app
CODE END

出力されたplistで、{{list:memory_entitlements}}の2つのキーが入っているかを確認します。Appleも、このコマンドによる署名後の確認を案内しています{{fn:diagnosing_entitlements}}。

## **実機で上限が変わったかを確認する**

{{list:codesign_entitlements}}でキーが見えても、カーネルが実際に反映したとは限りません。Increased Memory Limitは端末側が対応していない場合、署名が正しくても無効として扱われます。

実機での適用は、カーネルが自プロセスに対して持っている値を直接読んで判断します。それを取得するのが`task_info()`で、iOS SDKに標準で入っているMach APIです（`<mach/mach.h>`）。C#からは呼べないので、`Assets/Plugins/iOS/`に置いた`.mm`から読み出してC#へ返します。

CODE START objc | 適用状況を読むネイティブプラグイン {{list:task_vm_info_probe}}
#import <mach/mach.h>

extern "C" void UPT_ReadLimits(uint64_t *memlimit, uint64_t *ceiling)
{
    task_vm_info_data_t i;
    mach_msg_type_number_t c = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&i, &c) != KERN_SUCCESS) {
        return;
    }

    // footprint上限（Increased Memory Limit）
    *memlimit = i.limit_bytes_remaining + i.phys_footprint;
    // アドレス空間上限: 0xF_C000_0000まで広がっていれば拡張済み
    *ceiling  = i.max_address;
}
CODE END

C#側は`DllImport("__Internal")`で`UPT_ReadLimits`を宣言して呼び出します。

見るべき値は上限です。`os_proc_available_memory()`が返す残量はfootprintが増えればそのぶん小さくなるので、値だけを見ても上限が上がっていないのか単に使い込んでいるだけなのかを区別できません。

`limit_bytes_remaining`はその`os_proc_available_memory()`とカーネル内で同じ関数から得られる値なので{{fn:xnu_memorystatus}}、現在のfootprintを足せば適用中の上限に戻せます。アドレス空間側の`max_address`も同じ構造体から取れるため、2つのEntitlementを1回の呼び出しで確認できます。`max_address`は`TASK_VM_INFO_COUNT`を渡さないと埋まりません。

ただし`max_address`からExtended Virtual Addressing単独の効果は分離できません。Increased Memory Limitを付けたビルドや、XcodeのEnhanced Securityを有効にしたビルドでも同じ上限になります。確認できるのは「上限が63GBまで広がっているか」までです。

# **iOS 26.4で顕在化した仮想アドレス予約**

## **`JS VM Gigacage (reserved)`は1GBのRAM使用量ではない**

iOS 26.4を搭載した3GBのiPhoneでの検証では、クラッシュレポートのVM Region Summaryに`JS VM Gigacage (reserved)`として約1GBの領域が現れ、Extended Virtual Addressingを有効にするとOOM時のメモリ量の低下を防げました。
この領域は本来のGigacageではなく、既存のVM tag名を再利用して表示されたJavaScriptCoreのStructure Heap予約です{{fn:apple_forum_826325}}。

## **WebKitの修正は予約量を端末RAMに応じて縮小する**

JavaScriptCoreの公開バグ`315240`には、低メモリ端末でStructure Heapの予約が仮想アドレス空間の大きな割合を占めるため、物理メモリ量に応じて予約を縮小する方針が記録されています{{fn:webkit_bug_315240}}。

この問題に対応するWebKitのコミットで、iOS系プラットフォームについて、基本の予約サイズと物理メモリの20%の小さい方を取り、
さらに2のべき乗へ切り下げて、Structure Heapの希望予約サイズを決めるようになりました。
公開コミットの説明では、3GB端末で512MBになる例が示されています{{fn:webkit_commit_structure_heap}}。

この対応が入ったiOS 26.6.1のiPhone SE（第2世代、RAM 3GB）で計測すると、表示上の予約が約1GBから512MBへ縮小し、
26.4で低下していたOOM時のメモリ量が、Extended Virtual Addressingを有効にしなくても解消されていました。

# **まとめ**

Increased Memory LimitとExtended Virtual Addressingは、有効にしても失うものがほとんどない設定です。ページテーブルは実際にマップした範囲にだけ作られるため、アドレス空間を広げてもコストは増えません。App Storeの審査を通過して配信している実績もあります。Explicit App IDとプロファイルの準備さえできれば、基本的に両方を有効にしておくのがよいと考えています。

ただし、効果は端末に依存します。筆者のiPhoneでの計測では、Increased Memory Limitによる余裕は4GB端末で約240MB、6GB端末で約1GBでしたが、3GB端末では差がありませんでした。iPadでは別途計測が必要です。Extended Virtual Addressingは物理メモリを増やさず、仮想アドレス空間が先に足りなくなる場合にだけ効果があります。iOS 26.4で見られたStructure Heap予約の問題はiOS 26.6系で緩和されましたが、古いOSをサポートする場合や、アプリ自身のメモリマップでアドレス空間を多く使う場合には引き続き効果が見込めます。

どちらもメモリ使用量そのものは減らしません。追加の上限がない端末でも動作を続けられるよう、テクスチャや一時RenderTextureのピーク、アセットの常駐期間、ネイティブプラグインの大きなallocation、ファイルマッピングの寿命を減らす作業は別に必要です。2つのEntitlementは、その上で残る余裕を広げるための設定として使います。

脚注: {{fn:increased_memory_limit}} Apple Developer Documentation, `com.apple.developer.kernel.increased-memory-limit` [https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.increased-memory-limit](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.increased-memory-limit)

脚注: {{fn:extended_virtual_addressing}} Apple Developer Documentation, Extended Virtual Addressing Entitlement [https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.extended-virtual-addressing](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.extended-virtual-addressing)

脚注: {{fn:os_proc_available_memory}} Apple Developer Documentation, `os_proc_available_memory` [https://developer.apple.com/documentation/os/os_proc_available_memory](https://developer.apple.com/documentation/os/os_proc_available_memory)

脚注: {{fn:diagnosing_entitlements}} Apple Developer Documentation, Diagnosing Issues with Entitlements [https://developer.apple.com/documentation/bundleresources/diagnosing-issues-with-entitlements](https://developer.apple.com/documentation/bundleresources/diagnosing-issues-with-entitlements)

脚注: {{fn:apple_forum_826325}} Apple Developer Forums, iPadOS 26.4+ significantly reduced per-app memory limit from 6GB to 3GB on 8GB iPad, breaking memory-intensive apps [https://developer.apple.com/forums/thread/826325](https://developer.apple.com/forums/thread/826325)

脚注: {{fn:webkit_bug_315240}} WebKit Bugzilla 315240, Scale down StructureAlignedMemoryAllocator reservation in proportion to phys-mem size [https://bugs.webkit.org/show_bug.cgi?id=315240](https://bugs.webkit.org/show_bug.cgi?id=315240)

脚注: {{fn:webkit_commit_structure_heap}} WebKit commit `cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b`, Scale down structure-heap reservation size with physical memory [https://github.com/WebKit/WebKit/commit/cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b](https://github.com/WebKit/WebKit/commit/cffeaa6ccbf42e8d5ff626cc9806f7eac3c66e6b)

脚注: {{fn:apple_game_memory}} Apple Developer Tech Talks, Bring your high-end game to iPhone 15 Pro [https://developer.apple.com/videos/play/tech-talks/111372/](https://developer.apple.com/videos/play/tech-talks/111372/)

脚注: {{fn:explicit_app_id}} Apple Developer, Register an App ID [https://developer.apple.com/help/account/identifiers/register-an-app-id/](https://developer.apple.com/help/account/identifiers/register-an-app-id/)

脚注: {{fn:capability_membership}} Apple Developer, Supported capabilities (iOS) [https://developer.apple.com/help/account/reference/supported-capabilities-ios/](https://developer.apple.com/help/account/reference/supported-capabilities-ios/)

脚注: {{fn:unity_memory_api}} Unity 6.0 Scripting API, ProjectCapabilityManager [https://docs.unity3d.com/6000.0/Documentation/ScriptReference/iOS.Xcode.ProjectCapabilityManager.html](https://docs.unity3d.com/6000.0/Documentation/ScriptReference/iOS.Xcode.ProjectCapabilityManager.html)

脚注: {{fn:unity_capability_release}} Unity 6000.0.11f1 Release Notes, iOS: Updated capability list（UUM-65394）。APIの追加内容は、公式配布の6000.0.10f1と6000.0.11f1のiOS Build Support内にあるUnityEditor.iOS.Extensions.Xcode.dllを比較して確認。[https://unity.com/releases/editor/whats-new/6000.0.11f1](https://unity.com/releases/editor/whats-new/6000.0.11f1)

脚注: {{fn:xnu_memorystatus}} Apple Open Source, xnu-12377.121.6, `bsd/kern/kern_memorystatus.c`（`memorystatus_available_memory_internal`） [https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/bsd/kern/kern_memorystatus.c](https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/bsd/kern/kern_memorystatus.c)

脚注: {{fn:xnu_pmap}} Apple Open Source, xnu-12377.121.6, `osfmk/arm/pmap/pmap.c`（`pmap_max_64bit_offset`） [https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/osfmk/arm/pmap/pmap.c](https://github.com/apple-oss-distributions/xnu/blob/xnu-12377.121.6/osfmk/arm/pmap/pmap.c)

[image1]: assets/kim-minhyuk-2/media/image1.png
[image2]: assets/kim-minhyuk-2/media/image2.png
[image3]: assets/kim-minhyuk-2/media/image3.png
[image4]: assets/kim-minhyuk-2/media/image4.png
