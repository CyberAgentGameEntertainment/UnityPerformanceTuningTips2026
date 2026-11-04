# C# パフォーマンスルール

> 出典: UnityPerformanceTuningBible - tuning_practice_script_csharp

## ルール一覧

### CS01 ホットパスでのLINQ使用
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: Update等ホットパス内の `using System.Linq`, `Where(`, `Select(`, `ToList(`, `ToArray(`, `OrderBy(`
- **grep_pattern**: `using\s+System\.Linq|\.Where\(|\.Select\(|\.ToList\(|\.ToArray\(|\.OrderBy\(|\.Any\(|\.First\(|\.Count\(`
- **理由**: LINQはイテレータ生成・デリゲート・ボクシング等で大量GC.Allocを発生させる（書籍実測: 非LINQ比約19倍遅いケースあり）
- **対処**: forループやSpan操作に置換。CriticalはUpdate系ホットパス内の使用。初期化時・非ホットパスはWarning相当として扱う（rules-overridesのinit緩和と整合）

### CS02 文字列連結・補間
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: `$"`, `string.Format`, 変数を含む`+`連結, `string.Concat`
- **grep_pattern**: `\$"|string\.Format\(|string\.Concat\(|\w\s*\+\s*"|"\s*\+\s*\w`
- **理由**: 毎回新しいstring領域をヒープに確保する。リテラル同士の連結（`"a" + "b"`）はコンパイル時に畳み込まれ無害なので対象外
- **対処**: `StringBuilder`（初期化時のみ）、`Span<char>`、`stackalloc char[]`

### CS03 async/awaitのGC
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `async Task`, `async ValueTask`
- **grep_pattern**: `async\s+(Task|ValueTask)`
- **理由**: 非同期サスペンド時にステートマシンのbox化とTaskオブジェクト生成でヒープ確保が発生する（同期完了パスはValueTaskでAlloc回避可）
- **対処**: UniTask使用（UniTaskはPoolingでGCゼロ達成可能）。同期完了するパスにはasyncを付けずメソッド分割。ホットパスではコールバック方式も検討

### CS04 sealed修飾子の欠如
- **レベル**: Info
- **対象**: コードレビュー
- **検出パターン**: 継承されないクラスに`sealed`がない（手動確認前提のlocator）
- **grep_pattern**: `\bclass\s+\w+[^{]*\{`（sealed/static/abstract宣言は除外して精査）
- **理由**: sealedでメソッド呼び出しがdevirtualize最適化される。ただし近年のIL2CPPは一部自動devirtualizeするため効果は限定的（最適化マークとしての付与推奨）
- **対処**: 継承予定のないクラスにsealedを付与

### CS05 構造体のボクシング
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: structのobjectキャスト、`IEquatable`未実装structのEquals/Dictionary使用
- **grep_pattern**: `\(object\)\s*\w+`（struct宣言全件検出は誤爆過多のため廃止。IEquatable未実装は手動確認）
- **理由**: 値型をobjectに変換するとヒープコピーが発生。IEquatable未実装structはEquals(object)経由でbox化する
- **対処**: ジェネリック制約（`where T : IEquatable<T>`）使用、`IEquatable<T>`実装

### CS06 クロージャキャプチャ
- **レベル**: Critical
- **対象**: コードレビュー
- **検出パターン**: ラムダ式内でローカル変数参照、`() =>`内の外部変数（式形式メンバー`int Foo => _x;`は対象外）
- **grep_pattern**: `=>\s*\{|=>\s*[^{]`（宣言行・式形式メンバーは精査時に除外）
- **理由**: キャプチャクラスがヒープに生成される。メソッドグループ渡し（`Invoke(MyMethod)`）はstaticメソッドでも毎回Delegate Allocationが発生する（UnityのC#バージョンではメソッドグループキャッシュ非対応）
- **対処**: キャプチャなしラムダ（コンパイラがキャッシュ）、デリゲートの事前キャッシュ、引数渡し。メソッドグループの直接渡しは避ける

### CS07 params配列
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `params T[]`, `params object[]`
- **grep_pattern**: `\bparams\s+\w+\[\]`
- **理由**: 呼び出し毎に配列がヒープ確保される
- **対処**: オーバーロード（引数1-4個）、`Span<T>`

### CS08 Dictionary/Listの初期容量未指定
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `new Dictionary<`, `new List<` で容量引数なし
- **grep_pattern**: `new\s+(Dictionary|List|HashSet)<[^>]+>\(\s*\)`
- **理由**: リサイズ時に旧バッファが破棄され追加GCが発生
- **対処**: 予測可能なサイズで初期容量を指定

### CS09 foreachによるアロケーション
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `foreach`（カスタムコレクション対象。List/Array/Dictionaryは対象外）
- **grep_pattern**: `\bforeach\s*\(`
- **理由**: カスタムコレクションのGetEnumerator()がIEnumeratorボクシングを起こす場合がある。標準コレクション(List/Array等)はstruct enumeratorのためGCなし
- **対処**: カスタムコレクションはstruct enumerator実装を確認、未実装ならforループ使用

### CS10 stackalloc未活用
- **レベル**: Info
- **対象**: コードレビュー
- **検出パターン**: 小さな一時配列の`new T[]`
- **grep_pattern**: `new\s+(byte|int|float|char|double)\[\s*\d+\s*\]`
- **理由**: スタックに確保すればGCを完全回避できる
- **対処**: `Span<T>` + `stackalloc`（512byte以下推奨）

### CS11 静的フィールドの不適切な使用
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: クリアされず成長し続ける`static List<`, `static Dictionary<`（手動確認前提のlocator。static readonlyの固定キャッシュは正当なパターン）
- **grep_pattern**: `static\s+(readonly\s+)?(List|Dictionary|HashSet)<`
- **理由**: GCルートとなりメモリリークの原因
- **対処**: 明示的なクリア処理、WeakReference検討

### CS12 例外スローの多用
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: ホットパス内の`throw new`（コールドパスの引数検証throwは正当な実践のため対象外）
- **grep_pattern**: `throw\s+new\s+\w+Exception`
- **理由**: スロー時のコストが高い（例外オブジェクト生成+スタックトレース取得+スタックアンワインド。定義自体は無害）
- **対処**: ホットパスではTryPatternで事前チェック、エラーコード返却

### CS13 不要なToString()
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `.ToString()`（特に値型）
- **grep_pattern**: `\.ToString\(\)`
- **理由**: 毎回文字列をヒープ確保する。enumやToString未オーバーライドのstructではボクシングも発生（int/float等のオーバーライド済み値型はconstrained callでbox化しない）
- **対処**: ランタイムでの文字列変換を回避

### CS14 Enum操作のボクシング
- **レベル**: Warning
- **対象**: コードレビュー
- **検出パターン**: `enum.ToString()`, ホットパスの`HasFlag`
- **grep_pattern**: `\benum\b[^{]*\.ToString\(|\.HasFlag\(`
- **理由**: enumのToString()はボクシング+文字列GC。HasFlagのボクシング有無はUnityのMono/IL2CPPバージョン依存（Unity 6もMono/IL2CPPでありCoreCLRではない）。Dictionary<Enum,T>のGetHashCodeボクシングは旧Unity（2018より前）の問題で現行はほぼ解消済み
- **対処**: ToString回避（事前キャッシュ）。HasFlagはビット演算（`(flags & flag) != 0`）で代替するかProfilerで実測して判断

### CS15 構造体のコピーコスト
- **レベル**: Info
- **対象**: コードレビュー
- **検出パターン**: 大きなstruct（16byte超）の値渡し（サイズはgrepで判定不能、手動確認前提のlocator）
- **grep_pattern**: `\bstruct\s+\w+`
- **理由**: 値型は代入・引数渡しでコピーが発生
- **対処**: `in`修飾子（defensive copy回避のため`readonly struct`と併用）、`ref`渡し
