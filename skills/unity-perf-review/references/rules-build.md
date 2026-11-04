# ビルド設定 パフォーマンスルール

> 出典: UnityPerformanceTuningBible 2026 - リンク時最適化(LTO)
> 対象: IL2CPP。公式トグル/Master内蔵LTOはUnity 6000.5+、手動thinルートはそれ以前でも可（6000.3.15f1で実測）。実測はPixel 8a / iPhone 13ベース。
> Managed Code Stripping系のルールは rules-build-stripping.md を参照。

## ルール一覧

### BD01 公式LTOトグル（engine ThinLTO）の有効化
- **レベル**: Warning
- **対象**: ビルド設定 / ProjectSettings
- **適用条件**: il2cpp, android, unity>=6000.5
- **検出パターン**: Android Player SettingsのLink Time OptimizationがThin
- **grep_pattern**: `LinkTimeOptimization"[: ]*"Thin"`（対象: `Library/Bee/Player*-inputdata.json`）
- **理由**: 効果は`libunity.so`のみで、実測13ワークロード中はっきり効いたのは3本（gfx -13.93% / cull -9.35% / text -3.61%、合成幾何平均-2.46%）。Unity公開値（5プロジェクト×コールドスタート10回）もCPU 13.88ms→13.36ms（-3.7%）/ 初期表示247.96ms→229.90ms（-7.3%）と桁は一致するが、公開値のAPK+3.3%はStrip Engine Code無効構成どうしの比較で前提コストを含まない。前提のStrip Engine Code無効化に配信APK+5.67MB / インストール+20.3MB / 回収されにくい常駐+8.0MBを払う
- **対処**: 無効化を基本とする。もともとStrip Engine Code無効で、C#から極小ネイティブ関数を大量に叩く経路がボトルネックの場合のみ再検討

### BD02 Strip Engine Codeの無効化
- **レベル**: Warning
- **対象**: ProjectSettings
- **適用条件**: il2cpp
- **検出パターン**: `ProjectSettings/ProjectSettings.asset`の`stripEngineCode: 0`
- **grep_pattern**: `stripEngineCode:\s*0`
- **理由**: 実測（Android / Pixel 8a）で非圧縮.so合計+14.65MB / APK+5.67MB / インストール+20.3MB / RSS+18.0MB（うち回収されにくい領域+8.0MB）。マネージド側エンジンモジュールのストリップも無くなるためlibil2cpp.soも3.89MB増える。iOSへの数値の外挿は不可。なおStrip Engine Codeはネイティブコード対象で、Managed Code Stripping（managedStrippingLevel、BD12-BD18）とは別機能
- **対処**: 出荷構成では有効を維持。無効化する場合はサイズ・メモリ増を先に評価

### BD03 SetAdditionalIl2CppArgsの上書き（退避・復元なし）
- **レベル**: Critical
- **対象**: コードレビュー（Editorビルドスクリプト）
- **適用条件**: il2cpp
- **検出パターン**: `PlayerSettings.SetAdditionalIl2CppArgs`を、`GetAdditionalIl2CppArgs`での退避とfinallyでの復元なしに呼んでいる
- **grep_pattern**: `SetAdditionalIl2CppArgs`（Get→Set→finally復元の3点セットを確認）
- **理由**: SetAdditionalIl2CppArgsは「追記」ではなく「置換」。既存の追加引数をサイレントに消し、設定がプロジェクトに永続化される
- **対処**: `GetAdditionalIl2CppArgs()`で退避 → 既存値に追記 → finallyで必ず元に戻す

### BD04 「性能のため」のMaster構成（Android 6000.5+）
- **レベル**: Info
- **対象**: ビルド設定 / ProjectSettings
- **適用条件**: il2cpp, android, unity>=6000.5
- **検出パターン**: AndroidのC++ Compiler Configuration = Master
- **grep_pattern**: `il2cppCompilerConfiguration`（値の解釈は手動確認）
- **理由**: MasterのCPU優位は内蔵ThinLTO由来（2×2実測: -O2単独-5.9%、ThinLTOありはどちらも-20.9%）。Release+手動thinは同一CPU時間でlibil2cpp.soが約13.5%小さく、コールドビルドも71s対86s
- **対処**: 性能目的ならRelease構成 + `--compiler-flags=-flto=thin --linker-flags=-flto=thin`（手動thin）を検討

### BD05 Full LTOの指定
- **レベル**: Critical
- **対象**: ビルド設定 / コードレビュー
- **適用条件**: il2cpp
- **検出パターン**: IL2CPP追加引数に`-flto`（`=thin`なし）または`-flto=full`
- **grep_pattern**: `-flto(?!=thin)`（PCRE、`rg -P`使用）
- **理由**: 実タイトル規模（libil2cpp.so 189MB）でモノリシックリンクがメモリ20GBに達しOOMで完走不能（48分・2,903sのビルドの末に失敗）。合成ベンチでも性能はThinLTOと同等なのにコールドビルド71s→247s（約3.5倍）
- **対処**: ThinLTO（`-flto=thin`）を使う。実タイトル規模でFull LTOを選ぶ利点は確認されていない

### BD06 -O3 + ThinLTOの併用
- **レベル**: Warning
- **対象**: ビルド設定 / コードレビュー
- **適用条件**: il2cpp, android
- **検出パターン**: IL2CPPのcompiler-flagsに`-O3`をThinLTOと併せて指定（ThinLTO併用かはgrep後に手動確認）
- **grep_pattern**: `compiler-flags.*-O3`（クォート位置の揺れに対応するため広めに取り精査で絞る）
- **理由**: 実測で性能差0.3%（測定可能な追加効果なし）に対しlibil2cpp.soは+23.4%（38.16→47.08MB）
- **対処**: `-O3`をやめThinLTO単独（AndroidのReleaseは`-Os`が既定。iOSのReleaseは`-O3`が既定のため本ルールの前提が異なる）にする

### BD07 -fwhole-program-vtables（whole-program devirtualization）の指定
- **レベル**: Info
- **対象**: コードレビュー / ビルド設定
- **適用条件**: il2cpp
- **検出パターン**: IL2CPP追加引数に`-fwhole-program-vtables`（`-fsplit-lto-unit`はこのときclangが自動付加されるもので、単独の手動指定も同様に効果なし）
- **grep_pattern**: `-f(whole-program-vtables|split-lto-unit)`
- **理由**: 実測（Pixel 8a・12セッション）で追加のCPU効果なし。サイズはlibil2cpp.so +40B / `.text` -400B、コールドビルドはThinLTO比±0。IL2CPP生成C++には`virtual`も多態継承もなく、マネージドの仮想・interface呼び出しはランタイムのメソッドテーブルを使うため対象外。効果はランタイム/libc++の少数のネイティブC++ vtableに限られる
- **対処**: フラグを外してビルド設定を単純化。間接呼び出し削減はC#側（sealed、具象型受け、ジェネリクスによる静的ディスパッチ）で対処

### BD08 -import-instr-limitのチューニング
- **レベル**: Info
- **対象**: コードレビュー / ビルド設定
- **適用条件**: il2cpp
- **検出パターン**: IL2CPP追加引数に`-import-instr-limit=N`
- **grep_pattern**: `import-instr-limit=\d+`
- **理由**: 30/100（clang既定）/300を実測し、性能・サイズ・ビルド時間とも測定可能な差なし
- **対処**: 既定値のままにしフラグを削除してビルド設定を単純化

### BD09 iOSへのThinLTO適用
- **レベル**: Warning
- **対象**: ビルド設定 / CI設定
- **適用条件**: il2cpp, ios
- **検出パターン**: iOSビルドでIL2CPP追加引数に`-flto=thin`
- **grep_pattern**: `-flto=thin`（iOSビルド分岐内。プラットフォーム判定は手動確認）
- **理由**: ThinLTO自体はiOSでも機能する（iPhone 13の検証用ベンチでdirect -23.77%・全セッション完全分離。ただしThinLTOに最適な条件での上限に近い値）が、実タイトルではcpu/wall単純平均+1.08%（悪化方向）・サーマル一致組でも-1.45%で分布が重なり改善を分離できない。代償はBuild ipa 5分57秒→51分34秒（約9倍・1回につき約46分増）、UnityFramework+24.31%、il2cppセクション+43.85%、ipa+5.60%
- **対処**: iOSではThinLTOを適用しない（Androidのみ採用）。再検討は、プロファイラでIL2CPPの静的な直接呼び出しが明確なボトルネックと判明し、かつ約46分の追加ビルド時間を許容できる場合に限る

### BD10 Xcode LLVM_LTOでIL2CPP生成コードにLTOを掛けようとする設定
- **レベル**: Warning
- **対象**: ビルド設定（xcconfig / ポストプロセス）
- **適用条件**: il2cpp, ios
- **検出パターン**: `LLVM_LTO = YES_THIN`（またはYES）をpbxproj/xcconfig/`OnPostprocessBuild`で設定し生成コードへの効果を期待
- **grep_pattern**: `LLVM_LTO\s*=\s*YES`（Xcode側でコンパイルされるネイティブプラグイン等への意図的な設定は正当なため除外。手動確認）
- **理由**: iOSのGameAssemblyターゲットはSourcesビルドフェーズが空でシェルスクリプトがil2cppを起動するだけ。生成コードのコンパイルフラグ入口は`AdditionalIl2CppArgs`のみでXcode設定は効かない
- **対処**: 設定を削除するか`AdditionalIl2CppArgs`経由に修正（ただしBD09のとおりiOSは見送り推奨）

### BD11 LTO適用のサイレント失敗（適用検証の欠如）
- **レベル**: Warning
- **対象**: ビルド設定 / CIパイプライン
- **適用条件**: il2cpp
- **検出パターン**: (手動確認) LTOを有効化しているのにビルドパイプラインに適用検証ステップが無い
- **grep_pattern**: (手動確認)
- **理由**: 公式トグルは3条件（IL2CPPバックエンド / Strip Engine Code無効 / 非Developmentビルド）のいずれかを満たさないとエラーも警告も出ずにNoneとしてビルドされる（DevelopmentビルドON時はトグル自体がUIに表示されない）。手動thinルートも、フラグがコンパイル/リンクの片方にしか渡っていない・文字列出現数だけでは判別できないため検証が必要
- **対処**: CIのビルド後チェックに検証を組み込む: `Library/Bee/Player*-inputdata.json`のLinkTimeOptimization値のgrep、`Player*.dag.json`で`-flto=thin`がコンパイル・リンク両方に付いているかの確認、IL2CPP生成.oの先頭4バイト（bitcode `4243 c0de` / ELF `7f45 4c46`）確認

### BD19 IL2CPP LTO Mode設定によるLTO有効化の誤解
> 出典: UnityPerformanceTuningBible 2026 - リンク時最適化(LTO)
- **レベル**: Warning
- **対象**: ビルド設定 / ProjectSettings
- **適用条件**: il2cpp, android, unity>=6000.5
- **検出パターン**: Player Settings ▸ Configurationの`IL2CPP LTO Mode`を設定しているが、C++ Compiler ConfigurationがMasterでない。`IL2CPP LTO Mode`はLTOの有無ではなくThin/Fullのモードを選ぶ項目で、Master構成でなければ`libil2cpp.so`にLTOは適用されない
- **grep_pattern**: (手動確認 — Player Settings ▸ ConfigurationのIL2CPP LTO ModeとC++ Compiler Configurationの組み合わせを確認。補助として`il2cppCompilerConfiguration`のgrepでMaster以外を特定)
- **理由**: `libil2cpp.so`へのLTO有効化はC++ Compiler Configuration = Masterでのみ適用される。IL2CPP LTO Modeは6000.5から表示されるがAndroidでは6000.6以前はThinモード固定で、6000.6からMasterの場合にThin/Fullを選択できるだけ。6000.6/6000.7でも「ReleaseのままLTOだけ有効化する」公式ルートは無い。適用されないままだと、手動thinで実測された効果（合成ベンチdirect -21.8%、実タイトルUnityMain -3.21%）は得られない
- **対処**: LTOを効かせたいなら、Master構成にするか、Release構成のまま`--compiler-flags=-flto=thin --linker-flags=-flto=thin`の手動ルート（BD04の対処）を使う。いずれもBD11の適用検証を行う
